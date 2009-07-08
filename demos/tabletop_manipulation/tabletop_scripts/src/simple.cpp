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
 *
 *
 *********************************************************************/

/* \author: Ioan Sucan */

#include <ros/ros.h>
#include <robot_actions/action_client.h>

#include <pr2_robot_actions/MoveArmGoal.h>
#include <pr2_robot_actions/MoveArmState.h>

#include <boost/thread/thread.hpp>
#include <recognition_lambertian/FindObjectPoses.h>
#include <visualization_msgs/Marker.h>

void sendPoint(ros::Publisher &vmPub, const roslib::Header &header, double x, double y, double z)
{
    visualization_msgs::Marker mk;
    
    mk.header = header;
    mk.ns = "test_recognition";
    mk.id = 1;
    mk.type = visualization_msgs::Marker::SPHERE;
    mk.action = visualization_msgs::Marker::ADD;
    mk.pose.position.x = x;
    mk.pose.position.y = y;
    mk.pose.position.z = z;
    mk.pose.orientation.w = 1.0;
    
    mk.scale.x = mk.scale.y = mk.scale.z = 0.01;
    
    mk.color.a = 1.0;
    mk.color.r = 1.0;
    mk.color.g = 0.04;
    mk.color.b = 0.04;
    
    vmPub.publish(mk);
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "test_recognition_planning");

    ros::NodeHandle nh;
    recognition_lambertian::FindObjectPoses::Request  req;
    recognition_lambertian::FindObjectPoses::Response res;
    
    ros::Publisher vmPub = nh.advertise<visualization_msgs::Marker>("visualization_marker", 10240);
    
    ros::ServiceClient client = nh.serviceClient<recognition_lambertian::FindObjectPoses>("table_top/find_object_poses");
    if (client.call(req, res))
    {
	ROS_INFO("Found %d objects", (int)res.objects.size());
	if (res.objects.size() > 0)
	{
	    recognition_lambertian::TableTopObject obj = res.objects[0];
	    
	    robot_actions::ActionClient<pr2_robot_actions::MoveArmGoal, pr2_robot_actions::MoveArmState, int32_t> move_arm("move_arm");
	    int32_t                         feedback;
	    pr2_robot_actions::MoveArmGoal  goal;
	    
	    goal.goal_constraints.set_pose_constraint_size(1);
	    goal.goal_constraints.pose_constraint[0].pose.header = obj.pose.header;
	    
	    goal.goal_constraints.pose_constraint[0].link_name = "r_wrist_roll_link";
	    goal.goal_constraints.pose_constraint[0].pose.pose = obj.pose.pose;

	    goal.goal_constraints.pose_constraint[0].position_distance = 0.01;
	    goal.goal_constraints.pose_constraint[0].orientation_distance = 0.01;
	    goal.goal_constraints.pose_constraint[0].orientation_importance = 0.1;
	    goal.goal_constraints.pose_constraint[0].type =
		motion_planning_msgs::PoseConstraint::POSITION_XYZ + 
		motion_planning_msgs::PoseConstraint::ORIENTATION_RPY;
	    
	    sendPoint(vmPub, obj.pose.header, obj.pose.pose.position.x, obj.pose.pose.position.y, obj.pose.pose.position.z);
	    
	    if (move_arm.execute(goal, feedback, ros::Duration(10.0)) != robot_actions::SUCCESS)
		ROS_ERROR("failed achieving goal");
	}
    }
    else
	ROS_ERROR("Unable to call find_object_poses service");
    
    
    return 0;
}