/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/
#include <algorithm>
#include <pr2_mechanism_controllers/laser_scanner_traj_controller.h>
#include <angles/angles.h>

#include <math.h>

using namespace std;
using namespace controller;

ROS_REGISTER_CONTROLLER(LaserScannerTrajController)

LaserScannerTrajController::LaserScannerTrajController() : traj_(1)
{

}

LaserScannerTrajController::~LaserScannerTrajController()
{

}

bool LaserScannerTrajController::initXml(mechanism::RobotState *robot, TiXmlElement *config)
{
  if (!robot || !config)
    return false ;
  robot_ = robot ;

  // Look through XML to grab the joint name
  TiXmlElement *j = config->FirstChildElement("joint") ;
  if (!j)
    return false ;

  const char *jn = j->Attribute("name") ;

  if (!jn)
    return false ;
  std::string joint_name = jn ;

  joint_state_ = robot_->getJointState(joint_name) ;  // Need joint state to check calibrated flag

  joint_position_controller_.initXml(robot, config) ; //Pass down XML snippet to encapsulated joint_position_controller_

  TiXmlElement *max_rate_elem = config->FirstChildElement("max_rate") ;
  if (!max_rate_elem)
    return false ;
  if(max_rate_elem->QueryDoubleAttribute("value", &max_rate_) != TIXML_SUCCESS )
    return false ;

  TiXmlElement *max_acc_elem = config->FirstChildElement("max_acc") ;
  if (!max_acc_elem)
    return false ;
  if(max_acc_elem->QueryDoubleAttribute("value", &max_acc_) != TIXML_SUCCESS )
    return false ;

  return true ;
}

void LaserScannerTrajController::update()
{
  //if (!joint_state_->calibrated_)
  //  return;
  if (traj_lock_.trylock())
  {
    if (traj_duration_ > 1e-6)                                   // Short trajectories could make the mod_time calculation unstable
    {
      double profile_time = getCurProfileTime() ;

      trajectory::Trajectory::TPoint sampled_point ;
      sampled_point.dimension_ = 1 ;
      sampled_point.q_.resize(1) ;
      sampled_point.qdot_.resize(1) ;
      int result ;

      result = traj_.sample(sampled_point, profile_time) ;
      if (result > 0)
      {
        joint_position_controller_.setCommand(sampled_point.q_[0]) ;
        joint_position_controller_.update() ;
      }
    }
    traj_lock_.unlock() ;
  }
}

double LaserScannerTrajController::getCurProfileTime()
{
  double time = robot_->hw_->current_time_ ;
  double time_from_start = time - traj_start_time_ ;
  double mod_time = time_from_start - floor(time_from_start/traj_.getTotalTime())*traj_.getTotalTime() ;
  return mod_time ;
}

double LaserScannerTrajController::getProfileDuration()
{
  return traj_duration_ ;
}

void LaserScannerTrajController::setTrajectory(const std::vector<trajectory::Trajectory::TPoint>& traj_points, double max_rate, double max_acc, std::string interp)
{
  while (!traj_lock_.trylock())
    usleep(100) ;

  vector<double> max_rates ;
  max_rates.push_back(max_rate) ;
  vector<double> max_accs ;
  max_accs.push_back(max_acc) ;

  traj_.setMaxRates(max_rates) ;
  traj_.setMaxAcc(max_accs) ;
  traj_.setInterpolationMethod(interp) ;

  traj_.setTrajectory(traj_points) ;

  traj_start_time_ = robot_->hw_->current_time_ ;

  traj_duration_ = traj_.getTotalTime() ;

  traj_lock_.unlock() ;
}

