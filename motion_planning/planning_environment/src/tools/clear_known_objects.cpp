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

/**

@b ClearKnownObjects is a node that removes known objects from a
collision map.

**/

#include <ros/ros.h>
#include "planning_environment/monitors/kinematic_model_state_monitor.h"
#include "planning_environment/util/construct_object.h"

#include <geometric_shapes/bodies.h>
#include <tf/message_filter.h>
#include <message_filters/subscriber.h>
#include <sensor_msgs/PointCloud.h>
#include <geometry_msgs/PoseStamped.h>
#include <mapping_msgs/AttachedObject.h>
#include <mapping_msgs/ObjectInMap.h>

class ClearKnownObjects
{
public:

    ClearKnownObjects(void)
    {    
	rm_ = new planning_environment::RobotModels("robot_description");
	if (rm_->loadedModels())
	{
	    kmsm_ = new planning_environment::KinematicModelStateMonitor(rm_, &tf_);
	    nh_.param<std::string>("~fixed_frame", fixed_frame_, kmsm_->getFrameId());
	    nh_.param<double>("~object_scale", scale_, 1.0);
	    nh_.param<double>("~object_padd", padd_, 0.02);
	    nh_.param<std::string>("~sensor_frame", sensor_frame_, std::string());
	    
	    ROS_INFO("Clearing points on known objects using '%s' as fixed frame, %f padding and %f scaling", fixed_frame_.c_str(), padd_, scale_);
	    
	    cloudPublisher_ = nh_.advertise<sensor_msgs::PointCloud>("cloud_out", 1);	    
	    kmsm_->setOnAfterAttachBodyCallback(boost::bind(&ClearKnownObjects::attachObjectEvent, this, _1));
	    
	    cloudSubscriber_ = new message_filters::Subscriber<sensor_msgs::PointCloud>(nh_, "cloud_in", 1);
	    cloudFilter_ = new tf::MessageFilter<sensor_msgs::PointCloud>(*cloudSubscriber_, tf_, fixed_frame_, 1);
	    cloudFilter_->registerCallback(boost::bind(&ClearKnownObjects::cloudCallback, this, _1));
	    
	    objectInMapSubscriber_ = new message_filters::Subscriber<mapping_msgs::ObjectInMap>(nh_, "object_in_map", 1024);
	    objectInMapFilter_ = new tf::MessageFilter<mapping_msgs::ObjectInMap>(*objectInMapSubscriber_, tf_, fixed_frame_, 1024);
	    objectInMapFilter_->registerCallback(boost::bind(&ClearKnownObjects::objectInMapCallback, this, _1));
	}
	else
	{
	    kmsm_ = NULL;
	    cloudFilter_ = NULL;
	    cloudSubscriber_ = NULL;
	    objectInMapFilter_ = NULL;
	    objectInMapSubscriber_ = NULL;
	}
    }

    ~ClearKnownObjects(void)
    {
	if (cloudFilter_)
	    delete cloudFilter_;
	if (cloudSubscriber_)
	    delete cloudSubscriber_;
	if (objectInMapFilter_)
	    delete objectInMapFilter_;
	if (objectInMapSubscriber_)
	    delete objectInMapSubscriber_;
	if (kmsm_)
	    delete kmsm_;
	if (rm_)
	    delete rm_;
	for (unsigned int i = 0 ; i < attachedObjects_.size() ; ++i)
	    delete attachedObjects_[i].body;	
	for (std::map<std::string, KnownObject>::iterator it = objectsInMap_.begin() ; it != objectsInMap_.end() ; ++it)
	    delete it->second.body;
    }
    
    void run(void)
    {   
	if (rm_->loadedModels())
	{
	    kmsm_->waitForState();
	    ros::spin();
	}
    }
    
private:

    struct KnownObject
    {
	KnownObject(void)
	{
	    body = NULL;
	}
	
	void usePose(const btTransform &pose)
	{
	    body->setPose(pose);
	    body->computeBoundingSphere(bsphere);
	    rsquare = bsphere.radius * bsphere.radius;
	}
	
	bodies::Body           *body;
	bodies::BoundingSphere  bsphere;
	double                  rsquare;
    };
    
