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
 *   * Neither the name of Willow Garage nor the names of its
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

/* Author: Wim Meeusen */

#ifndef ACTION_UNLATCH_HANDLE_DOOR_H
#define ACTION_UNLATCH_HANDLE_DOOR_H

#include <ros/node.h>
#include <door_msgs/Door.h>
#include <door_msgs/DoorCmd.h>
#include <manipulation_msgs/TaskFrameFormalism.h>
#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <deprecated_srvs/MoveToPose.h>
#include <kdl/frames.hpp>
#include <robot_actions/action.h>
#include <boost/thread/mutex.hpp>


namespace door_handle_detector{

typedef boost::shared_ptr<geometry_msgs::Twist const> TffConstPtr;


class UnlatchHandleActionDoor: public robot_actions::Action<door_msgs::DoorCmd, door_msgs::Door>
{
public:
  UnlatchHandleActionDoor(tf::TransformListener& tf);
  ~UnlatchHandleActionDoor();

  virtual robot_actions::ResultStatus execute(const door_msgs::DoorCmd& goal, door_msgs::Door& feedback);

private:
  void tffCallback(const TffConstPtr& tff);

  ros::NodeHandle node_;
  ros::Publisher tff_pub_;
  tf::TransformListener& tf_;

  manipulation_msgs::TaskFrameFormalism tff_stop_, tff_handle_, tff_door_;

  geometry_msgs::Twist tff_state_;
  bool tff_state_received_;
  boost::mutex tff_mutex_;
  double time_door_move_;
};


}



#endif