void LaserScannerTrajController::setPeriodicCmd(const pr2_mechanism_controllers::PeriodicCmd& cmd_)
{
  if (cmd_.profile == "linear" ||
      cmd_.profile == "blended_linear")
  {
    double high_pt = cmd_.amplitude + cmd_.offset ;
    double low_pt = -cmd_.amplitude + cmd_.offset ;

    std::vector<trajectory::Trajectory::TPoint> tpoints ;

    trajectory::Trajectory::TPoint cur_point(1) ;

    cur_point.dimension_ = 1 ;

    cur_point.q_[0] = low_pt ;
    cur_point.time_ = 0.0 ;
    tpoints.push_back(cur_point) ;

    cur_point.q_[0] = high_pt ;
    cur_point.time_ = cmd_.period/2.0 ;
    tpoints.push_back(cur_point) ;

    cur_point.q_[0] = low_pt ;
    cur_point.time_ = cmd_.period ;
    tpoints.push_back(cur_point) ;

    setTrajectory(tpoints, max_rate_, max_acc_, cmd_.profile) ;
    ROS_INFO("LaserScannerTrajController: Periodic Command set") ;
  }
  else
  {
    ROS_WARN("Unknown Periodic Trajectory Type. Not setting command.") ;
  }
}

ROS_REGISTER_CONTROLLER(LaserScannerTrajControllerNode)
LaserScannerTrajControllerNode::LaserScannerTrajControllerNode(): node_(ros::node::instance()), c_()
{
  need_to_send_msg_ = false ;                                           // Haven't completed a sweep yet, so don't need to send a msg
  publisher_ = NULL ;                                                   // We don't know our topic yet, so we can't build it
}

LaserScannerTrajControllerNode::~LaserScannerTrajControllerNode()
{
  node_->unsubscribe(service_prefix_ + "/set_periodic_cmd") ;

  publisher_->stop() ;
  delete publisher_ ;    // Probably should wait on publish_->is_running() before exiting. Need to
                         //   look into shutdown semantics for realtime_publisher
}

void LaserScannerTrajControllerNode::update()
{
  c_.update() ;

  double cur_profile_time = c_.getCurProfileTime() ;

  // Check if we crossed the middle point of our profile
  if (cur_profile_time  >= c_.getProfileDuration()/2.0 &&
      prev_profile_time_ < c_.getProfileDuration()/2.0)
  {
    // Should we be populating header.stamp here? Or, we can simply let ros take care of the timestamp
    m_scanner_signal_.signal = 0 ;
    need_to_send_msg_ = true ;
  }
  else if (cur_profile_time < prev_profile_time_)        // Check if we wrapped around
  {
    m_scanner_signal_.signal = 1 ;
    need_to_send_msg_ = true ;
  }
  prev_profile_time_ = cur_profile_time ;

  // Use the realtime_publisher to try to send the message.
  //   If it fails sending, it's not a big deal, since we can just try again 1 ms later. No one will notice.
  if (need_to_send_msg_)
  {
    if (publisher_->trylock())
    {
      publisher_->msg_.header = m_scanner_signal_.header ;
      publisher_->msg_.signal = m_scanner_signal_.signal ;
      publisher_->unlockAndPublish() ;
      need_to_send_msg_ = false ;
    }
    //printf("tilt_laser: Signal trigger (%u)\n", m_scanner_signal_.signal) ;
    //std::cout << std::flush ;
  }
}

bool LaserScannerTrajControllerNode::initXml(mechanism::RobotState *robot, TiXmlElement *config)
{
  service_prefix_ = config->Attribute("name") ;

  if (!c_.initXml(robot, config))
  {
    ROS_ERROR("Error Loading LaserScannerTrajControllerNode XML") ;
    return false ;
  }

  node_->subscribe(service_prefix_ + "/set_periodic_cmd", cmd_, &LaserScannerTrajControllerNode::setPeriodicCmd, this, 1) ;

  if (publisher_ != NULL)               // Make sure that we don't memory leak if initXml gets called twice
    delete publisher_ ;
  publisher_ = new misc_utils::RealtimePublisher <pr2_mechanism_controllers::LaserScannerSignal> (service_prefix_ + "/laser_scanner_signal", 1) ;

  prev_profile_time_ = 0.0 ;

  return true ;
}

void LaserScannerTrajControllerNode::setPeriodicCmd()
{
  c_.setPeriodicCmd(cmd_) ;
}
