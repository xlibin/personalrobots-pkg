/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2009, Willow Garage, Inc.
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

#include "image_transport/image_publisher.h"
#include "image_transport/publisher_plugin.h"
#include <pluginlib/class_loader.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/foreach.hpp>

namespace image_transport {

struct ImagePublisher::Impl
{
  Impl()
    : loader("image_transport", "image_transport::PublisherPlugin")
  {
  }
  
  std::string topic;
  pluginlib::ClassLoader<PublisherPlugin> loader;
  boost::ptr_vector<PublisherPlugin> publishers;
};

ImagePublisher::ImagePublisher()
  : impl_(new Impl)
{
}

ImagePublisher::ImagePublisher(const ImagePublisher& rhs)
  : impl_(rhs.impl_)
{
}

ImagePublisher::~ImagePublisher()
{
}

void ImagePublisher::advertise(ros::NodeHandle& nh, const std::string& topic,
                               uint32_t queue_size, bool latch)
{
  impl_->topic = topic;
  
  BOOST_FOREACH(const std::string& lookup_name, impl_->loader.getDeclaredClasses()) {
    ROS_INFO("Loading %s", lookup_name.c_str());
    PublisherPlugin* pub = impl_->loader.createClassInstance(lookup_name);
    impl_->publishers.push_back(pub);
    // @todo: support overriding the topic names
    std::string sub_topic = pub->getDefaultTopic(topic);
    nh.setParam(sub_topic + "/transport_type", pub->getTransportType());
    pub->advertise(nh, sub_topic, queue_size, latch);
  }
}

uint32_t ImagePublisher::getNumSubscribers() const
{
  uint32_t count = 0;
  BOOST_FOREACH(const PublisherPlugin& pub, impl_->publishers)
    count += pub.getNumSubscribers();
  return count;
}

std::string ImagePublisher::getTopic() const
{
  return impl_->topic;
}

void ImagePublisher::publish(const sensor_msgs::Image& message) const
{
  BOOST_FOREACH(const PublisherPlugin& pub, impl_->publishers) {
    if (pub.getNumSubscribers() > 0)
      pub.publish(message);
  }
}

void ImagePublisher::publish(const sensor_msgs::ImageConstPtr& message) const
{
  publish(*message);
}

void ImagePublisher::shutdown()
{
  BOOST_FOREACH(PublisherPlugin& pub, impl_->publishers)
    pub.shutdown();
}

} //namespace image_transport