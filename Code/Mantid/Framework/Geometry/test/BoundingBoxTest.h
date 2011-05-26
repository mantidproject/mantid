#ifndef BOUNDINGBOXTEST_H_
#define BOUNDINGBOXTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Timer.h"
#include <iostream>

using namespace Mantid;
using namespace Mantid::Geometry;

/** 
* BoundingBox Unit test
*/
class BoundingBoxTest : public CxxTest::TestSuite
{
public:
  
  void test_That_Construction_With_Six_Valid_Points_Gives_A_BoundingBox()
  {
    BoundingBox * bbox = NULL;
    TS_ASSERT_THROWS_NOTHING(bbox = new BoundingBox(1.0, 4.0, 5.0, 0.0, 2.0, 3.0));
    if( !bbox ) 
    {
      return;
    }

    TS_ASSERT_EQUALS(bbox->xMin(), 0.0);
    TS_ASSERT_EQUALS(bbox->xMax(), 1.0);
    TS_ASSERT_EQUALS(bbox->yMin(), 2.0);
    TS_ASSERT_EQUALS(bbox->yMax(), 4.0);
    TS_ASSERT_EQUALS(bbox->zMin(), 3.0);
    TS_ASSERT_EQUALS(bbox->zMax(), 5.0);
    TS_ASSERT_EQUALS(bbox->isAxisAligned(),true);
    delete bbox;
  }

  void test_That_Construction_With_An_Invalid_Point_Does_Not_Create_A_Bounding_Box()
  {
    //xmax < xmin
    TS_ASSERT_THROWS(BoundingBox(1.0, 4.0, 5.0, 3.0, 2.0, 3.0), std::invalid_argument);
    //ymax < ymin
    TS_ASSERT_THROWS(BoundingBox(1.0, 0.5, 5.0, 3.0, 2.0, 3.0), std::invalid_argument);
    //zmax < zmin
    TS_ASSERT_THROWS(BoundingBox(1.0, 4.0, 5.0, 3.0, 2.0, 6.0), std::invalid_argument);
  }

  void test_That_Default_Construction_Gives_A_Null_Bounding_Box()
  {
    BoundingBox box;
    TS_ASSERT_EQUALS(box.isNull(), true);
    TS_ASSERT_EQUALS(box.isNonNull(), false);
    TS_ASSERT_EQUALS(box.isAxisAligned(),true);
  }

  void test_That_Construction_With_Points_Gives_A_NonNull_Bounding_Box()
  {
    BoundingBox box(3.0,4.0,5.0,0.0,1.0,2.0);
    TS_ASSERT_EQUALS(box.isNonNull(), true);
    TS_ASSERT_EQUALS(box.isNull(), false);
    TS_ASSERT_EQUALS(box.isAxisAligned(),true);
  }

  void test_That_Querying_For_The_Min_and_Max_Points_Gives_The_Correct_Points()
  {
    BoundingBox bbox(1.0, 2.0, 3.0, -1.0, -2.0, -3.0);
    TS_ASSERT_EQUALS(bbox.minPoint(), V3D(-1.0,-2.0,-3.0));
    TS_ASSERT_EQUALS(bbox.maxPoint(), V3D(1.0,2.0,3.0));
    TS_ASSERT_EQUALS(bbox.isAxisAligned(),true);
  }

  void test_That_Querying_A_Point_Inside_A_Valid_Bounding_Box_Returns_That_This_It_Is_Inside()
  {
    doPointTest(true);
  }

  void test_That_Querying_A_Point_Outside_A_Valid_Bounding_Box_Returns_That_This_It_Is_Not_Inside()
  {
    doPointTest(false);
  }

