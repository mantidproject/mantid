#ifndef TESTSAMPLE_H_
#define TESTSAMPLE_H_

#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Exception.h"
#include <cxxtest/TestSuite.h>

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

  //--------------------------------------------------------------------------------------------
  Object_sptr createCappedCylinder(double radius, double height, const V3D & baseCentre, const V3D & axis, const std::string & id)
  {
    std::ostringstream xml;
    xml << "<cylinder id=\"" << id << "\">"
      << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\"" << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\"/>"
      << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
      << "<radius val=\"" << radius << "\" />"
      << "<height val=\"" << height << "\" />"  << "</cylinder>";
    ShapeFactory shapeMaker;
    return shapeMaker.createShape(xml.str());
  }
  ObjComponent * createSingleObjectComponent()
  {
    Object_sptr pixelShape = createCappedCylinder(0.5, 1.5, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube");
    return new ObjComponent("pixel", pixelShape);
  }

  void testShape()
  {
    Object_sptr shape_sptr = 
      createCappedCylinder(0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
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
    kit->add(createSingleObjectComponent());
    
    TS_ASSERT_THROWS_NOTHING(sample.setEnvironment(kit));
    
    const SampleEnvironment & sampleKit = sample.getEnvironment();
    // Test that this references the correct object
    TS_ASSERT_EQUALS(&sampleKit, kit);
    TS_ASSERT_EQUALS(sampleKit.getName(), envName);
    TS_ASSERT_EQUALS(sampleKit.nelements(), 1);
  }

  void test_OrientedLattice()
  {
    Sample sample;
    const std::string envName("TestKit");
    OrientedLattice *latt = new OrientedLattice(1.0,2.0,3.0, 90, 90, 90);

    TS_ASSERT_THROWS_NOTHING(sample.setOrientedLattice(latt));

    const OrientedLattice & retLatt = sample.getOrientedLattice();
    // Test that this references the correct object
    TS_ASSERT_EQUALS(&retLatt, latt);
    TS_ASSERT_EQUALS(retLatt.a(), 1.0);
    TS_ASSERT_EQUALS(retLatt.b(), 2.0);
    TS_ASSERT_EQUALS(retLatt.c(), 3.0);
  }


  void test_OrientedLattice_and_theCopyconstructor()
  {
    Sample sample;
    const std::string envName("TestKit");
    OrientedLattice *latt = new OrientedLattice(1.0,2.0,3.0, 90, 90, 90);

    TS_ASSERT_THROWS_NOTHING(sample.setOrientedLattice(latt));

    // Copy constructor
    Sample sample2(sample);

    // Equals operator
    Sample sample3;
    sample3 = sample;
    TS_ASSERT_EQUALS(sample3.getOrientedLattice().c(), 3.0);

    // Change the lattice in the original (this won't change the copy)
    sample.getOrientedLattice().seta(4.0);
    sample.getOrientedLattice().setb(5.0);

    const OrientedLattice & retLatt = sample2.getOrientedLattice();
    // The copy does NOT refer to the same object
    TS_ASSERT_DIFFERS(&retLatt, latt);
    TS_ASSERT_EQUALS(retLatt.a(), 1.0);
    TS_ASSERT_EQUALS(retLatt.b(), 2.0);
    TS_ASSERT_EQUALS(retLatt.c(), 3.0);

  }


  void test_Material_Returns_The_Correct_Value()
  {
    Material *vanBlock = new Material("vanBlock", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072);
    Sample sample;
    sample.setMaterial(*vanBlock);

    const Material * mat = &sample.getMaterial();
    const double lambda(2.1);
    TS_ASSERT_DELTA(mat->cohScatterXSection(lambda), 0.0184,  1e-02);
    TS_ASSERT_DELTA(mat->incohScatterXSection(lambda), 5.08,  1e-02);
    TS_ASSERT_DELTA(mat->absorbXSection(lambda), 5.93, 1e-02);

    delete vanBlock;
  }

  

  
};

#endif /*TESTSAMPLE_H_*/
