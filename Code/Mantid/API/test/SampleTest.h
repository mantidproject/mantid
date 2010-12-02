#ifndef TESTSAMPLE_H_
#define TESTSAMPLE_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/SampleEnvironment.h"

#include "../../Geometry/test/ComponentCreationHelpers.hh"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::API::Sample;
using Mantid::API::SampleEnvironment;

class SampleTest : public CxxTest::TestSuite
{
public:
  void testSetGetName()
  {
    Sample sample;
    TS_ASSERT( ! sample.getName().compare("") )
    sample.setName("test");
    TS_ASSERT( ! sample.getName().compare("test") )
  }

  void testShape()
  {
    Object_sptr shape_sptr = 
      ComponentCreationHelper::createCappedCylinder(0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
    Sample sample;
    TS_ASSERT_THROWS_NOTHING(sample.setShape(*shape_sptr))
    const Object & sampleShape = sample.getShape();
    TS_ASSERT_EQUALS(shape_sptr->getName(), sampleShape.getName());
  }

  void test_That_An_Setting_An_Invalid_Shape_Throws_An_Invalid_Argument()
  {
    Sample sample;
    Object object;
    TS_ASSERT_EQUALS(object.hasValidShape(), false);
    TS_ASSERT_THROWS(sample.setShape(object), std::invalid_argument);
  }

  void test_Requests_For_Pos_Information_With_Valid_Component_Link_Do_Not_Throw()
  {
    const V3D pos(0.0,0.0,1.0);
    const Quat rot(10., V3D(0.,1.0,0.0));
    Component *sampleHolder = new Component("SamplePos", pos, rot);
    Sample sample;
    sample.attachToPosition(sampleHolder);

    TS_ASSERT_EQUALS(sample.getPos(), pos);
    TS_ASSERT_EQUALS(sample.getRotation(), rot);
  }

  void test_Requests_For_Pos_Information_Without_Attaching_A_Component_Throw()
  {
    Sample sample;
    TS_ASSERT_THROWS(sample.getPos(), std::runtime_error);
    TS_ASSERT_THROWS(sample.getRotation(), std::runtime_error);
  }
  
  void test_That_Requests_For_An_Undefined_Environment_Throw()
  {
    Sample sample;
    TS_ASSERT_THROWS(sample.getEnvironment(), std::runtime_error);
  }

  void test_That_An_Environment_Can_Be_Set_And_The_Same_Environment_Is_Returned()
  {
    Sample sample;
    const std::string envName("TestKit");
    SampleEnvironment *kit = new SampleEnvironment(envName);
    kit->add(ComponentCreationHelper::createSingleObjectComponent());
    
    TS_ASSERT_THROWS_NOTHING(sample.setEnvironment(kit));
    
    const SampleEnvironment & sampleKit = sample.getEnvironment();
    // Test that this references the correct object
    TS_ASSERT_EQUALS(&sampleKit, kit);
    TS_ASSERT_EQUALS(sampleKit.getName(), envName);
    TS_ASSERT_EQUALS(sampleKit.nelements(), 1);
  }
  
};

#endif /*TESTSAMPLE_H_*/
