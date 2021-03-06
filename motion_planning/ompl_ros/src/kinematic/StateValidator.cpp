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

/** \author Ioan Sucan */

#include "ompl_ros/kinematic/StateValidator.h"

bool ompl_ros::ROSStateValidityPredicateKinematic::operator()(const ompl::base::State *s) const
{
    EnvironmentDescription *ed = model_->getEnvironmentDescription();
    return check(s, ed->collisionSpace, ed->group, ed->constraintEvaluator);
}

void ompl_ros::ROSStateValidityPredicateKinematic::setConstraints(const motion_planning_msgs::KinematicConstraints &kc)
{
    clearConstraints();
    model_->constraintEvaluator.add(model_->planningMonitor->getEnvironmentModel()->getRobotModel().get(), kc.pose_constraint);
}

void ompl_ros::ROSStateValidityPredicateKinematic::clearConstraints(void)
{
    model_->constraintEvaluator.clear();
}

void ompl_ros::ROSStateValidityPredicateKinematic::printSettings(std::ostream &out) const
{    
    out << "Path constraints:" << std::endl;
    model_->constraintEvaluator.print(out);
}

bool ompl_ros::ROSStateValidityPredicateKinematic::check(const ompl::base::State *s, collision_space::EnvironmentModel *em, planning_models::KinematicModel::JointGroup *jg,
							 const planning_environment::KinematicConstraintEvaluatorSet *kce) const
{
    jg->computeTransforms(s->values);
    
    bool valid = kce->decide(s->values, jg);
    if (valid)
    {
	em->updateRobotModel();
	valid = !em->isCollision();
    }
    
    return valid;
}
