#include <gtest/gtest.h>
#include <tf/tf.h>
#include <math_utils/angles.h>
#include <sys/time.h>

#include "LinearMath/btVector3.h"


void seed_rand()
{
  //Seed random number generator with current microseond count
  timeval temp_time_struct;
  gettimeofday(&temp_time_struct,NULL);
  srand(temp_time_struct.tv_usec);
};

void generate_rand_vectors(double scale, unsigned int runs, std::vector<double>& xvalues, std::vector<double>& yvalues, std::vector<double>&zvalues)
{
  seed_rand();
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    xvalues[i] = 1.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    yvalues[i] = 1.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    zvalues[i] = 1.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
  }
}


using namespace tf;


TEST(tf, ListOneForward)
{
  unsigned int runs = 400;
  double epsilon = 1e-6;
  seed_rand();
  
  tf::Transformer mTR(true);
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    xvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    yvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    zvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;

    Stamped<btTransform> tranStamped(btTransform(btQuaternion(0,0,0), btVector3(xvalues[i],yvalues[i],zvalues[i])), 10 + i, "child",  "my_parent");
    mTR.setTransform(tranStamped);
  }

  std::cout << mTR.allFramesAsString() << std::endl;
  //  std::cout << mTR.chainAsString("child", 0, "my_parent2", 0, "my_parent2") << std::endl;

  for ( unsigned int i = 0; i < runs ; i++ )

  {
    Stamped<btTransform> inpose (btTransform(btQuaternion(0,0,0), btVector3(0,0,0)), 10 + i, "child");

    try{
    Stamped<Pose> outpose;
    outpose.data_.setIdentity(); //to make sure things are getting mutated
    mTR.transformPose("my_parent",inpose, outpose);
    EXPECT_NEAR(outpose.data_.getOrigin().x(), xvalues[i], epsilon);
    EXPECT_NEAR(outpose.data_.getOrigin().y(), yvalues[i], epsilon);
    EXPECT_NEAR(outpose.data_.getOrigin().z(), zvalues[i], epsilon);
    }
    catch (tf::TransformException & ex)
    {
      std::cout << "TransformExcepion got through!!!!! " << ex.what() << std::endl;
      bool exception_improperly_thrown = true;
      EXPECT_FALSE(exception_improperly_thrown);
    }
  }
  
}


TEST(tf, ListOneInverse)
{
  unsigned int runs = 400;
  double epsilon = 1e-6;
  seed_rand();
  
  tf::Transformer mTR(true);
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    xvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    yvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    zvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;

    Stamped<btTransform> tranStamped(btTransform(btQuaternion(0,0,0), btVector3(xvalues[i],yvalues[i],zvalues[i])), 10 + i, "child",  "my_parent");
    mTR.setTransform(tranStamped);
  }

  std::cout << mTR.allFramesAsString() << std::endl;
  //  std::cout << mTR.chainAsString("child", 0, "my_parent2", 0, "my_parent2") << std::endl;

  for ( unsigned int i = 0; i < runs ; i++ )

  {
    Stamped<btTransform> inpose (btTransform(btQuaternion(0,0,0), btVector3(0,0,0)), 10 + i, "my_parent");

    try{
    Stamped<btTransform> outpose;
    outpose.data_.setIdentity(); //to make sure things are getting mutated
    mTR.transformPose("child",inpose, outpose);
    EXPECT_NEAR(outpose.data_.getOrigin().x(), -xvalues[i], epsilon);
    EXPECT_NEAR(outpose.data_.getOrigin().y(), -yvalues[i], epsilon);
    EXPECT_NEAR(outpose.data_.getOrigin().z(), -zvalues[i], epsilon);
    }
    catch (tf::TransformException & ex)
    {
      std::cout << "TransformExcepion got through!!!!! " << ex.what() << std::endl;
      bool exception_improperly_thrown = true;
      EXPECT_FALSE(exception_improperly_thrown);
    }
  }
  
}



