/*
 * find_planes.cpp
 *
 *  Created on: Jul 7, 2009
 *      Author: sturm
 */

#include "find_planes.h"

using namespace ros;
using namespace std;
using namespace robot_msgs;

#include <point_cloud_mapping/sample_consensus/sac_model_plane.h>
#include <point_cloud_mapping/sample_consensus/sac.h>
#include <point_cloud_mapping/sample_consensus/ransac.h>
#include <point_cloud_mapping/geometry/angles.h>
#include <point_cloud_mapping/geometry/areas.h>


bool find_planes::fitSACPlanes(PointCloud *points, vector<int> &indices, vector<vector<int> > &inliers, vector<vector<
    double> > &coeff, const robot_msgs::Point32 &viewpoint_cloud, double dist_thresh, int n_max,
                               int min_points_per_model)
{
  // Create and initialize the SAC model
  sample_consensus::SACModelPlane *model = new sample_consensus::SACModelPlane();
  sample_consensus::SAC *sac = new sample_consensus::RANSAC(model, dist_thresh);
  sac->setMaxIterations(100);
  model->setDataSet(points, indices);

  int nr_models = 0, nr_points_left = indices.size();
  while (nr_models < n_max && nr_points_left > min_points_per_model)
  {
    // Search for the best plane
    if (sac->computeModel())
    {
      vector<double> model_coeff;
      sac->computeCoefficients(model_coeff); // Compute the model coefficients
      sac->refineCoefficients(model_coeff); // Refine them using least-squares
      coeff.push_back(model_coeff);

      // Get the list of inliers
      vector<int> model_inliers;
      model->selectWithinDistance(model_coeff, dist_thresh, model_inliers);
      inliers.push_back(model_inliers);

      // Flip the plane normal towards the viewpoint
      cloud_geometry::angles::flipNormalTowardsViewpoint(model_coeff, points->pts.at(model_inliers[0]), viewpoint_cloud);

      ROS_INFO ("Found a planar model supported by %d inliers: [%g, %g, %g, %g]", (int)model_inliers.size (),
          model_coeff[0], model_coeff[1], model_coeff[2], model_coeff[3]);

      // Remove the current inliers from the global list of points
      nr_points_left = sac->removeInliers();
      nr_models++;
    }
    else
    {
      ROS_ERROR ("Could not compute a planar model for %d points.", nr_points_left);
      break;
    }
  }

  delete sac;
  delete model;
  return (true);
}

void find_planes::filterByZBounds(const PointCloud& pc, double zmin, double zmax, PointCloud& filtered_pc,
                                  PointCloud& filtered_outside)
{
  vector<int> indices_remove;
  for (size_t i = 0; i < pc.get_pts_size(); ++i)
  {
    if (pc.pts[i].z > zmax || pc.pts[i].z < zmin)
    {
      indices_remove.push_back(i);
    }
  }
  cloud_geometry::getPointCloudOutside(pc, indices_remove, filtered_pc);
  cloud_geometry::getPointCloud(pc, indices_remove, filtered_outside);
}

void find_planes::getPointIndicesInZBounds(const PointCloud &points, double z_min, double z_max, vector<int> &indices)
{
  indices.resize(points.pts.size());
  int nr_p = 0;
  for (unsigned int i = 0; i < points.pts.size(); i++)
  {
    if ((points.pts[i].z >= z_min && points.pts[i].z <= z_max))
    {
      indices[nr_p] = i;
      nr_p++;
    }
  }
  indices.resize(nr_p);
}

void find_planes::segmentPlanes(const PointCloud &const_points, double z_min, double z_max, double support,
                                double min_area, int n_max, vector<vector<int> > &indices,
                                vector<vector<double> > &models, int number)
{
  PointCloud points = const_points;
  // This should be given as a parameter as well, or set global, etc
  double sac_distance_threshold_ = 0.02; // 2cm distance threshold for inliers (point-to-plane distance)

  vector<int> indices_in_bounds;
  // Get the point indices within z_min <-> z_max
  find_planes::getPointIndicesInZBounds(points, z_min, z_max, indices_in_bounds);
  ROS_INFO("segmentPlanes #%d running on %d/%d points",number,points.get_pts_size(),indices_in_bounds.size());

  // We need to know the viewpoint where the data was acquired
  // For simplicity, assuming 0,0,0 for stereo data in the stereo frame - however if this is not true, use TF to get
  //the point in a different frame !
  Point32 viewpoint;
  viewpoint.x = viewpoint.y = viewpoint.z = 0;

  // Use the entire data to estimate the plane equation.
  // NOTE: if this is slow, we can downsample first, then fit (check mapping/point_cloud_mapping/src/planar_fit.cpp)
  indices.clear(); //Points that are in plane
  models.clear(); //Plane equations

  find_planes::fitSACPlanes(&points, indices_in_bounds, indices, models, viewpoint, sac_distance_threshold_, n_max, 100);

  // Check the list of planar areas found against the minimally imposed area
  for (unsigned int i = 0; i < models.size(); i++)
  {
    // Compute the convex hull of the area
    // NOTE: this is faster than computing the concave (alpha) hull, so let's see how this works out
    Polygon3D polygon;
    cloud_geometry::areas::convexHull2D(points, indices[i], models[i], polygon);

    // Compute the area of the polygon
    double area = cloud_geometry::areas::compute2DPolygonalArea(polygon, models[i]);

    //     If the area is smaller, reset this planar model
    if (area < min_area)
    {
      models[i].resize(0);
      indices[i].resize(0);
      continue;
    }
  }
}

void find_planes::findPlanes(const PointCloud& cloud, int n_planes_max,
                             std::vector<std::vector<int> >& indices,vector<PointCloud>& plane_cloud, vector<vector<
    double> >& plane_coeff, PointCloud& outside)
{

  double z_min = 0.3;
  double z_max = 3.0;
  double support = 0.1;
  double min_area = 0.00;

  find_planes::segmentPlanes(cloud, z_min, z_max, support, min_area, n_planes_max, indices, plane_coeff, 1);

  outside = cloud;
  plane_cloud.resize(indices.size());
  for (size_t i = 0; i < indices.size(); i++)
  {
    cloud_geometry::getPointCloud(cloud, indices[i], plane_cloud[i]);
    PointCloud current = outside;
    cloud_geometry::getPointCloudOutside(current, indices[i], outside);
  }
}