  void test_That_A_Line_Originating_Outside_The_Box_And_Fired_Towards_It_Intersects_The_Box()
  {
    BoundingBox bbox(4.1, 4.1, 4.1, -4.1, -4.1, -4.1);
    //X initial
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(-6.0,0.0,0.0), V3D(1.0,0.0,0.0)), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(6.0,0.0,0.0), V3D(-1.0,0.0,0.0)), true);
    // Y initial
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(0.0,-6.0,0.0), V3D(0.0,1.0,0.0)), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(0.0,6.0,0.0), V3D(0.0,-1.0,0.0)), true);
    //Z initial
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(0.0,0.0,-6.0), V3D(0.0,0.0,1.0)), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(0.0,0.0,6.0), V3D(0.0,0.0,-1.0)), true);

    // Mix
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(-5.0,-1.0,0.0),V3D(1.0,1.0,0.0)), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(-5.0,-1.0,-0.5),V3D(1.0,1.0,1.0)), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(V3D(10.0,10.0,0.0),V3D(-1.0,-0.4,0.0)), false);

  }

  void test_That_A_Track_Originating_Outside_The_Box_And_Fired_Towards_It_Intersects_The_Box()
  {
    BoundingBox bbox(4.1, 4.1, 4.1, -4.1, -4.1, -4.1);
    //X initial
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(-6.0,0.0,0.0), V3D(1.0,0.0,0.0))), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(6.0,0.0,0.0), V3D(-1.0,0.0,0.0))), true);
    // Y initial
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(0.0,-6.0,0.0), V3D(0.0,1.0,0.0))), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(0.0,6.0,0.0), V3D(0.0,-1.0,0.0))), true);
    //Z initial
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(0.0,0.0,-6.0), V3D(0.0,0.0,1.0))), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(0.0,0.0,6.0), V3D(0.0,0.0,-1.0))), true);

    // Mix
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(-5.0,-1.0,0.0),V3D(1.0,1.0,0.0))), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(-5.0,-1.0,-0.5),V3D(1.0,1.0,1.0))), true);
    TS_ASSERT_EQUALS(bbox.doesLineIntersect(Track(V3D(10.0,10.0,0.0),V3D(-1.0,-0.4,0.0))), false);

  }

  void test_That_Angular_Width_From_Point_Outside_Bounding_Box_Is_Valid()
  {
    BoundingBox bbox(4.1, 4.1, 4.1, -4.1, -4.1, -4.1);

    // Same distance each side
    TS_ASSERT_DELTA(bbox.angularWidth(V3D(-8.,0.,0.)), 0.97868779, 1e-08);
    TS_ASSERT_DELTA(bbox.angularWidth(V3D(8.,0.,0.)), 0.97868779, 1e-08);
    TS_ASSERT_DELTA(bbox.angularWidth(V3D(0.,-8.,0.)), 0.97868779, 1e-08);
    TS_ASSERT_DELTA(bbox.angularWidth(V3D(0.,8.,0.)), 0.97868779, 1e-08);
    TS_ASSERT_DELTA(bbox.angularWidth(V3D(0.,0.,-8.)), 0.97868779, 1e-08);
    TS_ASSERT_DELTA(bbox.angularWidth(V3D(0.,0.,8.)), 0.97868779, 1e-08);

    // Other points
    TS_ASSERT_DELTA(bbox.angularWidth(V3D(-8.,-8.,0.)), 0.63924353, 1e-08);
    TS_ASSERT_DELTA(bbox.angularWidth(V3D(10.,10.,10.)), 0.42097566, 1e-08);
  }

  void test_That_A_Defined_Bounding_Box_Grows_By_Enough_In_The_Correct_Direction_To_Encompass_A_New_Box()
  {
    BoundingBox parentBox(1.,2.,3., -0.5, -1.5, -2.5);
    BoundingBox otherBox(1.5,3.,3., 0.0, -2.0, -2.5);
    parentBox.grow(otherBox);

    TS_ASSERT_DELTA(parentBox.xMin(), -0.5, 1e-08);
    TS_ASSERT_DELTA(parentBox.xMax(), 1.5, 1e-08);
    TS_ASSERT_DELTA(parentBox.yMin(), -2.0, 1e-08);
    TS_ASSERT_DELTA(parentBox.yMax(), 3.0, 1e-08);
    TS_ASSERT_DELTA(parentBox.zMin(), -2.5, 1e-08);
    TS_ASSERT_DELTA(parentBox.zMax(), 3.0, 1e-08);
  }

  void test_That_An_Empty_Bounding_Box_Grows_By_Enough_In_The_Correct_Direction_To_Encompass_A_New_Box()
  {
    BoundingBox parentBox;
    BoundingBox otherBox(1.5,2.,3., 0.5, -2.0, -2.5);
    parentBox.grow(otherBox);

    TS_ASSERT_DELTA(parentBox.xMin(), 0.5, 1e-08);
    TS_ASSERT_DELTA(parentBox.xMax(), 1.5, 1e-08);
    TS_ASSERT_DELTA(parentBox.yMin(), -2.0, 1e-08);
    TS_ASSERT_DELTA(parentBox.yMax(), 2.0, 1e-08);
    TS_ASSERT_DELTA(parentBox.zMin(), -2.5, 1e-08);
    TS_ASSERT_DELTA(parentBox.zMax(), 3.0, 1e-08);
  }

  void test_That_The_Width_Returns_The_Correct_Vector_For_The_Box()
  {
    BoundingBox box(3.0,4.0,5.0,1.0,1.0,2.5);
    TS_ASSERT_EQUALS(box.width(), V3D(2.0, 3.0, 2.5));
  }
  void testNullifyWorks(){
      BoundingBox box(3.0,4.0,5.0,1.0,1.0,2.5);
      box.nullify();
      TS_ASSERT_EQUALS(box.isNull(), true);
      TS_ASSERT_EQUALS(box.maxPoint()==V3D(-FLT_MAX,-FLT_MAX,-FLT_MAX), true);
      TS_ASSERT_EQUALS(box.minPoint()==V3D(FLT_MAX,FLT_MAX,FLT_MAX), true);
  }
  void testBB_expansion_works_fine(){
      BoundingBox box(3.0,4.0,5.5,1.0,1.0,1.5);
      std::vector<V3D> points;
      box.getFullBox(points,V3D(1,1,1.5));

      TS_ASSERT_EQUALS(points[0]==V3D(0,0,0),true);
      TS_ASSERT_EQUALS(points[1]==V3D(2,0,0),true);
      TS_ASSERT_EQUALS(points[2]==V3D(2,3,0),true);
      TS_ASSERT_EQUALS(points[3]==V3D(0,3,0),true);
      TS_ASSERT_EQUALS(points[4]==V3D(0,3,4),true);
      TS_ASSERT_EQUALS(points[5]==V3D(0,0,4),true);
      TS_ASSERT_EQUALS(points[6]==V3D(2,0,4),true);
      TS_ASSERT_EQUALS(points[7]==V3D(2,3,4),true);
  }
  void testAxisAlignedCSisSimple(){
    BoundingBox bbox(3.0,4.0,5.5,1.0,1.0,1.5);
    std::vector<V3D> cs;
    bbox.getCoordSystem(cs);
    TS_ASSERT_EQUALS(bbox.isAxisAligned(),true);

    TS_ASSERT_EQUALS(cs.size(),4);
    TS_ASSERT_EQUALS(cs[0]==V3D(0,0,0),true);
    TS_ASSERT_EQUALS(cs[1]==V3D(1,0,0),true);
    TS_ASSERT_EQUALS(cs[2]==V3D(0,1,0),true);
    TS_ASSERT_EQUALS(cs[3]==V3D(0,0,1),true);
  }

  void testBBAlignedToNewCoordinateSystemIsCorrect(){
    BoundingBox bbox(3.0,4.0,5.5,1.0,1.0,1.5);
    std::vector<V3D> ort(3);
    ort[0]=V3D(0,1,0);
    ort[1]=V3D(0,0,1);
    ort[2]=V3D(1,0,0);
    bbox.setBoxAlignment(V3D(1,1,1.5),ort);

    bbox.getCoordSystem(ort);
    TS_ASSERT_EQUALS(bbox.isAxisAligned(),false);

    TS_ASSERT_EQUALS(ort.size(),4);
    TS_ASSERT_EQUALS(ort[0]==V3D(1,1,1.5),true);
    TS_ASSERT_EQUALS(ort[1]==V3D(0,1,0),true);
    TS_ASSERT_EQUALS(ort[2]==V3D(0,0,1),true);
    TS_ASSERT_EQUALS(ort[3]==V3D(1,0,0),true);

    TS_ASSERT_THROWS(bbox.doesLineIntersect(V3D(-5.0,-1.0,0.0),V3D(1.0,1.0,0.0)),Kernel::Exception::NotImplementedError);
    TS_ASSERT_THROWS(bbox.isPointInside(V3D(-5.0,-1.0,0.0)),Kernel::Exception::NotImplementedError);
  }
  void testBBAlignedToNewCoordinateSystemWorksCorrect(){
      BoundingBox bbox(3.0,4.0,5.5,1.0,1.0,1.5);
      std::vector<V3D> ort(3);
      ort[0]=V3D(0,1,0);
      ort[1]=V3D(0,0,1);
      ort[2]=V3D(1,0,0);
      bbox.setBoxAlignment(V3D(1,1,1.5),ort);
      TS_ASSERT_EQUALS(bbox.isAxisAligned(),false);
      bbox.realign();

      TS_ASSERT_EQUALS(bbox.minPoint()==V3D(0,0,0),true);
      TS_ASSERT_EQUALS(bbox.maxPoint()==V3D(3,4,2),true);
  }
  void testBBComplexRealignmentOK(){
      BoundingBox bbox(2,2,2,1,1,1);
      std::vector<V3D> cs(4);
      cs[0] =V3D(1,1,1);
      cs[1]= V3D(1, 1,0);
      cs[2]= V3D(1,-1,0);
      cs[1].normalize();
      cs[2].normalize();
      cs[3]=cs[1].cross_prod(cs[2]);

      bbox.realign(&cs);
      TS_ASSERT_EQUALS(bbox.isAxisAligned(),false);

      TSM_ASSERT_EQUALS("min point should be (0,-sqrt(2.)/2,-1)",bbox.minPoint()==V3D(0,-sqrt(2.)/2,-1),true);
      TSM_ASSERT_EQUALS("max point should be (sqrt(2.),sqrt(2.)/2,0)",bbox.maxPoint()==V3D(sqrt(2.),sqrt(2.)/2,0),true);
    }
 void testBBComplexRealignment2OK(){
      BoundingBox bbox(2,2,2,1,1,1);
      std::vector<V3D> cs(4);
      cs[0] =V3D(1,1,1);
      cs[2]= V3D(0, 1,1);
      cs[3]= V3D(0,-1,1);
      cs[2].normalize();
      cs[3].normalize();
      cs[1]=cs[2].cross_prod(cs[3]);

      bbox.realign(&cs);
      TS_ASSERT_EQUALS(bbox.isAxisAligned(),false);

      TSM_ASSERT_EQUALS("min point should be (0,0,-sqrt(2.)/2)",bbox.minPoint()==V3D(0,0,-sqrt(2.)/2),true);
      TSM_ASSERT_EQUALS("max point should be (1,sqrt(2.),sqrt(2.)/2)",bbox.maxPoint()==V3D(1,sqrt(2.),sqrt(2.)/2),true);
    }