TEST(tf, TransformTransformsCartesian)
{
  unsigned int runs = 400;
  double epsilon = 1e-6;
  seed_rand();
  
  tf::Transformer mTR(true);
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    xvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    yvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    zvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;

    Stamped<btTransform> tranStamped(btTransform(btQuaternion(0,0,0), btVector3(xvalues[i],yvalues[i],zvalues[i])), 10 + i, "child", "my_parent");
    mTR.setTransform(tranStamped);

    /// \todo remove fix for graphing problem
    Stamped<btTransform> tranStamped2(btTransform(btQuaternion(0,0,0), btVector3(0,0,0)), 10 + i, "my_parent", "my_parent2");
    mTR.setTransform(tranStamped2);
  }

  //std::cout << mTR.allFramesAsString() << std::endl;
  //  std::cout << mTR.chainAsString("child", 0, "my_parent2", 0, "my_parent2") << std::endl;

  for ( unsigned int i = 0; i < runs ; i++ )

  {
    Stamped<btTransform> inpose (btTransform(btQuaternion(0,0,0), btVector3(0,0,0)), 10 + i, "child");

    try{
    Stamped<Pose
> outpose;
    outpose.data_.setIdentity(); //to make sure things are getting mutated
    mTR.transformPose("my_parent",inpose, outpose);
    EXPECT_NEAR(outpose.data_.getOrigin().x(), xvalues[i], epsilon);
    EXPECT_NEAR(outpose.data_.getOrigin().y(), yvalues[i], epsilon);
    EXPECT_NEAR(outpose.data_.getOrigin().z(), zvalues[i], epsilon);
    }
    catch (tf::TransformException & ex)
    {
      std::cout << "TransformExcepion got through!!!!! " << ex.what() << std::endl;
      bool exception_improperly_thrown = true;
      EXPECT_FALSE(exception_improperly_thrown);
    }
  }
  
  Stamped<Pose> inpose (btTransform(btQuaternion(0,0,0), btVector3(0,0,0)), runs, "child");
  Stamped<Pose> outpose;
  outpose.data_.setIdentity(); //to make sure things are getting mutated
  mTR.transformPose("child",inpose, outpose);
  EXPECT_NEAR(outpose.data_.getOrigin().x(), 0, epsilon);
  EXPECT_NEAR(outpose.data_.getOrigin().y(), 0, epsilon);
  EXPECT_NEAR(outpose.data_.getOrigin().z(), 0, epsilon);
  
  
}

TEST(tf, TransformPointCartesian)
{
  unsigned int runs = 400;
  double epsilon = 1e-6;
  seed_rand();
  
  tf::Transformer mTR(true);
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    xvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    yvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    zvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;

    Stamped<btTransform> tranStamped(btTransform(btQuaternion(0,0,0), btVector3(xvalues[i],yvalues[i],zvalues[i])), 10 + i, "child", "my_parent");
    mTR.setTransform(tranStamped);

    /// \todo remove fix for graphing problem
    Stamped<btTransform> tranStamped2(btTransform(btQuaternion(0,0,0), btVector3(0,0,0)), 10 + i, "my_parent", "my_parent2");
    mTR.setTransform(tranStamped2);
  }

  //  std::cout << mTR.allFramesAsString() << std::endl;
  //  std::cout << mTR.chainAsString("child", 0, "my_parent2", 0, "my_parent2") << std::endl;

  for ( unsigned int i = 0; i < runs ; i++ )

  {
    double x =10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    double y =10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    double z =10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    Stamped<Point> invec (btVector3(x,y,z), 10 + i, "child");

    try{
    Stamped<Point> outvec(btVector3(0,0,0), 10 + i, "child");
    //    outpose.data_.setIdentity(); //to make sure things are getting mutated
    mTR.transformPoint("my_parent",invec, outvec);
    EXPECT_NEAR(outvec.data_.x(), xvalues[i]+x, epsilon);
    EXPECT_NEAR(outvec.data_.y(), yvalues[i]+y, epsilon);
    EXPECT_NEAR(outvec.data_.z(), zvalues[i]+z, epsilon);
    }
    catch (tf::TransformException & ex)
    {
      std::cout << "TransformExcepion got through!!!!! " << ex.what() << std::endl;
      bool exception_improperly_thrown = true;
      EXPECT_FALSE(exception_improperly_thrown);
    }
  }
  
}

