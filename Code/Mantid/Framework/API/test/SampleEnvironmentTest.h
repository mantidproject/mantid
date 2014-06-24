#ifndef TESTSAMPLEENVIRONMENT_H_
#define TESTSAMPLEENVIRONMENT_H_

#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using Mantid::API::SampleEnvironment;
using namespace Mantid::PhysicalConstants;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

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
    Component part("part");
    SampleEnvironment kit("TestKit");
    TS_ASSERT_THROWS(kit.add(&part), std::invalid_argument);
  }


  //--------------------------------------------------------------------------------------------
  Object_sptr createCappedCylinder(double radius, double height, const V3D & baseCentre, const V3D & axis, const std::string & id)
  {
    std::ostringstream xml;
    xml << "<cylinder id=\"" << id << "\">"
      << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\"" << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\"/>"
      << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
      << "<radius val=\"" << radius << "\" />"
      << "<height val=\"" << height << "\" />"
      << "</cylinder>";

    ShapeFactory shapeMaker;
    return shapeMaker.createShape(xml.str());
  }
  ObjComponent * createSingleObjectComponent()
  {
    Object_sptr pixelShape = createCappedCylinder(0.5, 1.5, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube");
    return new ObjComponent("pixel", pixelShape);
  }

  void test_That_Adding_Valid_Components_Gives_The_Correct_Number_Of_Elements_In_The_Environment()
  {
    ObjComponent *physicalObject = createSingleObjectComponent();
    SampleEnvironment kit("TestKit");
    
    int numElements(0);
    TS_ASSERT_THROWS_NOTHING(numElements = kit.add(physicalObject));
    TS_ASSERT_EQUALS(numElements, 1);
  }



};



#endif // TESTSAMPLEENVIRONMENT_H_
