/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Author: Stuart Glaser

#ifndef CARTESIAN_HYBRID_CONTROLLER_H
#define CARTESIAN_HYBRID_CONTROLLER_H

#include "robot_mechanism_controllers/cartesian_hybrid_controller.h"
#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/chainfksolvervel_recursive.hpp>
#include <kdl/chainjnttojacsolver.hpp>
#include <kdl/jacobian.hpp>
#include "tf_conversions/tf_kdl.h"
#include "realtime_tools/realtime_publisher.h"
#include "control_toolbox/pid.h"
#include "visualization_msgs/Marker.h"
#include "angles/angles.h"

#include "std_msgs/Float64MultiArray.h"

namespace controller {

void TransformKDLToMsg(const KDL::Frame &k, robot_msgs::Pose &m)
{
  tf::Transform tf;
  tf::TransformKDLToTF(k, tf);
  tf::PoseTFToMsg(tf, m);
}

void TwistKDLToMsg(const KDL::Twist &k, robot_msgs::Twist &m)
{
  m.vel.x = k.vel.x();
  m.vel.y = k.vel.y();
  m.vel.z = k.vel.z();
  m.rot.x = k.rot.x();
  m.rot.y = k.rot.y();
  m.rot.z = k.rot.z();
}

void WrenchKDLToMsg(const KDL::Wrench &k, robot_msgs::Wrench &m)
{
  m.force.x = k.force.x();
  m.force.y = k.force.y();
  m.force.z = k.force.z();
  m.torque.x = k.torque.x();
  m.torque.y = k.torque.y();
  m.torque.z = k.torque.z();
}

CartesianHybridController::CartesianHybridController()
  : robot_(NULL), last_time_(0), use_filter_(false)
{
}

bool CartesianHybridController::initXml(mechanism::RobotState *robot, TiXmlElement *config)
{
  assert(robot);
  robot_ = robot;
  std::string name = config->Attribute("name") ? config->Attribute("name") : "";
  if (name.empty())
  {
    ROS_ERROR("No name for CartesianHybridController");
    return false;
  }

  ros::Node *node = ros::Node::instance();

  // Chain

  std::string root_link, tip_link;
  if (!node->getParam(name + "/root_link", root_link)) {
    ROS_ERROR("No root link specified");
    return false;
  }
  if (!node->getParam(name + "/tip_link", tip_link)) {
    ROS_ERROR("No tip link specified");
    return false;
  }
  if (!chain_.init(robot->model_, root_link, tip_link))
    return false;
  chain_.toKDL(kdl_chain_);

  // Pids

  control_toolbox::Pid temp_pid;

  if (!temp_pid.initParam(name + "/pose/fb_trans"))
    return false;
  for (size_t i = 0; i < 3; ++i)
    pose_pids_[i] = temp_pid;

  if (!temp_pid.initParam(name + "/pose/fb_rot"))
    return false;
  for (size_t i = 0; i < 3; ++i)
    pose_pids_[i+3] = temp_pid;

  if (!temp_pid.initParam(name + "/twist/fb_trans"))
    return false;
  for (size_t i = 0; i < 3; ++i)
    twist_pids_[i] = temp_pid;

  if (!temp_pid.initParam(name + "/twist/fb_rot"))
    return false;
  for (size_t i = 0; i < 3; ++i)
    twist_pids_[i+3] = temp_pid;

  // Filter

  if (node->hasParam(name + "/twist_filter"))
  {
    use_filter_ = true;
    std::string filter_xml;
    node->getParam(name + "/twist_filter", filter_xml);

    TiXmlDocument doc;
    doc.Parse(filter_xml.c_str());
    if (!doc.RootElement())
    {
      ROS_ERROR("%s: Could not parse twist_filter xml", name.c_str());
      return false;
    }

    if (!twist_filter_.configure(6, doc.RootElement()))
      return false;
    ROS_INFO("%s: Successfully configured twist_filter", name.c_str());
  }
  else
    use_filter_ = false;

  // Initial mode

  if (!node->getParam(name + "/initial_mode", initial_mode_))
    initial_mode_ = robot_msgs::TaskFrameFormalism::FORCE;

  // Saturated velocity

  node->param(name + "/saturated_velocity", saturated_velocity_, -1.0);
  node->param(name + "/saturated_rot_velocity", saturated_rot_velocity_, -1.0);

  // Tool frame

  if (node->hasParam(name + "/tool_frame"))
  {
    if (!node->getParam(name + "/tool_frame/translation/x", tool_frame_offset_.p[0]) ||
        !node->getParam(name + "/tool_frame/translation/y", tool_frame_offset_.p[1]) ||
        !node->getParam(name + "/tool_frame/translation/z", tool_frame_offset_.p[2]))
    {
      ROS_ERROR("Tool frame was missing elements of the translation");
      return false;
    }
    tf::Quaternion q;
    if (!node->getParam(name + "/tool_frame/rotation/x", q[0]) ||
        !node->getParam(name + "/tool_frame/rotation/y", q[1]) ||
        !node->getParam(name + "/tool_frame/rotation/z", q[2]) ||
        !node->getParam(name + "/tool_frame/rotation/w", q[3]))
    {
      ROS_ERROR("Tool frame was missing elements of the rotation");
      return false;
    }
    tool_frame_offset_.M = KDL::Rotation::Quaternion(q[0], q[1], q[2], q[3]);
  }
  else
  {
    ROS_DEBUG("No tool frame specified");
    tool_frame_offset_ = KDL::Frame::Identity();
  }

  // Default commands

  task_frame_offset_ = KDL::Frame::Identity();

  return true;
}

void CartesianHybridController::update()
{
  if (!chain_.allCalibrated(robot_->joint_states_))
    return;
  double time = robot_->hw_->current_time_;
  double dt = time - last_time_;
  last_time_ = time;

  // Measures the current pose and twist

  // Finds the pose/twist of the ee frame w.r.t. the chain root
  KDL::JntArrayVel jnt_vel(kdl_chain_.getNrOfJoints());
  chain_.getVelocities(robot_->joint_states_, jnt_vel);
  KDL::FrameVel ee_in_root;
  KDL::ChainFkSolverVel_recursive fkvel_solver(kdl_chain_);
  fkvel_solver.JntToCart(jnt_vel, ee_in_root);

  // The pose/twist of the tool frame w.r.t. the task frame
  KDL::FrameVel tool = task_frame_offset_.Inverse() * ee_in_root * tool_frame_offset_;
  pose_meas_ = tool.GetFrame();
  twist_meas_ = tool.GetTwist();

  // Computes the desired wrench from the command

  // Computes the filtered twist
  if (use_filter_)
  {
    std::vector<double> tmp_twist(6);
    for (size_t i = 0; i < 6; ++i)
      tmp_twist[i] = twist_meas_[i];
    twist_filter_.update(tmp_twist, tmp_twist);
    for (size_t i = 0; i < 6; ++i)
      twist_meas_filtered_[i] = tmp_twist[i];
  }
  else
  {
    twist_meas_filtered_ = twist_meas_;
  }

  // Computes the desired pose
  for (int i = 0; i < 6; ++i)
  {
    if (mode_[i] == robot_msgs::TaskFrameFormalism::POSITION)
      pose_desi_[i] = setpoint_[i];
    else
      pose_desi_[i] = 0.0;
  }

  // Computes the pose error
  pose_error_.vel = tool.p.p - pose_desi_.vel;
  pose_error_.rot =
    diff(KDL::Rotation::RPY(
           mode_[3] == robot_msgs::TaskFrameFormalism::POSITION ? setpoint_[3] : 0.0,
           mode_[4] == robot_msgs::TaskFrameFormalism::POSITION ? setpoint_[4] : 0.0,
           mode_[5] == robot_msgs::TaskFrameFormalism::POSITION ? setpoint_[5] : 0.0),
         tool.M.R);

  // Computes the desired twist
  for (int i = 0; i < 6; ++i)
  {
    switch (mode_[i])
    {
    case robot_msgs::TaskFrameFormalism::POSITION:
      twist_desi_[i] = pose_pids_[i].updatePid(pose_error_[i], twist_meas_filtered_[i], dt);
      break;
    case robot_msgs::TaskFrameFormalism::VELOCITY:
      twist_desi_[i] = setpoint_[i];
      break;
    }
  }

  // Limits the velocity
  if (saturated_velocity_ >= 0.0)
  {
    if (twist_desi_.vel.Norm() > saturated_velocity_)
    {
      twist_desi_.vel = saturated_velocity_ * twist_desi_.vel / twist_desi_.vel.Norm();
    }
  }
  if (saturated_rot_velocity_ >= 0.0)
  {
    if (twist_desi_.rot.Norm() > saturated_rot_velocity_)
    {
      twist_desi_.rot = saturated_rot_velocity_ * twist_desi_.rot / twist_desi_.rot.Norm();
    }
  }

  for (int i = 0; i < 6; ++i)
  {
    twist_error_[i] = twist_meas_filtered_[i] - twist_desi_[i];
  }

  // Computes the desired wrench
  for (int i = 0; i < 6; ++i)
  {
    switch (mode_[i])
    {
    case robot_msgs::TaskFrameFormalism::POSITION:
    case robot_msgs::TaskFrameFormalism::VELOCITY:
      wrench_desi_[i] = twist_pids_[i].updatePid(twist_error_[i], dt);
      break;
    case robot_msgs::TaskFrameFormalism::FORCE:
      wrench_desi_[i] = setpoint_[i];
      break;
    default:
      abort();
    }
  }

  // Transforms the wrench from the task frame to the chain root frame
  KDL::Wrench wrench_in_root;
  wrench_in_root.force = task_frame_offset_.M * wrench_desi_.force;
  wrench_in_root.torque = task_frame_offset_.M * wrench_desi_.torque;

  // Finds the Jacobian for the tool
  KDL::ChainJntToJacSolver jac_solver(kdl_chain_);
  KDL::Jacobian ee_jacobian(kdl_chain_.getNrOfJoints());
  KDL::Jacobian jacobian(kdl_chain_.getNrOfJoints());  // Tool Jacobian
  jac_solver.JntToJac(jnt_vel.q, ee_jacobian);
  KDL::changeRefFrame(ee_jacobian, tool_frame_offset_, jacobian);

  // jnt_eff = jacobian * wrench
  KDL::JntArray jnt_eff(kdl_chain_.getNrOfJoints());
  for (size_t i = 0; i < kdl_chain_.getNrOfJoints(); ++i)
  {
    jnt_eff(i) = 0;
    for (size_t j = 0; j < 6; ++j)
      jnt_eff(i) += jacobian(j,i) * wrench_in_root(j);
  }

  chain_.addEfforts(jnt_eff, robot_->joint_states_);
}

bool CartesianHybridController::starting()
{
  task_frame_offset_ = KDL::Frame::Identity();
  //tool_frame_offset_ = KDL::Frame::Identity();


  switch(initial_mode_)
  {
  case robot_msgs::TaskFrameFormalism::POSITION: {
    // Finds the starting pose/twist
    KDL::JntArrayVel jnt_vel(kdl_chain_.getNrOfJoints());
    chain_.getVelocities(robot_->joint_states_, jnt_vel);
    KDL::FrameVel frame;
    KDL::ChainFkSolverVel_recursive fkvel_solver(kdl_chain_);
    fkvel_solver.JntToCart(jnt_vel, frame);
    frame = frame * tool_frame_offset_;

    for (size_t i = 0; i < 6; ++i) {
      mode_[i] = initial_mode_;
    }
    for (size_t i = 0; i < 3; ++i) {
      setpoint_[i] = frame.p.p[i];
    }
    frame.M.R.GetRPY(setpoint_[3], setpoint_[4], setpoint_[5]);
    break;
  }
  case robot_msgs::TaskFrameFormalism::VELOCITY:
    for (size_t i = 0; i < 6; ++i) {
      mode_[i] = initial_mode_;
      setpoint_[i] = 0.0;
    }
    break;
  case robot_msgs::TaskFrameFormalism::FORCE:
    for (size_t i = 0; i < 6; ++i) {
      mode_[i] = initial_mode_;
      setpoint_[i] = 0.0;
    }
    break;
  default:
    return false;
  }

  return true;
}

ROS_REGISTER_CONTROLLER(CartesianHybridControllerNode)

CartesianHybridControllerNode::CartesianHybridControllerNode()
: TF(*ros::Node::instance(), false, ros::Duration(10.0)), loop_count_(0)
{
}

CartesianHybridControllerNode::~CartesianHybridControllerNode()
{
  ros::Node *node = ros::Node::instance();
  node->unsubscribe(name_ + "/command");
  node->unadvertiseService(name_ + "/set_tool_frame");
}

bool CartesianHybridControllerNode::initXml(mechanism::RobotState *robot, TiXmlElement *config)
{
  if (!c_.initXml(robot, config))
    return false;

  if (!config->Attribute("name"))
    return false;
  name_ = config->Attribute("name");

  ros::Node *node = ros::Node::instance();

  task_frame_name_ = c_.chain_.getLinkName(0);

  node->subscribe(name_ + "/command", command_msg_, &CartesianHybridControllerNode::command, this, 5);

  pub_state_.reset(new realtime_tools::RealtimePublisher<robot_mechanism_controllers::CartesianHybridState>(name_ + "/state", 1));
  pub_tf_.reset(new realtime_tools::RealtimePublisher<tf::tfMessage>("/tf_message", 5));
  pub_tf_->msg_.transforms.resize(1);

  node->advertiseService(name_ + "/set_tool_frame", &CartesianHybridControllerNode::setToolFrame, this);

  return true;

}

void CartesianHybridControllerNode::update()
{

  KDL::Twist last_pose_desi = c_.pose_desi_;
  KDL::Twist last_twist_desi = c_.twist_desi_;
  KDL::Wrench last_wrench_desi = c_.wrench_desi_;

  c_.update();

  if (++loop_count_ % 10 == 0)
  {
    if (pub_state_->trylock())
    {
      pub_state_->msg_.header.frame_id = task_frame_name_;

      TwistKDLToMsg(c_.pose_error_, pub_state_->msg_.pose_error);
      TwistKDLToMsg(c_.twist_error_, pub_state_->msg_.twist_error);

      TwistKDLToMsg(c_.pose_desi_, pub_state_->msg_.last_pose_desi);
      TwistKDLToMsg(c_.twist_meas_, pub_state_->msg_.last_twist_meas);
      TwistKDLToMsg(c_.twist_meas_filtered_, pub_state_->msg_.last_twist_meas_filtered);
      TwistKDLToMsg(c_.twist_desi_, pub_state_->msg_.last_twist_desi);
      WrenchKDLToMsg(c_.wrench_desi_, pub_state_->msg_.last_wrench_desi);

      KDL::Twist last_pose_meas;
      last_pose_meas.vel = c_.pose_meas_.p;
      c_.pose_meas_.M.GetRPY(last_pose_meas.rot[0],
                             last_pose_meas.rot[1],
                             last_pose_meas.rot[2]);
      TwistKDLToMsg(last_pose_meas, pub_state_->msg_.last_pose_meas);

      pub_state_->unlockAndPublish();
    }
    if (pub_tf_->trylock())
    {
      //pub_tf_->msg_.transforms[0].header.stamp.fromSec();
      pub_tf_->msg_.transforms[0].header.frame_id = name_ + "/tool_frame";
      pub_tf_->msg_.transforms[0].parent_id = c_.chain_.getLinkName();
      tf::Transform t;
      tf::TransformKDLToTF(c_.tool_frame_offset_, t);
      tf::TransformTFToMsg(t, pub_tf_->msg_.transforms[0].transform);
      pub_tf_->unlockAndPublish();
    }
  }
}

void CartesianHybridControllerNode::command()
{
  task_frame_name_ = command_msg_.header.frame_id;
  tf::Stamped<tf::Transform> task_frame;

  try {
    //ROS_INFO("Waiting on transform (%.3lf vs %.3lf)", command_msg_.header.stamp.toSec(), ros::Time::now().toSec());
    while (!TF.canTransform(c_.chain_.getLinkName(0), command_msg_.header.frame_id, command_msg_.header.stamp))
    {
      usleep(10000);
    }
    //ROS_INFO("Got transform.");
    TF.lookupTransform(c_.chain_.getLinkName(0), command_msg_.header.frame_id, command_msg_.header.stamp,
                       task_frame);
  }
  catch (tf::TransformException &ex)
  {
    ROS_WARN("Transform Exception %s", ex.what());
    return;
  }
  tf::TransformTFToKDL(task_frame, c_.task_frame_offset_);

  c_.mode_[0] = (int)command_msg_.mode.vel.x;
  c_.mode_[1] = (int)command_msg_.mode.vel.y;
  c_.mode_[2] = (int)command_msg_.mode.vel.z;
  c_.mode_[3] = (int)command_msg_.mode.rot.x;
  c_.mode_[4] = (int)command_msg_.mode.rot.y;
  c_.mode_[5] = (int)command_msg_.mode.rot.z;
  c_.setpoint_[0] = command_msg_.value.vel.x;
  c_.setpoint_[1] = command_msg_.value.vel.y;
  c_.setpoint_[2] = command_msg_.value.vel.z;
  c_.setpoint_[3] = command_msg_.value.rot.x;
  c_.setpoint_[4] = command_msg_.value.rot.y;
  c_.setpoint_[5] = command_msg_.value.rot.z;
}

bool CartesianHybridControllerNode::setToolFrame(
  robot_srvs::SetPoseStamped::Request &req,
  robot_srvs::SetPoseStamped::Response &resp)
{
  if (!TF.canTransform(c_.chain_.getLinkName(-1), req.p.header.frame_id,
                       req.p.header.stamp, ros::Duration(3.0)))
  {
    ROS_ERROR("Cannot transform %s -> %s at %lf", c_.chain_.getLinkName(-1).c_str(),
              req.p.header.frame_id.c_str(), req.p.header.stamp.toSec());
    return false;
  }

  robot_msgs::PoseStamped tool_in_tip_msg;
  tf::Transform tool_in_tip;
  TF.transformPose(c_.chain_.getLinkName(-1), req.p, tool_in_tip_msg);
  tf::PoseMsgToTF(tool_in_tip_msg.pose, tool_in_tip);
  tf::TransformTFToKDL(tool_in_tip, c_.tool_frame_offset_);
  double rpy[3]; c_.tool_frame_offset_.M.GetRPY(rpy[0], rpy[1], rpy[2]);
  ROS_INFO("(%.3lf, %.3lf, %.3lf)@(%.2lf, %.2lf, %.2lf)",
           c_.tool_frame_offset_.p[0], c_.tool_frame_offset_.p[1], c_.tool_frame_offset_.p[2],
           rpy[0], rpy[1], rpy[2]);
  return true;
}

}

#endif