TEST(tf, TransformVectorCartesian)
{
  unsigned int runs = 400;
  double epsilon = 1e-6;
  seed_rand();
  
  tf::Transformer mTR(true);
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    xvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    yvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    zvalues[i] = 10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;

    Stamped<btTransform> tranStamped(btTransform(btQuaternion(0,0,0), btVector3(xvalues[i],yvalues[i],zvalues[i])), 10 + i, "child", "my_parent");
    mTR.setTransform(tranStamped);

    /// \todo remove fix for graphing problem
    Stamped<btTransform> tranStamped2(btTransform(btQuaternion(0,0,0), btVector3(0,0,0)), 10 + i, "my_parent", "my_parent2");
    mTR.setTransform(tranStamped2);
  }

  //  std::cout << mTR.allFramesAsString() << std::endl;
  //  std::cout << mTR.chainAsString("child", 0, "my_parent2", 0, "my_parent2") << std::endl;

  for ( unsigned int i = 0; i < runs ; i++ )

  {
    double x =10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    double y =10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    double z =10.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    Stamped<Point> invec (btVector3(x,y,z), 10 + i, "child");

    try{
    Stamped<Vector3> outvec(btVector3(0,0,0), 10 + i, "child");
    //    outpose.data_.setIdentity(); //to make sure things are getting mutated
    mTR.transformVector("my_parent",invec, outvec);
    EXPECT_NEAR(outvec.data_.x(), x, epsilon);
    EXPECT_NEAR(outvec.data_.y(), y, epsilon);
    EXPECT_NEAR(outvec.data_.z(), z, epsilon);
    }
    catch (tf::TransformException & ex)
    {
      std::cout << "TransformExcepion got through!!!!! " << ex.what() << std::endl;
      bool exception_improperly_thrown = true;
      EXPECT_FALSE(exception_improperly_thrown);
    }
  }
  
}

TEST(tf, TransformQuaternionCartesian)
{
  unsigned int runs = 400;
  double epsilon = 1e-6;
  seed_rand();
  
  tf::Transformer mTR(true);
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    xvalues[i] = 1.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    yvalues[i] = 1.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;
    zvalues[i] = 1.0 * ((double) rand() - (double)RAND_MAX /2.0) /(double)RAND_MAX;


    Stamped<btTransform> tranStamped(btTransform(btQuaternion(0,0,0), btVector3(xvalues[i],yvalues[i],zvalues[i])), 10 + i, "child", "my_parent");
    mTR.setTransform(tranStamped);

    /// \todo remove fix for graphing problem
    Stamped<btTransform> tranStamped2(btTransform(btQuaternion(0,0,0), btVector3(0,0,0)), 10 + i, "my_parent", "my_parent2");
    mTR.setTransform(tranStamped2);
  }

  //  std::cout << mTR.allFramesAsString() << std::endl;
  //  std::cout << mTR.chainAsString("child", 0, "my_parent2", 0, "my_parent2") << std::endl;

  for ( unsigned int i = 0; i < runs ; i++ )

  {
    Stamped<btQuaternion> invec (btQuaternion(xvalues[i],yvalues[i],zvalues[i]), 10 + i, "child");
    //    printf("%f, %f, %f\n", xvalues[i],yvalues[i], zvalues[i]);

    try{
    Stamped<btQuaternion> outvec(btQuaternion(xvalues[i],yvalues[i],zvalues[i]), 10 + i, "child");

    mTR.transformQuaternion("my_parent",invec, outvec);
    EXPECT_NEAR(outvec.data_.angle(invec.data_) , 0, epsilon);
    }
    catch (tf::TransformException & ex)
    {
      std::cout << "TransformExcepion got through!!!!! " << ex.what() << std::endl;
      bool exception_improperly_thrown = true;
      EXPECT_FALSE(exception_improperly_thrown);
    }
  }
  
}

