#! /usr/bin/python

#***********************************************************
#* Software License Agreement (BSD License)
#*
#*  Copyright (c) 2009, Willow Garage, Inc.
#*  All rights reserved.
#*
#*  Redistribution and use in source and binary forms, with or without
#*  modification, are permitted provided that the following conditions
#*  are met:
#*
#*   * Redistributions of source code must retain the above copyright
#*     notice, this list of conditions and the following disclaimer.
#*   * Redistributions in binary form must reproduce the above
#*     copyright notice, this list of conditions and the following
#*     disclaimer in the documentation and/or other materials provided
#*     with the distribution.
#*   * Neither the name of the Willow Garage nor the names of its
#*     contributors may be used to endorse or promote products derived
#*     from this software without specific prior written permission.
#*
#*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
#*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#*  POSSIBILITY OF SUCH DAMAGE.
#***********************************************************

import roslib
roslib.load_manifest('writing_core')
import rospy

import time
import sys
import math

from std_msgs.msg import Empty

from robot_actions.msg import NoArgumentsActionState
import pr2_robot_actions.msg
import robot_msgs
import robot_srvs
import python_actions

class FindHelperAction(python_actions.Action):

  def __init__(self, *args):
    python_actions.Action.__init__(self, args[0], args[1], args[2], args[3])
    name = args[0]
    
    try:
      self.gripper_controller = rospy.get_param(name + "/gripper_controller")
    except KeyError:
      self.gripper_controller = "r_gripper_position_controller"
      rospy.set_param(name + "/gripper_controller", self.gripper_controller)

    try:
      self.arm_controller = rospy.get_param(name + "/arm_controller")
    except KeyError:
      self.arm_controller = "r_arm_cartesian_trajectory_controller"
      rospy.set_param(name + "/arm_controller", self.arm_controller)

    self.gripper_controller_publisher = rospy.Publisher(self.gripper_controller + "/set_command", std_msgs.msg.Float64)

    
  def execute(self, goal):

    rospy.logdebug("%s: executing.", self.name)

    #open the gripper 
    self.gripper_controller_publisher(std_msgs.msg.Float64(0.35))

    mtp = robot_srvs.srv.MoveToPoseRequest()
    mtp.header.stamp = rospy.get_rostime()
    mtp.header.frame_id = "torso_lift_link"
    mtp.pose.position.x =
    mtp.pose.position.y =
    mtp.pose.position.z =
    mtp.pose.orientation.x =
    mtp.pose.orientation.y =
    mtp.pose.orientation.z =
    mtp.pose.orientation.w =

    #move the arm 
    try:
      move_arm = rospy.ServiceProxy(self.arm_controller + "/move_to", robot_srvs.srv.MoveToPose)
      resp = move_arm()
    except rospy.ServiceException, e:
      rospy.logerr("%s: failed to call move to service call.", self.name)  
      rospy.logdebug("%s: aborted.", self.name) 
      return python_actions.ABORTED  
    
    if self.isPreemptRequested():
      rospy.logdebug("%s: preempted.", self.name)
      return python_actions.PREEMPTED

    #ask for the pen by opening and closing the gripper
    self.gripper_controller_publisher(std_msgs.msg.Float64(0.4))
    time.sleep(0.5)
    self.gripper_controller_publisher(std_msgs.msg.Float64(0.35))
    time.sleep(0.5)
    self.gripper_controller_publisher(std_msgs.msg.Float64(0.4))

    self.update()
      
    rospy.logdebug("%s: succeeded.", self.name)   
    return python_actions.SUCCESS
        
        

#sys.exit()

if __name__ == '__main__':

  try:

    rospy.init_node("ask_for_pen")
    w = TrackHelperAction("ask_for_pen", Empty, robot_actions.msg.NoArgumentsActionState, Empty)
    w.run()
    rospy.spin();

  except KeyboardInterrupt, e:

    pass

  print "exiting"
