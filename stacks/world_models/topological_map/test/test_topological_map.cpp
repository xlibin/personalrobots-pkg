/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "topological_map/topological_map.h"
#include "topological_map/exception.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <gtest/gtest.h>

using namespace topological_map;
using namespace std;

using std::cout;
using std::endl;
using boost::extents;
using boost::shared_ptr;
using boost::tie;
using std::stringstream;
using topological_map::OccupancyGrid;
using topological_map::TopologicalMapPtr;
using topological_map::topologicalMapFromGrid;


// Helpers

bool isRearrangement (RegionIdVector v, RegionId* a, unsigned int k)
{
  if (v.size()!=k) {
    return false;
  }

  for (unsigned int i=0; i<k; i++) {
    if (find(v.begin(), v.end(), a[i])==v.end()) {
      return false;
    }
  }
  return true;
}



// Tests


TEST(TopologicalMap, BasicAPI)
{
  OccupancyGrid grid(boost::extents[100][100]);
  TopologicalMap m(grid);
  unsigned int s=m.allRegions().size();
  int r, c;

  EXPECT_EQ (0u, s);
  
  MutableRegionPtr r1(new Region);
  for (r=2; r<4; r++) {
    for (c=9; c<13; c++) {
      r1->insert(Cell2D(r,c));
    }
  }
  unsigned int id=m.addRegion(r1, 3);
  EXPECT_EQ(1u, id);
  EXPECT_EQ(1u, m.allRegions().size());

  MutableRegionPtr r2(new Region);
  
  for (r=4; r<8; r++) {
    for (c=9; c<12; c++) {
      r2->insert(Cell2D(r,c));
    }
  }
  
  EXPECT_EQ(2u, m.addRegion(r2, 4));
  EXPECT_EQ(2u, m.allRegions().size());

  RegionIdVector n1=m.neighbors(1);
  RegionIdVector n2=m.neighbors(2);
  RegionId en1[1] = {2};
  RegionId en2[1] = {1};
  EXPECT_TRUE(isRearrangement(n1, en1, 1u));
  EXPECT_TRUE(isRearrangement(n2, en2, 1u));

  MutableRegionPtr r4(new Region);
  for (r=-10; r<10; r++) {
    for (c=0; c<20; c++) {
      r4->insert(Cell2D(r,c));
    }
  }

  try {
    m.addRegion(r4, 2);
    ADD_FAILURE() << "Expected exception didn't happen";
  }
  catch (topological_map::OverlappingRegionException& e) {}

  MutableRegionPtr r3(new Region);
  r3->insert(Cell2D(3,13));
  r3->insert(Cell2D(4,13));
  EXPECT_EQ(3u, m.addRegion(r3, 3));
  EXPECT_EQ(3u, m.allRegions().size());

  n1=m.neighbors(1);
  n2=m.neighbors(2);
  RegionIdVector n3=m.neighbors(3);
  RegionId en11[2]={3,2};
  RegionId en3[1]={1};
  EXPECT_TRUE(isRearrangement(n1, en11, 2u));
  EXPECT_TRUE(isRearrangement(n2, en2, 1u));
  EXPECT_TRUE(isRearrangement(n3, en3, 1u));
  

  EXPECT_EQ(1u, m.containingRegion(Cell2D(3,10)));
  EXPECT_EQ(3u, m.containingRegion(Cell2D(4,13)));
  EXPECT_EQ(2u, m.containingRegion(Cell2D(4,9)));
  m.removeRegion(2);
  try {
    m.removeRegion(2);
    ADD_FAILURE() << "Expected UnknownRegionException didn't happen";
  }
  catch (topological_map::UnknownRegionException& e) {}

  EXPECT_EQ(2u, m.allRegions().size());
  n1=m.neighbors(1);
  n3=m.neighbors(3);

  RegionId en12[1]={3};
  EXPECT_TRUE(isRearrangement(n1, en12, 1));
  EXPECT_TRUE(isRearrangement(n3, en3, 1));

  MutableRegionPtr r5(new Region);
  for (c=0; c<20; c++) {
    r5->insert(Cell2D(5,c));
  }
  EXPECT_EQ(4u, m.addRegion(r5,1));
  n1=m.neighbors(1);
  n3=m.neighbors(3);
  RegionIdVector n5=m.neighbors(4);
  RegionId en32[2]={1,4};
  RegionId en5[1]={3};
  EXPECT_TRUE(isRearrangement(n1,en12,1));
  EXPECT_TRUE(isRearrangement(n3,en32,2));
  EXPECT_TRUE(isRearrangement(n5,en5,1));
  EXPECT_EQ(3u, m.allRegions().size());

  stringstream ss;
  
  m.writeToStream(ss);

  TopologicalMap m2(ss);
  n1=m2.neighbors(1);
  n3=m2.neighbors(3);
  n5=m2.neighbors(4);
  EXPECT_TRUE(isRearrangement(n1,en12,1));
  EXPECT_TRUE(isRearrangement(n3,en32,2));
  EXPECT_TRUE(isRearrangement(n5,en5,1));
  EXPECT_EQ(3u, m2.allRegions().size());


  // Outlets
  m.addOutlet(OutletInfo(1,2,3,4,5,6,7,4,"orange"));
  m.addOutlet(OutletInfo(10,20,30,40,50,60,70,1,"white"));
  EXPECT_EQ(m.outletInfo(1).z, 3);
  EXPECT_EQ(m.outletInfo(2).sockets_color, "white");
  EXPECT_EQ(m.nearestOutlet(Point2D(3,4)), 1);
  EXPECT_EQ(m.nearestOutlet(Point2D(15,16)), 2);
  EXPECT_EQ(m.outletInfo(1).blocked, false);
  EXPECT_EQ(m.outletInfo(2).blocked, false);
  m.observeOutletBlocked(2);
  EXPECT_EQ(m.outletInfo(1).blocked, false);
  EXPECT_EQ(m.outletInfo(2).blocked, true);
}