TEST(data, Vector3Conversions)
{
  
  unsigned int runs = 400;
  double epsilon = 1e-6;
  
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  generate_rand_vectors(1.0, runs, xvalues, yvalues,zvalues);
  
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    btVector3 btv = btVector3(xvalues[i], yvalues[i], zvalues[i]);
    btVector3 btv_out = btVector3(0,0,0);
    std_msgs::Vector3 msgv;
    Vector3TFToMsg(btv, msgv);
    Vector3MsgToTF(msgv, btv_out);
    EXPECT_NEAR(btv.x(), btv_out.x(), epsilon);
    EXPECT_NEAR(btv.y(), btv_out.y(), epsilon);
    EXPECT_NEAR(btv.z(), btv_out.z(), epsilon);
  } 
  
}

TEST(data, Vector3StampedConversions)
{
  
  unsigned int runs = 400;
  double epsilon = 1e-6;
  
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  generate_rand_vectors(1.0, runs, xvalues, yvalues,zvalues);
  
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    Stamped<btVector3> btv = Stamped<btVector3>(btVector3(xvalues[i], yvalues[i], zvalues[i]), 1000000000ULL, "no frame");
    Stamped<btVector3> btv_out;
    std_msgs::Vector3Stamped msgv;
    Vector3StampedTFToMsg(btv, msgv);
    Vector3StampedMsgToTF(msgv, btv_out);
    EXPECT_NEAR(btv.data_.x(), btv_out.data_.x(), epsilon);
    EXPECT_NEAR(btv.data_.y(), btv_out.data_.y(), epsilon);
    EXPECT_NEAR(btv.data_.z(), btv_out.data_.z(), epsilon);
    EXPECT_STREQ(btv.frame_id_.c_str(), btv_out.frame_id_.c_str());
    EXPECT_EQ(btv.stamp_, btv_out.stamp_);
  } 
}

TEST(data, QuaternionConversions)
{
  
  unsigned int runs = 400;
  double epsilon = 1e-6;
  
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  generate_rand_vectors(1.0, runs, xvalues, yvalues,zvalues);
  
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    btQuaternion btv = btQuaternion(xvalues[i], yvalues[i], zvalues[i]);
    btQuaternion btv_out = btQuaternion(0,0,0);
    std_msgs::Quaternion msgv;
    QuaternionTFToMsg(btv, msgv);
    QuaternionMsgToTF(msgv, btv_out);
    EXPECT_NEAR(btv.x(), btv_out.x(), epsilon);
    EXPECT_NEAR(btv.y(), btv_out.y(), epsilon);
    EXPECT_NEAR(btv.z(), btv_out.z(), epsilon);
    EXPECT_NEAR(btv.w(), btv_out.w(), epsilon);
  } 
  
}

TEST(data, QuaternionStampedConversions)
{
  
  unsigned int runs = 400;
  double epsilon = 1e-6;
  
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  generate_rand_vectors(1.0, runs, xvalues, yvalues,zvalues);
  
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    Stamped<btQuaternion> btv = Stamped<btQuaternion>(btQuaternion(xvalues[i], yvalues[i], zvalues[i]), 1000000000ULL, "no frame");
    Stamped<btQuaternion> btv_out;
    std_msgs::QuaternionStamped msgv;
    QuaternionStampedTFToMsg(btv, msgv);
    QuaternionStampedMsgToTF(msgv, btv_out);
    EXPECT_NEAR(btv.data_.x(), btv_out.data_.x(), epsilon);
    EXPECT_NEAR(btv.data_.y(), btv_out.data_.y(), epsilon);
    EXPECT_NEAR(btv.data_.z(), btv_out.data_.z(), epsilon);
    EXPECT_NEAR(btv.data_.w(), btv_out.data_.w(), epsilon);
    EXPECT_STREQ(btv.frame_id_.c_str(), btv_out.frame_id_.c_str());
    EXPECT_EQ(btv.stamp_, btv_out.stamp_);
  } 
}

