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

#include <gtest/gtest.h>
#include <diagnostic_updater/diagnostic_updater.h>
#include <diagnostic_updater/update_functions.h>
#include <unistd.h>

using namespace diagnostic_updater;

class TestClass 
{
public: 
  void unwrapped(diagnostic_msgs::DiagnosticStatus &s)
  {
  }

  void wrapped(DiagnosticStatusWrapper &s)
  {
  }
};
                                   
TEST(DiagnosticUpdater, testDiagnosticUpdater)
{
  class classFunction : public DiagnosticTask
  {
  public:
    classFunction() : DiagnosticTask("classFunction")
    {}

    void run(DiagnosticStatusWrapper &s) 
    {
      s.summary(0, "Test is running");
      s.addf("Value", "%f", 5);
      s.add("String", "Toto");
      s.add("Floating", 5.55);
      s.add("Integer", 5);
      s.addf("Formatted %s %i", "Hello", 5);
    }
  };
  
  TestClass c;
  ros::NodeHandle nh;
  
  // Note: Some of this code does nothing in terms of testing, but ensures
  // that all the constructors compile.
  DiagnosticUpdater<TestClass> dummy1(&c);
  DiagnosticUpdater<TestClass> dummy2(&c, *nh.getNode());
  DiagnosticUpdater<TestClass> updater(&c, nh);
  
  updater.addUpdater(&TestClass::unwrapped);
  updater.Updater::add("wrapped", &c, &TestClass::wrapped);
  
  classFunction cf;
  updater.Updater::add(cf);
}

TEST(DiagnosticUpdater, testDiagnosticStatusWrapper)
{
  DiagnosticStatusWrapper stat;
  
  const char *message = "dummy";
  int level = 1;
  stat.summary(level, message);
  EXPECT_STREQ(message, stat.message.c_str()) << "DiagnosticStatusWrapper::summary failed to set message";
  EXPECT_EQ(level, stat.level) << "DiagnosticStatusWrapper::summary failed to set level";

  stat.addf("toto", "%.1f", 5.0);
  stat.add("baba", 5);
  stat.addf("foo", "%05i", 27);
  
  EXPECT_STREQ("5.0", stat.values[0].value.c_str()) << "Bad value, adding a value with addf";
  EXPECT_STREQ("5", stat.values[1].value.c_str()) << "Bad value, adding a string with add";
  EXPECT_STREQ("00027", stat.values[2].value.c_str()) << "Bad value, adding a string with addf";
  EXPECT_STREQ("toto", stat.values[0].key.c_str()) << "Bad label, adding a value with add";
  EXPECT_STREQ("baba", stat.values[1].key.c_str()) << "Bad label, adding a string with add";
  EXPECT_STREQ("foo", stat.values[2].key.c_str()) << "Bad label, adding a string with addf";
}

TEST(DiagnosticUpdater, testFrequencyStatus)
{
  double minFreq = 10;
  double maxFreq = 20;
  
  FrequencyStatus fs(FrequencyStatusParam(&minFreq, &maxFreq, 0.5, 2));
  
  DiagnosticStatusWrapper stat[5];
  fs.tick();
  usleep(20000);
  fs.run(stat[0]); // Should be too fast, 20 ms for 1 tick, lower limit should be 33ms.
  usleep(50000);
  fs.tick();
  fs.run(stat[1]); // Should be good, 70 ms for 2 ticks, lower limit should be 66 ms.
  usleep(300000);
  fs.tick();
  fs.run(stat[2]); // Should be good, 350 ms for 2 ticks, upper limit should be 400 ms.
  usleep(150000);
  fs.tick();
  fs.run(stat[3]); // Should be too slow, 450 ms for 2 ticks, upper limit should be 400 ms.
  fs.clear();
  fs.run(stat[4]); // Should be good, just cleared it.

  EXPECT_EQ(2, stat[0].level) << "max frequency exceeded but not reported";
  EXPECT_EQ(0, stat[1].level) << "within max frequency but reported error";
  EXPECT_EQ(0, stat[2].level) << "within min frequency but reported error";
  EXPECT_EQ(2, stat[3].level) << "min frequency exceeded but not reported";
  EXPECT_EQ(2, stat[4].level) << "freshly cleared should fail";
  EXPECT_STREQ("", stat[0].name.c_str()) << "Name should not be set by FrequencyStatus";
  EXPECT_STREQ("Frequency Status", fs.getName().c_str()) << "Name should be \"Frequency Status\"";
}

TEST(DiagnosticUpdater, testTimeStampStatus)
{
  TimeStampStatus ts(DefaultTimeStampStatusParam);

  DiagnosticStatusWrapper stat[5];
  ts.run(stat[0]);
  ts.tick(ros::Time::now().toSec() + 2);
  ts.run(stat[1]);
  ts.tick(ros::Time::now());
  ts.run(stat[2]);
  ts.tick(ros::Time::now().toSec() - 4);
  ts.run(stat[3]);
  ts.tick(ros::Time::now().toSec() - 6);
  ts.run(stat[4]);
 
  EXPECT_EQ(1, stat[0].level) << "no data should return a warning";
  EXPECT_EQ(2, stat[1].level) << "too far future not reported";
  EXPECT_EQ(0, stat[2].level) << "now not accepted";
  EXPECT_EQ(0, stat[3].level) << "4 seconds ago not accepted";
  EXPECT_EQ(2, stat[4].level) << "too far past not reported";
  EXPECT_STREQ("", stat[0].name.c_str()) << "Name should not be set by TimeStapmStatus";
  EXPECT_STREQ("Timestamp Status", ts.getName().c_str()) << "Name should be \"Timestamp Status\"";
}

int main(int argc, char **argv){
  ros::init(argc, argv, "test_node");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