void setV (topological_map::OccupancyGrid& grid, unsigned r0, unsigned dr, unsigned rmax, unsigned c0, unsigned dc, unsigned cmax, bool val) 
{
  for (unsigned r=r0; r<rmax; r+=dr) {
    for (unsigned c=c0; c<cmax; c+=dc) {
      grid[r][c] = val;
    }
  }
}


TEST(TopologicalMap, Creation)
{
  OccupancyGrid grid(extents[21][24]);
  setV(grid, 0, 1, 21, 0, 1, 24, false);
  setV(grid, 7, 7, 21, 0, 1, 24, true);
  setV(grid, 0, 1, 21, 8, 8, 24, true);
  setV(grid, 3, 7, 21, 8, 8, 24, false);
  setV(grid, 7, 7, 21, 4, 8, 24, false);
  grid[18][12] = true;
  grid[0][9] = true;
  

  TopologicalMapPtr m = topologicalMapFromGrid (grid, 0.1, 2, 1, 1, 0, "local");

  EXPECT_EQ(m->allRegions().size(), 45u);
  EXPECT_EQ(m->regionType(m->containingRegion(Cell2D(1,1))), 0);
  EXPECT_EQ(m->regionType(m->containingRegion(Point2D(.35,.82))), 1);
  EXPECT_TRUE(m->isObstacle(Point2D(2.35,.75)));
  EXPECT_TRUE(!(m->isObstacle(Point2D(2.35,.65))));

  RegionId region = m->containingRegion(Cell2D(1u,1u));
  EXPECT_EQ (region, m->containingRegion(Cell2D(2u,2u)));
  EXPECT_EQ (m->containingRegion(Cell2D(0, 8)), m->containingRegion(Cell2D(0,7)));
  region = m->containingRegion(Cell2D(18,12));
  EXPECT_EQ(region, m->containingRegion(Cell2D(17,12)));
             

  Point2D pos=m->connectorPosition(1);
  bool path_found;
  double d;
  tie(path_found,d) = m->distanceBetween(pos,Point2D(.35, .82));
  EXPECT_TRUE(path_found);
  double d2;
  tie(path_found, d2) = m->distanceBetween(pos,Point2D(1.1,.2));
  EXPECT_TRUE(path_found);
  d+=d2;

  EXPECT_TRUE ((d>=1.2)&&(d<=3));

  // Test serialization/deserialization
  stringstream ss;
  m->writeToStream(ss);

  TopologicalMap m2(ss);
  double d3, d4;
  tie(path_found,d3) = m2.distanceBetween(Point2D(.35,.82), pos);
  EXPECT_TRUE(path_found);

  tie(path_found, d4) = m2.distanceBetween(Point2D(1.1,.2), pos);
  EXPECT_TRUE(path_found);
  EXPECT_TRUE(abs(d-d3-d4)<.01);
  EXPECT_TRUE(abs(d2-d4)<.01);

  typedef pair<ConnectorId, double> ConnectorCost;
  vector<ConnectorCost> costs=m2.connectorCosts(Point2D(.1,.1), Point2D(1,1));
  EXPECT_TRUE(costs.size()>=2);
  EXPECT_TRUE(costs.size()<=4);
  EXPECT_TRUE(costs[0].second > 1.5);
  EXPECT_TRUE(costs[1].second > 1.5);
  EXPECT_TRUE(costs[0].second < 3);
  EXPECT_TRUE(costs[1].second < 3);
  ConnectorIdVector path = m2.shortestConnectorPath(Point2D(.1,.1), Point2D(1,1));
  EXPECT_TRUE(path.size()>=2);
  EXPECT_TRUE(path.size()<=100);
}

int main (int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
