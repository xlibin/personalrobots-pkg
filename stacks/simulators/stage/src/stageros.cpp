/*
 *  stageros
 *  Copyright (c) 2008, Willow Garage, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**

@mainpage

@htmlinclude manifest.html
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// libstage
#include <stage.hh>

// roscpp
#include <ros/node.h>
#include "boost/thread/mutex.hpp"
#include <sensor_msgs/LaserScan.h>
#include <nav_msgs/Odometry.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <roslib/Time.h>

#include "tf/transform_broadcaster.h"

#define USAGE "stageros <worldfile>"
#define ODOM "odom"
#define BASE_SCAN "base_scan"
#define BASE_POSE_GROUND_TRUTH "base_pose_ground_truth"
#define CMD_VEL "cmd_vel"

// Our node
class StageNode : public ros::Node
{
  private:
    // Messages that we'll send or receive
    geometry_msgs::Twist *velMsgs;
    sensor_msgs::LaserScan *laserMsgs;
    nav_msgs::Odometry *odomMsgs;
    nav_msgs::Odometry *groundTruthMsgs;
    roslib::Time timeMsg;

    // A mutex to lock access to fields that are used in message callbacks
    boost::mutex msg_lock;

    // The models that we're interested in
    std::vector<Stg::StgModelLaser *> lasermodels;
    std::vector<Stg::StgModelPosition *> positionmodels;

    // A helper function that is executed for each stage model.  We use it
    // to search for models of interest.
    static void ghfunc(gpointer key, Stg::StgModel* mod, StageNode* node);

    // Appends the given robot ID to the given message name.  If omitRobotID
    // is true, an unaltered copy of the name is returned.
    const char *mapName(const char *name, size_t robotID);

    tf::TransformBroadcaster tf;

    // Last time that we received a velocity command
    ros::Time base_last_cmd;
    ros::Duration base_watchdog_timeout;

    // Current simulation time
    ros::Time sim_time;

  public:
    // Constructor; stage itself needs argc/argv.  fname is the .world file
    // that stage should load.
     StageNode(int argc, char** argv, bool gui, const char* fname);
    ~StageNode();

    // Subscribe to models of interest.  Currently, we find and subscribe
    // to the first 'laser' model and the first 'position' model.  Returns
    // 0 on success (both models subscribed), -1 otherwise.
    int SubscribeModels();

    // Do one update of the simulator.  May pause if the next update time
    // has not yet arrived.
    void Update();

    // Message callback for a MsgBaseVel message, which set velocities.
    void cmdvelReceived();

    // The main simulator object
    Stg::StgWorld* world;
};

// since stageros is single-threaded, this is OK. revisit if that changes!
const char *
StageNode::mapName(const char *name, size_t robotID)
{
  if (positionmodels.size() > 1)
  {
    static char buf[100];
    snprintf(buf, sizeof(buf), "/robot_%d/%s", robotID, name);
    return buf;
  }
  else
    return name;
}

void
StageNode::ghfunc(gpointer key,
                  Stg::StgModel* mod,
                  StageNode* node)
{
  if (dynamic_cast<Stg::StgModelLaser *>(mod))
    node->lasermodels.push_back(dynamic_cast<Stg::StgModelLaser *>(mod));
  if (dynamic_cast<Stg::StgModelPosition *>(mod))
    node->positionmodels.push_back(dynamic_cast<Stg::StgModelPosition *>(mod));
}

void
StageNode::cmdvelReceived()
{
  boost::mutex::scoped_lock lock(msg_lock);
  for (size_t r = 0; r < this->positionmodels.size(); r++)
  {
    this->positionmodels[r]->SetSpeed(this->velMsgs[r].linear.x, 
                                      this->velMsgs[r].linear.y, 
                                      this->velMsgs[r].angular.z);
  }
  this->base_last_cmd = this->sim_time;
}

StageNode::StageNode(int argc, char** argv, bool gui, const char* fname) :
  ros::Node("stageros")
{
  this->sim_time.fromSec(0.0);
  this->base_last_cmd.fromSec(0.0);
  double t;
  param("~base_watchdog_timeout", t, 0.2);
  this->base_watchdog_timeout.fromSec(t);

  // initialize libstage
  Stg::Init( &argc, &argv );

  if(gui)
    this->world = new Stg::StgWorldGui(800, 700, "Stage (ROS)");
  else
    this->world = new Stg::StgWorld();

  this->world->Load(fname);

  this->world->ForEachModel((GHFunc)ghfunc, this);
  if (lasermodels.size() != positionmodels.size())
  {
    ROS_FATAL("number of position models and laser models must be equal in "
              "the world file.");
    ROS_BREAK();
  }
  size_t numRobots = positionmodels.size();
  ROS_INFO("found %d position model(s) in the file", numRobots);

  this->velMsgs = new geometry_msgs::Twist[numRobots];
  this->laserMsgs = new sensor_msgs::LaserScan[numRobots];
  this->odomMsgs = new nav_msgs::Odometry[numRobots];
  this->groundTruthMsgs = new nav_msgs::Odometry[numRobots];
}


// Subscribe to models of interest.  Currently, we find and subscribe
// to the first 'laser' model and the first 'position' model.  Returns
// 0 on success (both models subscribed), -1 otherwise.
//
// Eventually, we should provide a general way to map stage models onto ROS
// topics, similar to Player .cfg files.
int
StageNode::SubscribeModels()
{
  setParam("/use_sim_time", true);

  for (size_t r = 0; r < this->positionmodels.size(); r++)
  {
    if(this->lasermodels[r])
      this->lasermodels[r]->Subscribe();
    else
    {
      ROS_ERROR("no laser");
      return(-1);
    }
    if(this->positionmodels[r])
      this->positionmodels[r]->Subscribe();
    else
    {
      ROS_ERROR("no position");
      return(-1);
    }
    advertise<sensor_msgs::LaserScan>(mapName(BASE_SCAN,r), 10);
    advertise<nav_msgs::Odometry>(mapName(ODOM,r), 10);
    advertise<nav_msgs::Odometry>(
                                  mapName(BASE_POSE_GROUND_TRUTH,r), 10);
    subscribe(mapName(CMD_VEL,r), velMsgs[r], &StageNode::cmdvelReceived, 10);
  }
  advertise<roslib::Time>("/time",10);
  return(0);
}

StageNode::~StageNode()
{
  delete[] velMsgs;
  delete[] laserMsgs;
  delete[] odomMsgs;
  delete[] groundTruthMsgs;
}

void
StageNode::Update()
{
  // Wait until it's time to update
  this->world->PauseUntilNextUpdateTime();
  boost::mutex::scoped_lock lock(msg_lock);

  // Let the simulator update (it will sleep if there's time)
  this->world->Update();

  this->sim_time.fromSec(world->SimTimeNow() / 1e6);

  // TODO make this only affect one robot if necessary
  if((this->base_watchdog_timeout.toSec() > 0.0) &&
      ((this->sim_time - this->base_last_cmd) >= this->base_watchdog_timeout))
  {
    for (size_t r = 0; r < this->positionmodels.size(); r++)
      this->positionmodels[r]->SetSpeed(0.0, 0.0, 0.0);
  }

  // Get latest laser data
  for (size_t r = 0; r < this->lasermodels.size(); r++)
  {
    Stg::stg_laser_sample_t* samples = this->lasermodels[r]->GetSamples();
    if(samples)
    {
      // Translate into ROS message format and publish
      Stg::stg_laser_cfg_t cfg = this->lasermodels[r]->GetConfig();
      this->laserMsgs[r].angle_min = -cfg.fov/2.0;
      this->laserMsgs[r].angle_max = +cfg.fov/2.0;
      this->laserMsgs[r].angle_increment = cfg.fov/(double)(cfg.sample_count-1);
      this->laserMsgs[r].range_min = 0.0;
      this->laserMsgs[r].range_max = cfg.range_bounds.max;
      this->laserMsgs[r].ranges.resize(cfg.sample_count);
      this->laserMsgs[r].intensities.resize(cfg.sample_count);
      for(unsigned int i=0;i<cfg.sample_count;i++)
      {
        this->laserMsgs[r].ranges[i] = samples[i].range;
        this->laserMsgs[r].intensities[i] = (uint8_t)samples[i].reflectance;
      }

      this->laserMsgs[r].header.frame_id = mapName("base_laser", r);
      this->laserMsgs[r].header.stamp = sim_time;
      publish(mapName(BASE_SCAN,r), this->laserMsgs[r]);
    }

    // Also publish the base->base_laser Tx.  This could eventually move
    // into being retrieved from the param server as a static Tx.
    Stg::stg_pose_t lp = this->lasermodels[r]->GetPose();
    tf.sendTransform(tf::Stamped<tf::Transform> 
        (tf::Transform(tf::Quaternion(lp.a, 0, 0), 
                       tf::Point(lp.x, lp.y, 0.15)),
         sim_time, mapName("base_laser", r), mapName("base_link", r)));
    // Send the identity transform between base_footprint and base_link
    tf::Transform txIdentity(tf::Quaternion(0, 0, 0), tf::Point(0, 0, 0));
    tf.sendTransform(tf::Stamped<tf::Transform>
        (txIdentity,
         sim_time, mapName("base_link", r), mapName("base_footprint", r)));
    // Get latest odometry data
    // Translate into ROS message format and publish
    this->odomMsgs[r].pose_with_covariance.pose.position.x = this->positionmodels[r]->est_pose.x;
    this->odomMsgs[r].pose_with_covariance.pose.position.y = this->positionmodels[r]->est_pose.y;
    this->odomMsgs[r].pose_with_covariance.pose.orientation = tf::createQuaternionMsgFromYaw(this->positionmodels[r]->est_pose.a);
    Stg::stg_velocity_t v = this->positionmodels[r]->GetVelocity();
    this->odomMsgs[r].twist_with_covariance.twist.linear.x = v.x;
    this->odomMsgs[r].twist_with_covariance.twist.linear.y = v.y;
    this->odomMsgs[r].twist_with_covariance.twist.angular.z = v.a;

    //@todo Publish stall on a separate topic when one becomes available
    //this->odomMsgs[r].stall = this->positionmodels[r]->Stall();
    //
    this->odomMsgs[r].header.frame_id = mapName("odom", r);
    this->odomMsgs[r].header.stamp = sim_time;

    publish(mapName(ODOM,r),this->odomMsgs[r]);

    tf::Quaternion q;
    tf::quaternionMsgToTF(odomMsgs[r].pose_with_covariance.pose.orientation, q);
    tf::Stamped<tf::Transform> tx(
        tf::Transform(
          q, 
          tf::Point(odomMsgs[r].pose_with_covariance.pose.position.x, odomMsgs[r].pose_with_covariance.pose.position.y, 0.0)),
        sim_time, mapName("base_footprint", r), mapName("odom", r));
    this->tf.sendTransform(tx);
      
    // Also publish the ground truth pose
    Stg::stg_pose_t gpose = this->positionmodels[r]->GetGlobalPose();
    // Note that we correct for Stage's screwed-up coord system.
    tf::Transform gt(tf::Quaternion(gpose.a-M_PI/2.0, 0, 0), 
        tf::Point(gpose.y, -gpose.x, 0.0));

    this->groundTruthMsgs[r].pose_with_covariance.pose.position.x     = gt.getOrigin().x();
    this->groundTruthMsgs[r].pose_with_covariance.pose.position.y     = gt.getOrigin().y();
    this->groundTruthMsgs[r].pose_with_covariance.pose.position.z     = gt.getOrigin().z();
    this->groundTruthMsgs[r].pose_with_covariance.pose.orientation.x  = gt.getRotation().x();
    this->groundTruthMsgs[r].pose_with_covariance.pose.orientation.y  = gt.getRotation().y();
    this->groundTruthMsgs[r].pose_with_covariance.pose.orientation.z  = gt.getRotation().z();
    this->groundTruthMsgs[r].pose_with_covariance.pose.orientation.w  = gt.getRotation().w();

    this->groundTruthMsgs[r].header.frame_id = mapName("odom", r);
    this->groundTruthMsgs[r].header.stamp = sim_time;

    publish(mapName(BASE_POSE_GROUND_TRUTH,r), this->groundTruthMsgs[r]);

  }

  this->timeMsg.rostime = sim_time;
  publish("/time", this->timeMsg);
}

int 
main(int argc, char** argv)
{ 
  if( argc < 2 )
  {
    puts(USAGE);
    exit(-1);
  }

  ros::init(argc,argv);

  bool gui = true;
  for(int i=0;i<(argc-1);i++)
  {
    if(!strcmp(argv[i], "-g"))
      gui = false;
  }

  StageNode sn(argc-1,argv,gui,argv[argc-1]);

  if(sn.SubscribeModels() != 0)
    exit(-1);

  while(sn.ok() && !sn.world->TestQuit())
  {
    sn.Update();
  }

  exit(0);
}