private:

  void doPointTest(bool insideTest)
  {
    const double unit(1.0);
    BoundingBox bbox(unit, unit, unit, -unit, -unit, -unit);
    // Origin
    TS_ASSERT_EQUALS(bbox.isPointInside(V3D(0.0,0.0,0.0)), true);

    const double tol = Mantid::Geometry::Tolerance;
    /// The test is a strictly less-than test so when outside we need slighly more than the tolerance to be truely outside
    double singleDimValue = insideTest ? unit-tol : unit+tol+0.01*tol;
    // Near, but inside, +X-edge
    TS_ASSERT_EQUALS(bbox.isPointInside(V3D(singleDimValue,0.0,0.0)), insideTest);
    // Near, but inside, +Y-edge
    TS_ASSERT_EQUALS(bbox.isPointInside(V3D(0.0,singleDimValue,0.0)), insideTest);
    // Near, but inside, +Z-edge
    TS_ASSERT_EQUALS(bbox.isPointInside(V3D(0.0,0.0,singleDimValue)), insideTest);
    // Near, but inside, -X-edge
    TS_ASSERT_EQUALS(bbox.isPointInside(V3D(-(singleDimValue),0.0,0.0)), insideTest);
    // Near, but inside, -Y-edge
    TS_ASSERT_EQUALS(bbox.isPointInside(V3D(0.0,-(singleDimValue),0.0)), insideTest);
    // Near, but inside, -Z-edge
    TS_ASSERT_EQUALS(bbox.isPointInside(V3D(0.0,0.0,-(singleDimValue))), insideTest);

  }


};

#endif //BOUNDINGBOXTEST_H_
