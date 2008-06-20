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

/**

@mainpage

@htmlinclude manifest.html

@b World3dMap is a node capable of building 3D maps out of point
cloud data. The code is incomplete: it currently only forwards cloud
data.

**/

#include "ros/node.h"
#include "rosthread/member_thread.h"
#include "rosthread/mutex.h"
#include "std_msgs/PointCloudFloat32.h"
#include "std_msgs/Log.h"
using namespace std_msgs;
using namespace ros::thread::member_thread;

static const char MAP_PUBLISH_TOPIC[] = "world_3d_map";

class World3DMap : public ros::node
{
public:

    World3DMap(void) : ros::node("World3DMap")
    {
	advertise<PointCloudFloat32>(MAP_PUBLISH_TOPIC);
	advertise<Log>("roserr");
	
	subscribe("full_cloud", inputCloud, &World3DMap::pointCloudCallback);
	param((string(MAP_PUBLISH_TOPIC)+"/max_publish_frequency").c_str(), maxPublishFrequency, 0.5);
	
	/* create a thread that does the processing of the input data.
	 * and one that handles the publishing of the data */
	active = true;
	working = false;
	shouldPublish = false;
	
	processMutex.lock();
	publishMutex.lock();
	processingThread = startMemberFunctionThread<World3DMap>(this, &World3DMap::processDataThread);
	publishingThread = startMemberFunctionThread<World3DMap>(this, &World3DMap::publishDataThread);
    }
    
    ~World3DMap(void)
    {
	/* terminate spawned threads */
	active = false;
	processMutex.unlock();
	
	pthread_join(*publishingThread, NULL);
	pthread_join(*processingThread, NULL);
    }
    
    void pointCloudCallback(void)
    {
	/* The idea is that if processing of previous input data is
	   not done, data will be discarded. Hopefully this discarding
	   of data will not happen, but we don't want the node to
	   postpone processing latest data just because it is not done
	   with older data. */
	
	flagMutex.lock();
	bool discard = working;
	if (!discard)
	    working = true;
	
	if (discard)
	{
	    /* log the fact that input was discarded */
	    Log l;
	    l.level = 20;
	    l.name  = get_name();
	    l.msg   = "Discarded point cloud data (previous input set not done processing)";
	    publish("roserr", l);
	}
	else
	{
	    toProcess = inputCloud;  /* copy data to a place where incoming messages do not affect it */
	    processMutex.unlock();   /* let the processing thread know that there is data to process */
	}
	flagMutex.unlock();
    }
    
    void processDataThread(void)
    {
	while (active)
	{
	    /* This mutex acts as a condition, but is safer (condition
	       messages may get lost or interrupted by signals) */
	    processMutex.lock();
	    if (active)
	    {
		worldDataMutex.lock();
		processData();
		worldDataMutex.unlock();
		
		/* notify the publishing thread that it can send data */
		shouldPublish = true;
	    }
	    
	    /* make a note that there is no active processing */
	    flagMutex.lock();
	    working = false;
	    flagMutex.unlock();
	}
    }
    

    void publishDataThread(void)
    {
	double us = 1000000.0/maxPublishFrequency;
	
	/* while everything else is running (map building) check if
	   there are any updates to send, but do so at most at the
	   maximally allowed frequency of sending data */
	while (active)
	{
	    // should change from usleep() to some other method when rostime looks better
	    usleep(us);
	    if (shouldPublish)
	    {
		worldDataMutex.lock();
		if (active)
		    publish(MAP_PUBLISH_TOPIC, toProcess);
		shouldPublish = false;
		worldDataMutex.unlock();
	    }
	}
    }
    
    void processData(void)
    {
	// build a 3D representation of the world
	// NEED TO FILL THIS IN
	printf("processing some cloud data: %d\n", toProcess.pts_size);
    }
    
private:
    
    PointCloudFloat32  inputCloud;
    PointCloudFloat32  toProcess;
    double             maxPublishFrequency;

    pthread_t         *processingThread;
    pthread_t         *publishingThread;
    ros::thread::mutex processMutex, publishMutex, worldDataMutex, flagMutex;
    bool               active, working, shouldPublish;
};


int main(int argc, char **argv)
{  
    ros::init(argc, argv);

    World3DMap map;
    map.spin();
    map.shutdown();
    
    return 0;    
}