TEST(data, TransformConversions)
{
  
  unsigned int runs = 400;
  double epsilon = 1e-6;
  
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  generate_rand_vectors(1.0, runs, xvalues, yvalues,zvalues);
  std::vector<double> xvalues2(runs), yvalues2(runs), zvalues2(runs);
  generate_rand_vectors(1.0, runs, xvalues, yvalues,zvalues);
  
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    btTransform btv = btTransform(btQuaternion(xvalues2[i], yvalues2[i], zvalues2[i]), btVector3(xvalues[i], yvalues[i], zvalues[i]));
    btTransform btv_out;
    std_msgs::Transform msgv;
    TransformTFToMsg(btv, msgv);
    TransformMsgToTF(msgv, btv_out);
    EXPECT_NEAR(btv.getOrigin().x(), btv_out.getOrigin().x(), epsilon);
    EXPECT_NEAR(btv.getOrigin().y(), btv_out.getOrigin().y(), epsilon);
    EXPECT_NEAR(btv.getOrigin().z(), btv_out.getOrigin().z(), epsilon);
    EXPECT_NEAR(btv.getRotation().x(), btv_out.getRotation().x(), epsilon);
    EXPECT_NEAR(btv.getRotation().y(), btv_out.getRotation().y(), epsilon);
    EXPECT_NEAR(btv.getRotation().z(), btv_out.getRotation().z(), epsilon);
    EXPECT_NEAR(btv.getRotation().w(), btv_out.getRotation().w(), epsilon);
  } 
  
}

TEST(data, TransformStampedConversions)
{
  
  unsigned int runs = 400;
  double epsilon = 1e-6;
  
  std::vector<double> xvalues(runs), yvalues(runs), zvalues(runs);
  generate_rand_vectors(1.0, runs, xvalues, yvalues,zvalues);
  std::vector<double> xvalues2(runs), yvalues2(runs), zvalues2(runs);
  generate_rand_vectors(1.0, runs, xvalues, yvalues,zvalues);
  
  for ( unsigned int i = 0; i < runs ; i++ )
  {
    Stamped<btTransform> btv = Stamped<btTransform>(btTransform(btQuaternion(xvalues2[i], yvalues2[i], zvalues2[i]), btVector3(xvalues[i], yvalues[i], zvalues[i])), 1000000000ULL, "no frame");
    Stamped<btTransform> btv_out;
    std_msgs::TransformStamped msgv;
    TransformStampedTFToMsg(btv, msgv);
    TransformStampedMsgToTF(msgv, btv_out);
    EXPECT_NEAR(btv.data_.getOrigin().x(), btv_out.data_.getOrigin().x(), epsilon);
    EXPECT_NEAR(btv.data_.getOrigin().y(), btv_out.data_.getOrigin().y(), epsilon);
    EXPECT_NEAR(btv.data_.getOrigin().z(), btv_out.data_.getOrigin().z(), epsilon);
    EXPECT_NEAR(btv.data_.getRotation().x(), btv_out.data_.getRotation().x(), epsilon);
    EXPECT_NEAR(btv.data_.getRotation().y(), btv_out.data_.getRotation().y(), epsilon);
    EXPECT_NEAR(btv.data_.getRotation().z(), btv_out.data_.getRotation().z(), epsilon);
    EXPECT_NEAR(btv.data_.getRotation().w(), btv_out.data_.getRotation().w(), epsilon);
    EXPECT_STREQ(btv.frame_id_.c_str(), btv_out.frame_id_.c_str());
    EXPECT_EQ(btv.stamp_, btv_out.stamp_);
  } 
}


/** @todo Make this actually Assert something */

int main(int argc, char **argv){
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
