#ifndef TESTSAMPLEENVIRONMENT_H_
#define TESTSAMPLEENVIRONMENT_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::API::SampleEnvironment;
using namespace Mantid::PhysicalConstants;
using namespace Mantid::Geometry;

class SampleEnvironmentTest : public CxxTest::TestSuite
{

public:
  
  void test_That_Constructor_Giving_Name_Creates_The_Correct_Name()
  {
    SampleEnvironment kit("TestKit");
    TS_ASSERT_EQUALS(kit.getName(), "TestKit");
  }

  void test_That_Type_Is_SampleEnvironment()
  {
    SampleEnvironment kit("kit1");
    TS_ASSERT_EQUALS(kit.type(), "SampleEnvironment");
  }

  void test_That_Adding_A_Component_Without_A_Shape_Throws_Invalid_Argument()
  {
    Component *part = new Component("part");
    SampleEnvironment kit("TestKit");
    TS_ASSERT_THROWS(kit.add(part), std::invalid_argument);    
  }

  void test_That_Adding_Valid_Components_Gives_The_Correct_Number_Of_Elements_In_The_Environment()
  {
    ObjComponent *physicalObject = ComponentCreationHelper::createSingleObjectComponent();
    SampleEnvironment kit("TestKit");
    
    int numElements(0);
    TS_ASSERT_THROWS_NOTHING(numElements = kit.add(physicalObject));
    TS_ASSERT_EQUALS(numElements, 1);
  }



};



#endif // TESTSAMPLEENVIRONMENT_H_
