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

#include <gtest/gtest.h>
#include <costmap_2d/costmap_2d.h>
#include <mpglue/costmapper.h>

using namespace std;
using namespace boost;


static shared_ptr<costmap_2d::Costmap2D> createCostmap2D()
{
  static double const bbx0_(-1);
  static double const bby0_(-1);
  static double const bbx1_(1);
  static double const bby1_(1);
  static double const costmap_resolution(0.1);
  static double const costmap_inscribed_radius(0.15);
  static double const costmap_circumscribed_radius(0.25);
  static double const costmap_inflation_radius(0.35);
  double const origin_x(bbx0_);
  double const origin_y(bby0_);
  unsigned int const
    cells_size_x(1+static_cast<unsigned int>(ceil((bbx1_ - bbx0_) / costmap_resolution)));
  unsigned int const
    cells_size_y(1+static_cast<unsigned int>(ceil((bby1_ - bby0_) / costmap_resolution)));
  double const obstacle_range(8); // whatever
  double const max_obstacle_height(1); // whatever
  double const raytrace_range(8);	   // whatever
  double const weight(1); // whatever: documentation says 0<w<=1 but default is 25???
  vector<unsigned char> static_data(cells_size_x * cells_size_y);
  unsigned char const lethal_threshold(1);
  
  shared_ptr<costmap_2d::Costmap2D> cm2d(new costmap_2d::Costmap2D(cells_size_x,
								   cells_size_y,
								   costmap_resolution,
								   origin_x,
								   origin_y,
								   costmap_inscribed_radius,
								   costmap_circumscribed_radius,
								   costmap_inflation_radius,
								   obstacle_range,
								   max_obstacle_height,
								   raytrace_range,
								   weight,
								   static_data,
								   lethal_threshold));
  return cm2d;
}


TEST (costmapper, creation)
{
  static double const origin_x(-2);
  static double const origin_y(3);
  static double const resolution(0.05);
  static double const inscribed_radius(0.2);
  static double const circumscribed_radius(0.25);
  static double const inflation_radius(0.3);
  static mpglue::index_t const ix_end(100);
  static mpglue::index_t const iy_end(75);
  {
    mpglue::costmapper_params const pp(origin_x,
				       origin_y,
				       resolution,
				       inscribed_radius,
				       circumscribed_radius,
				       inflation_radius,
				       ix_end,
				       iy_end);
    shared_ptr<mpglue::Costmapper> cm(createCostmapper(&pp));
    EXPECT_TRUE (cm) << "mpglue::createCostmap2D(mpglue::costmapper_params const &) failed";
  }
  {
    static double const origin_th(-0.032);
    static mpglue::index_t const ix_begin(-10);
    static mpglue::index_t const iy_begin(32);
    static int const obstacle_cost(87);
    shared_ptr<mpglue::Costmapper>
      cm(createCostmapper(mpglue::sfl_costmapper_params(origin_x,
							origin_y,
							origin_th,
							resolution,
							inscribed_radius,
							circumscribed_radius,
							inflation_radius,
							ix_begin,
							iy_begin,
							ix_end,
							iy_end,
							obstacle_cost)));
    EXPECT_TRUE (cm) << "mpglue::createCostmap2D(mpglue::sfl_costmapper_params const &) failed";
  }
  {
    static double const obstacle_range(7);
    static double const max_obstacle_height(3.2);
    static double const raytrace_range(5.7);
    static double const weight(4.9);
    shared_ptr<mpglue::Costmapper>
      cm(createCostmapper(mpglue::cm2d_costmapper_params(origin_x,
							 origin_y,
							 resolution,
							 inscribed_radius,
							 circumscribed_radius,
							 inflation_radius,
							 ix_end,
							 iy_end,
							 obstacle_range,
							 max_obstacle_height,
							 raytrace_range,
							 weight)));
    EXPECT_TRUE (cm) << "mpglue::createCostmap2D(mpglue::cm2d_costmapper_params const &) failed";
  }
  {
    shared_ptr<costmap_2d::Costmap2D> cm2d(createCostmap2D());
    EXPECT_TRUE (cm2d) << "in-test createCostmap2D() failed";
    shared_ptr<mpglue::Costmapper> cm(mpglue::createCostmapper(cm2d));
    EXPECT_TRUE (cm) << "mpglue::createCostmapper() failed on in-test createCostmap2D()";
  }
}


TEST (costmapper, addition)
{
  shared_ptr<costmap_2d::Costmap2D> cm2d(createCostmap2D());
  ASSERT_TRUE (cm2d);
  shared_ptr<mpglue::Costmapper> cm(mpglue::createCostmapper(cm2d));
  ASSERT_TRUE (cm);
  mpglue::index_collection_t added_obstacle_indices;
  added_obstacle_indices.insert(mpglue::index_pair(2, 3));
  EXPECT_EQ (added_obstacle_indices.size(), size_t(1));
  size_t const count(cm->updateObstacles(&added_obstacle_indices, 0, &cerr));
  EXPECT_GT (count, size_t(0));
}


int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}