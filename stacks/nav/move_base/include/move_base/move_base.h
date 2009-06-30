/*********************************************************************
*
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
*
* Author: Eitan Marder-Eppstein
*********************************************************************/
#ifndef NAV_MOVE_BASE_ACTION_H_
#define NAV_MOVE_BASE_ACTION_H_
#include <robot_actions/action.h>
#include <robot_actions/action_runner.h>
#include <nav_robot_actions/MoveBaseState.h>
#include <nav_robot_actions/base_local_planner.h>
#include <nav_robot_actions/base_global_planner.h>
#include <robot_msgs/PoseStamped.h>
#include <ros/ros.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <costmap_2d/costmap_2d.h>
#include <costmap_2d/rate.h>
#include <vector>
#include <string>
#include <nav_srvs/Plan.h>
#include <visualization_msgs/Marker.h>
#include <robot_msgs/PoseDot.h>

namespace move_base {
  /**
   * @class MoveBase
   * @brief A class adhering to the robot_actions::Action interface that moves the robot base to a goal location.
   */
  class MoveBase : public robot_actions::Action<robot_msgs::PoseStamped, robot_msgs::PoseStamped> {
    public:
      /**
       * @brief  Constructor for the actions
       * @param name The name of the action
       * @param tf A reference to a TransformListener
       */
      MoveBase(std::string name, tf::TransformListener& tf);

      /**
       * @brief  Destructor - Cleans up
       */
      virtual ~MoveBase();

      /**
       * @brief  Runs whenever a new goal is sent to the move_base
       * @param goal The goal to pursue 
       * @param feedback Feedback that the action gives to a higher-level monitor, in this case, the position of the robot
       * @return The result of the execution, ie: Success, Preempted, Aborted, etc.
       */
      virtual robot_actions::ResultStatus execute(const robot_msgs::PoseStamped& goal, robot_msgs::PoseStamped& feedback);

    private:
      /**
       * @brief  A service call that can be made when the action is inactive that will return a plan
       * @param  req The goal request
       * @param  resp The plan request
       * @return True if planning succeeded, false otherwise
       */
      bool planService(nav_srvs::Plan::Request &req, nav_srvs::Plan::Response &resp);

      /**
       * @brief  Make a new global plan
       * @param  goal The goal to plan to
       */
      void makePlan(const robot_msgs::PoseStamped& goal);

      /**
       * @brief  Publish a goal to the visualizer
       * @param  goal The goal to visualize
       */
      void publishGoal(const robot_msgs::PoseStamped& goal);

      /**
       * @brief  Get the current pose of the robot in the specified frame
       * @param  frame The frame to get the pose in
       * @param  pose The pose returned
       */
      void getRobotPose(std::string frame, tf::Stamped<tf::Pose>& pose);

      /**
       * @brief  Resets the costmaps to the static map outside a given window
       */
      void resetCostmaps(double size_x, double size_y);

      void clearCostmapWindows(double size_x, double size_y);

      void resetState();

      bool tryPlan(robot_msgs::PoseStamped goal);

      ros::NodeHandle ros_node_;
      tf::TransformListener& tf_;
      nav_robot_actions::BaseLocalPlanner* tc_;
      costmap_2d::Costmap2DROS* planner_costmap_ros_, *controller_costmap_ros_;

      nav_robot_actions::BaseGlobalPlanner* planner_;
      std::vector<robot_msgs::PoseStamped> global_plan_;
      std::string robot_base_frame_, global_frame_;
      bool valid_plan_, new_plan_;
      boost::recursive_mutex lock_;
      robot_msgs::PoseStamped goal_;

      tf::Stamped<tf::Pose> global_pose_;
      double controller_frequency_, inscribed_radius_, circumscribed_radius_, planner_patience_, controller_patience_, clearing_radius_;
      bool attempted_rotation_, attempted_costmap_reset_;
      bool done_half_rotation_, done_full_rotation_;
      bool escaping_;
      ros::Time last_valid_control_;
      ros::Publisher vis_pub_, vel_pub_;
      bool shutdown_costmaps_;

  };
};
#endif
