#ifndef TESTSAMPLE_H_
#define TESTSAMPLE_H_

#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Exception.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/NexusTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
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

  void testShape()
  {
    Object_sptr shape_sptr = \
      ComponentCreationHelper::createCappedCylinder(0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
    Sample sample;
    TS_ASSERT_THROWS_NOTHING(sample.setShape(*shape_sptr))
    const Object & sampleShape = sample.getShape();
    TS_ASSERT_EQUALS(shape_sptr->getName(), sampleShape.getName());
  }

  void test_Setting_Default_Shape_Is_Accepted()
  {
    Sample sample;
    Object object;
    TS_ASSERT_EQUALS(object.hasValidShape(), false);
    TS_ASSERT_THROWS_NOTHING(sample.setShape(object));
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
    kit->add(Object());

    TS_ASSERT_THROWS_NOTHING(sample.setEnvironment(kit));

    const SampleEnvironment & sampleKit = sample.getEnvironment();
    // Test that this references the correct object
    TS_ASSERT_EQUALS(&sampleKit, kit);
    TS_ASSERT_EQUALS(sampleKit.name(), envName);
    TS_ASSERT_EQUALS(sampleKit.nelements(), 1);
  }

  void test_OrientedLattice()
  {
    Sample sample;
    OrientedLattice *latt = new OrientedLattice(1.0,2.0,3.0, 90, 90, 90);

    TS_ASSERT_THROWS_NOTHING(sample.setOrientedLattice(latt));

    const OrientedLattice & retLatt = sample.getOrientedLattice();
    // Test that this references the correct object
    //TS_ASSERT_EQUALS(&retLatt, latt);//This is no longer correct. setOrientedLattice makes a copy of the OrientedLattice object
    TS_ASSERT_EQUALS(retLatt.a(), 1.0);
    TS_ASSERT_EQUALS(retLatt.b(), 2.0);
    TS_ASSERT_EQUALS(retLatt.c(), 3.0);
    delete latt;
  }


  void test_OrientedLattice_and_theCopyconstructor()
  {
    Sample sample;
    //const std::string envName("TestKit");
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
    delete latt;
  }

  void test_clearOrientedLattice()
  {
    Sample sample;
    OrientedLattice *latt = new OrientedLattice(1.0,2.0,3.0, 90, 90, 90);
    TS_ASSERT_THROWS_NOTHING(sample.setOrientedLattice(latt));

    TS_ASSERT(sample.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sample.getOrientedLattice())

    // Now clear it.
    sample.clearOrientedLattice();

    TS_ASSERT(!sample.hasOrientedLattice())
    TS_ASSERT_THROWS(sample.getOrientedLattice(), std::runtime_error&)
    delete latt;
  }

  void test_clearOrientedLattice_and_the_copy_constructor()
  {
    // Create a sample with an oriented lattice.
    Sample sampleA;
    OrientedLattice *latticeA = new OrientedLattice(1.0,2.0,3.0, 90, 90, 90);
    TS_ASSERT_THROWS_NOTHING(sampleA.setOrientedLattice(latticeA));

    // Copy the sample.
    Sample sampleB(sampleA);

    // Check oriented lattice objects on both.
    TS_ASSERT(sampleA.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sampleA.getOrientedLattice())
    TS_ASSERT(sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sampleB.getOrientedLattice())

    // Now clear one.
    sampleA.clearOrientedLattice();

    // One should be cleared, the other should not.
    TS_ASSERT(!sampleA.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleA.getOrientedLattice(), std::runtime_error&)
    TS_ASSERT(sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sampleB.getOrientedLattice())

    // Now clear both.
    sampleA.clearOrientedLattice(); // Not strictly necessary, but object should be able to survive such calls.
    sampleB.clearOrientedLattice();

    // Both should be cleared.
    TS_ASSERT(!sampleA.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleA.getOrientedLattice(), std::runtime_error&)
    TS_ASSERT(!sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleB.getOrientedLattice(), std::runtime_error&)
    delete latticeA;
  }

  void test_clearOrientedLattice_and_assignment()
  {
    // Create a sample with an oriented lattice.
    Sample sampleA;
    OrientedLattice *latticeA = new OrientedLattice(1.0, 2.0, 3.0, 90, 90, 90);
    TS_ASSERT_THROWS_NOTHING(sampleA.setOrientedLattice(latticeA));

    // Create and then assign to the sample.
    Sample sampleB;
    sampleB = sampleA;

    // Check oriented lattice objects on both.
    TS_ASSERT(sampleA.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sampleA.getOrientedLattice())
    TS_ASSERT(sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sampleB.getOrientedLattice())

    // Now clear one.
    sampleA.clearOrientedLattice();

    // One should be cleared, the other should not.
    TS_ASSERT(!sampleA.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleA.getOrientedLattice(), std::runtime_error&)
    TS_ASSERT(sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sampleB.getOrientedLattice())

    // Now clear both.
    sampleA.clearOrientedLattice(); // Not strictly necessary, but object should be able to survive such calls.
    sampleB.clearOrientedLattice();

    // Both should be cleared.
    TS_ASSERT(!sampleA.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleA.getOrientedLattice(), std::runtime_error&)
    TS_ASSERT(!sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleB.getOrientedLattice(), std::runtime_error&)
    delete latticeA;
  }


  void test_Material_Returns_The_Correct_Value()
  {
    Material vanBlock("vanBlock", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072);
    Sample sample;
    Object shape;
    shape.setMaterial(vanBlock);
    sample.setShape(shape);

    const Material& mat = sample.getMaterial();
    const double lambda(2.1);
    TS_ASSERT_DELTA(mat.cohScatterXSection(lambda), 0.0184,  1e-02);
    TS_ASSERT_DELTA(mat.incohScatterXSection(lambda), 5.08,  1e-02);
    TS_ASSERT_DELTA(mat.absorbXSection(lambda), 5.93, 1e-02);
  }

  void test_Single_Sample()
  {
    Sample sample;
    sample.setName("test name for test_Single_Sample");
    TS_ASSERT_EQUALS(sample.size(),1);

    //void casts are to stop the unused variable warnings.
    TS_ASSERT_THROWS_ANYTHING(Sample& sampleRef = sample[1]; (void) sampleRef; );
    TS_ASSERT_THROWS_ANYTHING(Sample& sampleRef2 = sample[999]; (void) sampleRef2; );
    TS_ASSERT_THROWS_ANYTHING(Sample& sampleRef3 = sample[-1]; (void) sampleRef3; );
    TS_ASSERT_THROWS_NOTHING
    (
      Sample& sampleRef = sample[0];
      TS_ASSERT(sample.getName()==sampleRef.getName());
    );
  }

  void test_Multiple_Samples()
  {
    Sample sample;
    sample.setName("test name for test_Multiple_Sample");
    boost::shared_ptr<Sample> sample2 = boost::shared_ptr<Sample>(new Sample());
    sample2->setName("test name for test_Multiple_Sample - 2");

    TS_ASSERT_EQUALS(sample.size(),1);
    sample.addSample(sample2);
    TS_ASSERT_EQUALS(sample.size(),2);
    sample.addSample(sample2);
    TS_ASSERT_EQUALS(sample.size(),3);

    TS_ASSERT_THROWS_NOTHING
    (
      TS_ASSERT(sample[0].getName()==sample.getName());
      TS_ASSERT(sample[1].getName()==sample2->getName());
      TS_ASSERT(sample[2].getName()==sample2->getName());
    );

    TS_ASSERT_THROWS_ANYTHING(Sample& sampleRef = sample[3]; (void) sampleRef; );
  }

  void test_nexus()
  {
    NexusTestHelper th(true);
    th.createFile("SampleTest.nxs");

    Object_sptr shape_sptr = \
        ComponentCreationHelper::createCappedCylinder(0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
    Sample sample;
    sample.setShape(*shape_sptr);
    sample.setName("NameOfASample");
    sample.setWidth(1.234);
    OrientedLattice latt(4,5,6,90,91,92);
    sample.setOrientedLattice( &latt );
    boost::shared_ptr<Sample> sample2 = boost::shared_ptr<Sample>(new Sample());
    sample2->setName("test name for test_Multiple_Sample - 2");
    sample.addSample(sample2);
    TS_ASSERT( sample.getShape().getShapeXML() != "" );

    sample.saveNexus(th.file, "sample");
    th.reopenFile();

    Sample loaded;
    loaded.loadNexus(th.file, "sample");

    TS_ASSERT_EQUALS(loaded.size(), 2);
    TS_ASSERT_EQUALS(loaded.getName(), sample.getName());
    TS_ASSERT_EQUALS(loaded[0].getName(), sample[0].getName());
    TS_ASSERT_EQUALS(loaded[1].getName(), sample[1].getName());
    TS_ASSERT_EQUALS(loaded.hasOrientedLattice(), sample.hasOrientedLattice());
    TS_ASSERT_DELTA(loaded.getOrientedLattice().a(), 4.0, 1e-6);
    TS_ASSERT_DELTA(loaded.getOrientedLattice().b(), 5.0, 1e-6);
    TS_ASSERT_DELTA(loaded.getOrientedLattice().c(), 6.0, 1e-6);
    TS_ASSERT_EQUALS(loaded.getShape().getBoundingBox().xMax(), sample.getShape().getBoundingBox().xMax() );
    TS_ASSERT_EQUALS(loaded.getShape().getShapeXML(), sample.getShape().getShapeXML() );
    // Geometry values
    TS_ASSERT_DELTA(loaded.getWidth(), sample.getWidth(), 1e-6);

  }


};

#endif /*TESTSAMPLE_H_*/
