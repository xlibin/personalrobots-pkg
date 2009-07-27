/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage
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

#include <stdlib.h>

#include <set>
#include <map>
#include <vector>

#include <point_cloud_clustering/generic_clustering.h>

#include <robot_msgs/PointCloud.h>

#include <opencv/cxcore.h>
#include <opencv/cv.h>
#include <opencv/cvaux.hpp>

using namespace std;

namespace point_cloud_clustering
{
  typedef struct kmeans_params
  {
      double factor;
      double accuracy;
      int max_iter;
      vector<unsigned int> channel_indices;
  } kmeans_params_t;

  int pcKMeans(const robot_msgs::PointCloud& pt_cloud,
               const set<unsigned int>& ignore_indices,
               const kmeans_params_t& kmeans_params,
               map<unsigned int, vector<float> >& cluster_xyz_centroids,
               map<unsigned int, vector<int> >& cluster_pt_indices);

  class KMeans: public GenericClustering
  {
    public:
      KMeans()
      {
      }

      void setParameters();

      virtual int cluster(const robot_msgs::PointCloud& pt_cloud,
                          cloud_kdtree::KdTree& pt_cloud_kdtree,
                          const set<unsigned int>& indices_to_cluster,
                          map<unsigned int, vector<int> >& created_clusters)
      {
        return 0;
      }
  };
}
