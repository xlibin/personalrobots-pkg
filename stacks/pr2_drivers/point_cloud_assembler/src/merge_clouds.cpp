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

@b MergeClouds is a node capable of combining point clouds,
potentially from different sensors

**/

#include <ros/ros.h>
#include <robot_msgs/PointCloud.h>
#include <tf/message_notifier.h>
#include <tf/transform_listener.h>

#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

class MergeClouds
{
public:

    MergeClouds(void)
    {
	cloudOut_ = nh_.advertise<robot_msgs::PointCloud>("cloud_out", 1);
	nh_.param<std::string>("~output_frame", output_frame_, std::string());
	nh_.param<double>("~max_frequency", max_freq_, 0.0);
	newCloud1_ = newCloud2_ = false;
	
	if (output_frame_.empty())
	    ROS_ERROR("No output frame specified for merging pointclouds");
	
	// make sure we don't publish too fast
	if (max_freq_ > 1000.0 || max_freq_ < 0.0)
	    max_freq_ = 0.0;
	
	if (max_freq_ > 0.0)
	{
	    timer_ = nh_.createTimer(ros::Duration(1.0/max_freq_), boost::bind(&MergeClouds::onTimer, this, _1));
	    haveTimer_ = true;
	}
	else
	    haveTimer_ = false;
	
	cloudNotifier1_ = new tf::MessageNotifier<robot_msgs::PointCloud>(tf_, boost::bind(&MergeClouds::receiveCloud1, this, _1), "cloud_in1", output_frame_, 1);
	cloudNotifier2_ = new tf::MessageNotifier<robot_msgs::PointCloud>(tf_, boost::bind(&MergeClouds::receiveCloud2, this, _1), "cloud_in2", output_frame_, 1);
    }

    ~MergeClouds(void)
    {
	delete cloudNotifier1_;
	delete cloudNotifier2_;
    }
    
private:

    void onTimer(const ros::TimerEvent &e)
    {
	if (newCloud1_ && newCloud2_)
	    publishClouds();
    }
    
    void publishClouds(void)
    {
	lock1_.lock();	
	lock2_.lock();	

	newCloud1_ = false;
	newCloud2_ = false;

	robot_msgs::PointCloud out;
	if (cloud1_.header.stamp > cloud2_.header.stamp)
	    out.header = cloud1_.header;
	else
	    out.header = cloud2_.header;
	
	out.pts.resize(cloud1_.pts.size() + cloud2_.pts.size());
	
	// copy points
	std::copy(cloud1_.pts.begin(), cloud1_.pts.end(), out.pts.begin());
	std::copy(cloud2_.pts.begin(), cloud2_.pts.end(), out.pts.begin() + cloud1_.pts.size());
	
	// copy common channels
	for (unsigned int i = 0 ; i < cloud1_.chan.size() ; ++i)
	    for (unsigned int j = 0 ; j < cloud2_.chan.size() ; ++j)
		if (cloud1_.chan[i].name == cloud2_.chan[j].name)
		{
		    ROS_ASSERT(cloud1_.chan[i].vals.size() == cloud1_.pts.size());
		    ROS_ASSERT(cloud2_.chan[j].vals.size() == cloud2_.pts.size());
		    unsigned int oc = out.chan.size();
		    out.chan.resize(oc + 1);
		    out.chan[oc].name = cloud1_.chan[i].name;
		    out.chan[oc].vals.resize(cloud1_.chan[i].vals.size() + cloud2_.chan[j].vals.size());
		    std::copy(cloud1_.chan[i].vals.begin(), cloud1_.chan[i].vals.end(), out.chan[oc].vals.begin());
		    std::copy(cloud2_.chan[j].vals.begin(), cloud2_.chan[j].vals.end(), out.chan[oc].vals.begin() + cloud1_.chan[i].vals.size());
		    break;
		}
	
	lock1_.unlock();	
	lock2_.unlock();	

	cloudOut_.publish(out);
    }
    
    void receiveCloud1(const robot_msgs::PointCloudConstPtr &cloud)
    {
	lock1_.lock();	
	processCloud(cloud, cloud1_);
	lock1_.unlock();
	newCloud1_ = true;
	if (!haveTimer_ && newCloud2_)
	    publishClouds();
    }

    void receiveCloud2(const robot_msgs::PointCloudConstPtr &cloud)
    {
	lock2_.lock();
	processCloud(cloud, cloud2_);
	lock2_.unlock();
	newCloud2_ = true;	
	if (!haveTimer_ && newCloud1_)
	    publishClouds();
    }
    
    void processCloud(const robot_msgs::PointCloudConstPtr &cloud, robot_msgs::PointCloud &cloudOut)
    {
	if (output_frame_ != cloud->header.frame_id)
	    tf_.transformPointCloud(output_frame_, *cloud, cloudOut);
	else
	    cloudOut = *cloud;
    }
    
    ros::NodeHandle       nh_;
    tf::TransformListener tf_;

    ros::Timer            timer_;
    bool                  haveTimer_;
    
    ros::Publisher        cloudOut_;
    double                max_freq_;
    std::string           output_frame_;

    tf::MessageNotifier<robot_msgs::PointCloud> *cloudNotifier1_;
    tf::MessageNotifier<robot_msgs::PointCloud> *cloudNotifier2_;
    bool                   newCloud1_;
    bool                   newCloud2_;
    robot_msgs::PointCloud cloud1_;
    robot_msgs::PointCloud cloud2_;
    boost::mutex           lock1_;
    boost::mutex           lock2_;
    
};

    
int main(int argc, char **argv)
{
    ros::init(argc, argv, "merge_clouds", ros::init_options::AnonymousName);

    MergeClouds mc;
    ros::spin();
    
    return 0;
}
