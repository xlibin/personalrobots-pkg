/*
 * articulation_learner.h
 *
 *  Created on: Aug 3, 2009
 *      Author: sturm
 */

#ifndef ARTICULATED_OBJECTS_H_
#define ARTICULATED_OBJECTS_H_

#include "ros/ros.h"
#include "topic_synchronizer2/topic_synchronizer.h"

#include "visualization_msgs/Marker.h"

#include "planar_objects/BoxObservations.h"
#include "planar_objects/BoxTracks.h"
#include "planar_objects/ArticulatedObjects.h"
#include <tf/transform_listener.h>
#include <tf/transform_broadcaster.h>

#include "vis_utils.h"
#include "track_utils.h"
#include "articulation_models.h"

namespace planar_objects
{

class ArticulationLearner
{
public:
  ros::NodeHandle nh;
  TopicSynchronizer sync;

  std::vector<ManifoldModel*> models;

  // PARAMETERS
  bool visualize;
  bool verbose;
  double dist_vis;

  double thres_trans;
  double thres_rot;

  int oldLines;
  int newLines;

  // MESSAGES - INCOMING
  ros::Subscriber tracks_sub;
  BoxTracksConstPtr tracks_msg;
  std::vector<btBoxTrack> tracks;

  // MESSAGES - OUTGOING
  ros::Publisher visualization_pub;
  ros::Publisher cloud_pub;
  ros::Publisher articulated_pub;

  // Constructor
  ArticulationLearner();

  // Callbacks
  void tracksCallback(const BoxTracks::ConstPtr& tracks);
  void syncCallback();

  void releaseModels();
  void createModels();
  void updateModels();
  void thresholdModels();
  void selectSimpleModels();
  void suppressUnarticulatedModels();
  void selectModels();
  void visualizeModels();
  void visualizeTracks();

  void publishArticulatedObjects();

  void removeOldLines();
};

}

int main(int argc, char** argv);


#endif /* ARTICULATED_OBJECTS_H_ */