    void computeMask(const sensor_msgs::PointCloud &cloud, std::vector<int> &mask)
    {
	// check if we have attached bodies
	if (attachedObjects_.size() > 0)
	{
	    // update the poses for the attached bodies
	    kmsm_->getKinematicModel()->lock();
	    kmsm_->getKinematicModel()->computeTransforms(kmsm_->getRobotState()->getParams());
	    if (fixed_frame_ != kmsm_->getFrameId())
	    {
		std::string errStr;
		ros::Time tm;
		if (!tf_.getLatestCommonTime(kmsm_->getFrameId(), fixed_frame_, tm, &errStr) == tf::NO_ERROR)
		{
		    ROS_ERROR("Unable to transform attached body from frame '%s' to frame '%s'", kmsm_->getFrameId().c_str(), fixed_frame_.c_str());		
		    if (!errStr.empty())
			ROS_ERROR("TF said: %s", errStr.c_str());
		}
		else
		{
		    bool error = false;
		    for (unsigned int i = 0 ; i < attachedObjects_.size() ; ++i)
		    {
			tf::Stamped<tf::Pose> pose(attached_[i]->globalTrans, tm, kmsm_->getFrameId());
			tf::Stamped<tf::Pose> poseOut;
			try
			{
			    tf_.transformPose(fixed_frame_, pose, poseOut);
			    attachedObjects_[i].usePose(poseOut);
			}
			catch(...)
			{
			    error = true;
			}
		    }
		    if (error)
			ROS_ERROR("Errors encountered when transforming attached bodies from frame '%s' to frame '%s'", kmsm_->getFrameId().c_str(), fixed_frame_.c_str());
		}
	    }
	    else
	    {
		for (unsigned int i = 0 ; i < attachedObjects_.size() ; ++i)
		    attachedObjects_[i].usePose(attached_[i]->globalTrans);
	    }
	    kmsm_->getKinematicModel()->unlock();
	}
	
	// transform pointcloud into fixed frame, if needed
	sensor_msgs::PointCloud temp;
	const sensor_msgs::PointCloud *cloudTransf = &cloud;
	if (fixed_frame_ != cloud.header.frame_id)
	{
	    tf_.transformPointCloud(fixed_frame_, cloud, temp);
	    cloudTransf = &temp;
	}
	
	btVector3 sensor_pos(0, 0, 0);
	
	// compute the origin of the sensor in the frame of the cloud
	if (!sensor_frame_.empty())
	{
	    try
	    {
		tf::Stamped<btTransform> transf;
		tf_.lookupTransform(fixed_frame_, sensor_frame_, cloudTransf->header.stamp, transf);
		sensor_pos = transf.getOrigin();
	    }
	    catch(...)
	    {
		sensor_pos.setValue(0, 0, 0);
		ROS_ERROR("Unable to lookup transform from %s to %s", sensor_frame_.c_str(), fixed_frame_.c_str());
	    }
	}
	
	// compute mask for cloud
	int n = cloud.points.size();
	mask.resize(n);
	
#pragma omp parallel for
	for (int i = 0 ; i < n ; ++i)
	{
	    btVector3 pt = btVector3(cloudTransf->points[i].x, cloudTransf->points[i].y, cloudTransf->points[i].z);
	    btVector3 dir(sensor_pos - pt);
	    dir.normalize();
	    int out = 1;
	    
	    for (unsigned int j = 0 ; out && j < attachedObjects_.size() ; ++j)
		if (attachedObjects_[j].bsphere.center.distance2(pt) < attachedObjects_[j].rsquare)
		    if (attachedObjects_[j].body->containsPoint(pt) || attachedObjects_[j].body->intersectsRay(pt, dir))
			out = 0;

	    for (std::map<std::string, KnownObject>::iterator it = objectsInMap_.begin() ; out && it != objectsInMap_.end() ; ++it)
		if (it->second.bsphere.center.distance2(pt) < it->second.rsquare)
		    if (it->second.body->containsPoint(pt) || it->second.body->intersectsRay(pt, dir))
			out = 0;
	    
	    mask[i] = out;
	}
    }
    
    void cloudCallback(const sensor_msgs::PointCloudConstPtr &cloud)
    {
	ROS_DEBUG("Got pointcloud that is %f seconds old", (ros::Time::now() - cloud->header.stamp).toSec());
	
	std::vector<int> mask;
	bool filter = false;
	
	updateObjects_.lock();

	if  (attachedObjects_.size() > 0 || objectsInMap_.size() > 0)
	{
	    computeMask(*cloud, mask);
	    filter = true;
	}
	
	updateObjects_.unlock();

	if (filter)
	{
	    // publish new cloud
	    const unsigned int np = cloud->points.size();
	    sensor_msgs::PointCloud data_out;
	    
	    // fill in output data with points that are NOT in the known objects
	    data_out.header = cloud->header;	  
	    
	    data_out.points.resize(0);
	    data_out.points.reserve(np);
	    
	    data_out.channels.resize(cloud->channels.size());
	    for (unsigned int i = 0 ; i < data_out.channels.size() ; ++i)
	    {
		ROS_ASSERT(cloud->channels[i].values.size() == cloud->points.size());
		data_out.channels[i].name = cloud->channels[i].name;
		data_out.channels[i].values.reserve(cloud->channels[i].values.size());
	    }
	    
	    for (unsigned int i = 0 ; i < np ; ++i)
		if (mask[i])
		{
		    data_out.points.push_back(cloud->points[i]);
		    for (unsigned int j = 0 ; j < data_out.channels.size() ; ++j)
			data_out.channels[j].values.push_back(cloud->channels[j].values[i]);
		}

	    ROS_DEBUG("Published filtered cloud (%d points out of %d)", (int)data_out.points.size(), (int)cloud->points.size());
	    cloudPublisher_.publish(data_out);
	}
	else
	{
	    cloudPublisher_.publish(*cloud);
	    ROS_DEBUG("Republished unchanged cloud");
	}
    }
       
