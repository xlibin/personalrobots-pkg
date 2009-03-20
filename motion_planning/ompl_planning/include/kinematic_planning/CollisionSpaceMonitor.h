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

#include "kinematic_planning/KinematicStateMonitor.h"
#include <collision_space/environmentODE.h>

#include <robot_msgs/PointCloud.h>
#include <robot_msgs/CollisionMap.h>
#include <robot_msgs/AttachedObject.h>
#include <robot_srvs/CollisionCheckState.h>

/** Main namespace */
namespace kinematic_planning
{
    
    /**
       
       @b CollisionSpaceMonitor is a class which in addition to being aware of a robot model,
       is also aware of a collision space.
       
       <hr>
       
       @section topic ROS topics
       
       Subscribes to (name/type):
       - @b "collision_map"/CollisionMap : data describing the 3D environment
       - @b "attach_object"/AttachedObject : data describing an object to be attached to a link
       
       Publishes to (name/type):
       - None
       
       <hr>
       
       @section services ROS services
       
       Uses (name/type):
       - None
       
       Provides (name/type):
       - @b "set_collision_state"/CollisionCheckState : service to allow enabling and disabling collision checking for links
       
       <hr>
       
       @section notes Notes

       This class inherits from KinematicStateMonitor. Additional
       relevant topics, services and parameters are documented in
       KinematicStateMonitor.

       This class uses the following special groups (from the URDF document)
       
       - "collision_check" with the flag "collision": if present, this is used
       to define the links of the robot that are to be checked for
       collision. If not present, NO COLLISION CHECKING IS PERFORMED!
       
       - any group name with the flag "self_collision": checks the links in
       each group for collision against every other link in the same group.
       
    **/

    class CollisionSpaceMonitor : public KinematicStateMonitor
    {

    public:
	
        CollisionSpaceMonitor(ros::Node *node, collision_space::EnvironmentModel *collisionSpace = NULL) : KinematicStateMonitor(node)
	{
	    if (collisionSpace)
		m_collisionSpace = collisionSpace;
	    else
		m_collisionSpace = new collision_space::EnvironmentModelODE();
	    m_collisionSpace->setSelfCollision(true);
	    // hack for having ground plane
	    m_collisionSpace->addStaticPlane(0.0, 0.0, 1.0, -0.01);
	    
	    m_node->subscribe("collision_map", m_collisionMap, &CollisionSpaceMonitor::collisionMapCallback, this, 1);
	    m_node->advertiseService("set_collision_state", &CollisionSpaceMonitor::setCollisionState, this);
	    	
	    m_node->subscribe("attach_object", m_attachedObject, &CollisionSpaceMonitor::attachObject, this, 1);
	}

	virtual ~CollisionSpaceMonitor(void)
	{
	    if (m_collisionSpace)
	    {
		delete m_collisionSpace;
		m_kmodel = NULL;
	    }
	}
	
	void attachObject(void)
	{
	    m_collisionSpace->lock();
	    int model_id = m_collisionSpace->getModelID(m_attachedObject.robot_name);
	    planning_models::KinematicModel::Link *link = model_id >= 0 ? m_kmodel->getLink(m_attachedObject.link_name) : NULL;
	    
	    if (link)
	    {	
		// clear the previously attached bodies 
		for (unsigned int i = 0 ; i < link->attachedBodies.size() ; ++i)
		    delete link->attachedBodies[i];
		unsigned int n = m_attachedObject.get_objects_size();
		link->attachedBodies.resize(n);

		// create the new ones
		for (unsigned int i = 0 ; i < n ; ++i)
		{
		    link->attachedBodies[i] = new planning_models::KinematicModel::AttachedBody();
		    
		    robot_msgs::PointStamped center;
		    robot_msgs::PointStamped centerP;
		    center.point.x = m_attachedObject.objects[i].center.x;
		    center.point.y = m_attachedObject.objects[i].center.y;
		    center.point.z = m_attachedObject.objects[i].center.z;
		    center.header  = m_attachedObject.header;
		    m_tf.transformPoint(m_attachedObject.link_name, center, centerP);
		    
		    link->attachedBodies[i]->attachTrans.setOrigin(btVector3(centerP.point.x, centerP.point.y, centerP.point.z));
		    
		    // this is a HACK! we should have orientation
		    planning_models::shapes::Box *box = new planning_models::shapes::Box();
		    box->size[0] = m_attachedObject.objects[i].max_bound.x - m_attachedObject.objects[i].min_bound.x;
		    box->size[1] = m_attachedObject.objects[i].max_bound.y - m_attachedObject.objects[i].min_bound.y;
		    box->size[2] = m_attachedObject.objects[i].max_bound.z - m_attachedObject.objects[i].min_bound.z;
		    link->attachedBodies[i]->shape = box;
		}
		
		// update the collision model
		m_collisionSpace->updateAttachedBodies(model_id);
		ROS_INFO("Link '%s' on '%s' has %d objects attached", m_attachedObject.link_name.c_str(), m_attachedObject.robot_name.c_str(), n);
	    }
	    else
		ROS_WARN("Unable to attach object to link '%s' on '%s'", m_attachedObject.link_name.c_str(), m_attachedObject.robot_name.c_str());
	    m_collisionSpace->unlock();
	    if (link)
		afterAttachBody(link);
	}
	
