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

#ifndef JOINT_STATES_SETTLER_JOINT_STATES_DEFLATER_H_
#define JOINT_STATES_SETTLER_JOINT_STATES_DEFLATER_H_

#include <mechanism_msgs/JointStates.h>
#include <settlerlib/deflated.h>
#include "deflated_joint_states.h"

namespace joint_states_settler
{

/**
 * \brief Given a set a joint names, efficiently extracts a subset of joint positions
 *
 * This class is generally more efficient than other methods, because it caches the the mapping
 * from the incoming joint_states message to the deflated vector.  It also updates the mapping
 * whenever the ordering in joint_states changes
 **/
class JointStatesDeflater
{
public:

  JointStatesDeflater();

  /**
   * \brief Specify which joints to extract
   * \param joint_names Ordered list of joint names to extract
   */
  void setDeflationJointNames(std::vector<std::string> joint_names);

  /**
   * \brief Perform the deflation on a joint_states message
   * \param joint_states Incoming JointStates message
   * \param Ouput datatype. Stores the deflated data, along with the original joint states message
   */
  void deflate(const mechanism_msgs::JointStatesConstPtr& joint_states, DeflatedJointStates& deflated_elem);

private:
  std::vector<unsigned int> mapping_;
  std::vector<std::string> joint_names_;

  /**
   * \brief Given a stereotypical JointStates message, computes the mapping
   * from JointStates to the deflated data
   */
  void updateMapping(const mechanism_msgs::JointStates& joint_states);

};

}



#endif