    void objectInMapCallback(const mapping_msgs::ObjectInMapConstPtr &objectInMap)
    {
	updateObjects_.lock();
	if (objectInMap->action == mapping_msgs::ObjectInMap::ADD)
	{
	    // add the object to the map
	    shapes::Shape *shape = planning_environment::constructObject(objectInMap->object);
	    if (shape)
	    {
		bool err = false;
		geometry_msgs::PoseStamped psi;
		geometry_msgs::PoseStamped pso;
		psi.pose = objectInMap->pose;
		psi.header = objectInMap->header;
		try
		{
		    tf_.transformPose(fixed_frame_, psi, pso);
		}
		catch(...)
		{
		    err = true;
		}
		
		if (err)
		    ROS_ERROR("Unable to transform object '%s' in frame '%s' to frame '%s'", objectInMap->id.c_str(), objectInMap->header.frame_id.c_str(), fixed_frame_.c_str());
		else
		{
		    btTransform pose;
		    tf::poseMsgToTF(pso.pose, pose);
		    KnownObject kb;
		    kb.body = bodies::createBodyFromShape(shape);
		    kb.body->setScale(scale_);
		    kb.body->setPadding(padd_);
		    kb.usePose(pose);
		    objectsInMap_[objectInMap->id] = kb;
		    ROS_DEBUG("Added object '%s' to list of known objects", objectInMap->id.c_str());
		}
		delete shape;
	    }
	}
	else
	{
	    if (objectsInMap_.find(objectInMap->id) != objectsInMap_.end())
	    {
		delete objectsInMap_[objectInMap->id].body;
		objectsInMap_.erase(objectInMap->id);
	    }
	    else
		ROS_WARN("Object '%s' is not in list of known objects", objectInMap->id.c_str());
	}
	updateObjects_.unlock();
    }
    
    void attachObjectEvent(const planning_models::KinematicModel::Link* link)
    {
	updateObjects_.lock();
	attached_.clear();
	std::vector<const planning_models::KinematicModel::Link*> links;
	rm_->getKinematicModel()->getLinks(links);
	for (unsigned int i = 0 ; i < links.size() ; ++i)
	    for (unsigned int j = 0 ; j < links[i]->attachedBodies.size() ; ++j)
		attached_.push_back(links[i]->attachedBodies[j]);
	for (unsigned int i = 0 ; i < attachedObjects_.size() ; ++i)
	    delete attachedObjects_[i].body;	
	attachedObjects_.resize(attached_.size());
	for (unsigned int i = 0 ; i < attached_.size() ; ++i)
	{
	    attachedObjects_[i].body = bodies::createBodyFromShape(attached_[i]->shape);
	    attachedObjects_[i].body->setScale(scale_);
	    attachedObjects_[i].body->setPadding(padd_);
	}
	updateObjects_.unlock();
    }
    
    ros::NodeHandle                                                    nh_;
    tf::TransformListener                                              tf_;
    planning_environment::RobotModels                                 *rm_;
    planning_environment::KinematicModelStateMonitor                  *kmsm_;

    message_filters::Subscriber<mapping_msgs::ObjectInMap>            *objectInMapSubscriber_;
    tf::MessageFilter<mapping_msgs::ObjectInMap>                      *objectInMapFilter_;
    message_filters::Subscriber<sensor_msgs::PointCloud>              *cloudSubscriber_;
    tf::MessageFilter<sensor_msgs::PointCloud>                        *cloudFilter_;

    std::string                                                        fixed_frame_;
    boost::mutex                                                       updateObjects_;
    ros::Publisher                                                     cloudPublisher_;    

    std::string                                                        sensor_frame_;
    double                                                             scale_;
    double                                                             padd_;
    std::vector<const planning_models::KinematicModel::AttachedBody*>  attached_;
    std::vector<KnownObject>                                           attachedObjects_;
    std::map<std::string, KnownObject>                                 objectsInMap_;
};

   
int main(int argc, char **argv)
{
    ros::init(argc, argv, "clear_known_objects");

    ClearKnownObjects cko;
    cko.run();
    
    return 0;
}