	bool setCollisionState(robot_srvs::CollisionCheckState::Request &req, robot_srvs::CollisionCheckState::Response &res)
	{
	    m_collisionSpace->lock();
	    int model_id = m_collisionSpace->getModelID(req.robot_name);
	    if (model_id >= 0)
		res.value = m_collisionSpace->setCollisionCheck(model_id, req.link_name, req.value ? true : false);
	    else
		res.value = -1;
	    m_collisionSpace->unlock();
	    if (res.value == -1)
		ROS_WARN("Unable to change collision checking state for link '%s' on '%s'", req.link_name.c_str(), req.robot_name.c_str());
	    else
		ROS_INFO("Collision checking for link '%s' on '%s' is now %s", req.link_name.c_str(), req.robot_name.c_str(), res.value ? "enabled" : "disabled");
	    return true;	    
	}
	
	virtual void setRobotDescription(robot_desc::URDF *file)
	{
	    KinematicStateMonitor::setRobotDescription(file);
	    if (m_kmodel)
	    {
		std::vector<std::string> links;
		robot_desc::URDF::Group *g = file->getGroup("collision_check");
		if (g && g->hasFlag("collision"))
		    links = g->linkNames;
		m_collisionSpace->lock();
		unsigned int cid = m_collisionSpace->addRobotModel(m_kmodel, links);
		m_collisionSpace->unlock();
		addSelfCollisionGroups(cid, file);
	    }	    
	}
	
    	virtual void defaultPosition(void)
	{
	    KinematicStateMonitor::defaultPosition();
	    if (m_collisionSpace && m_collisionSpace->getModelCount() == 1)
		m_collisionSpace->updateRobotModel(0);
	}
	
	bool isMapUpdated(double sec)
	{
	    if (sec > 0 && m_lastMapUpdate < ros::Time::now() - ros::Duration(sec))
		return false;
	    else
		return true;
	}

    protected:
	
	void addSelfCollisionGroups(unsigned int cid, robot_desc::URDF *model)
	{
	    std::vector<robot_desc::URDF::Group*> groups;
	    model->getGroups(groups);
	    
	    m_collisionSpace->lock();
	    for (unsigned int i = 0 ; i < groups.size() ; ++i)
		if (groups[i]->hasFlag("self_collision"))
		    m_collisionSpace->addSelfCollisionGroup(cid, groups[i]->linkNames);
	    m_collisionSpace->unlock();
	}
	
	void collisionMapCallback(void)
	{
	    unsigned int n = m_collisionMap.get_boxes_size();
	    ROS_DEBUG("Received %u points (collision map)", n);
	    
	    beforeWorldUpdate();
	    
	    ros::WallTime startTime = ros::WallTime::now();
	    double *data = new double[4 * n];	
	    for (unsigned int i = 0 ; i < n ; ++i)
	    {
		unsigned int i4 = i * 4;	    
		data[i4    ] = m_collisionMap.boxes[i].center.x;
		data[i4 + 1] = m_collisionMap.boxes[i].center.y;
		data[i4 + 2] = m_collisionMap.boxes[i].center.z;
		
		data[i4 + 3] = radiusOfBox(m_collisionMap.boxes[i].extents);
	    }
	    
	    m_collisionSpace->lock();
	    m_collisionSpace->clearObstacles();
	    m_collisionSpace->addPointCloud(n, data);
	    m_collisionSpace->unlock();
	    
	    delete[] data;
	    
	    double tupd = (ros::WallTime::now() - startTime).toSec();
	    ROS_DEBUG("Updated world model in %f seconds", tupd);
	    m_lastMapUpdate = ros::Time::now();
	    
	    afterWorldUpdate();
	}
	
	virtual void beforeWorldUpdate(void)
	{
	}
	
	virtual void afterWorldUpdate(void)
	{
	}
	
	virtual void afterAttachBody(planning_models::KinematicModel::Link *link)
	{
	}
	
	double radiusOfBox(robot_msgs::Point32 &point)
	{
	    return std::max(std::max(point.x, point.y), point.z) * 1.73;
	}
	
	robot_msgs::CollisionMap              m_collisionMap;
	collision_space::EnvironmentModel    *m_collisionSpace;

	// add or remove objects to be attached to a link
	robot_msgs::AttachedObject            m_attachedObject;
	
	ros::Time                             m_lastMapUpdate;
    };
     
} // kinematic_planning
