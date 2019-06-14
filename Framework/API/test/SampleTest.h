// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TESTSAMPLE_H_
#define TESTSAMPLE_H_

#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Material.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/NexusTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::API::Sample;

class SampleTest : public CxxTest::TestSuite {
public:
  void testSetGetName() {
    Sample sample;
    TS_ASSERT(!sample.getName().compare(""))
    sample.setName("test");
    TS_ASSERT(!sample.getName().compare("test"))
  }

  //--------------------------------------------------------------------------------------------

  void testShape() {
    IObject_sptr shape_sptr = ComponentCreationHelper::createCappedCylinder(
        0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
    Sample sample;
    TS_ASSERT_THROWS_NOTHING(sample.setShape(shape_sptr))
    const IObject &sampleShape = sample.getShape();
    TS_ASSERT_EQUALS(shape_sptr->getName(), sampleShape.getName());
  }

  void test_Setting_Default_Shape_Is_Accepted() {
    Sample sample;
    IObject_sptr object;
    TS_ASSERT_THROWS_NOTHING(sample.setShape(object));
    TS_ASSERT_EQUALS(sample.getShape().hasValidShape(), false);
  }

  void test_That_Requests_For_An_Undefined_Environment_Throw() {
    Sample sample;
    TS_ASSERT_THROWS(sample.getEnvironment(), const std::runtime_error &);
  }

  void
  test_That_An_Environment_Can_Be_Set_And_The_Same_Environment_Is_Returned() {
    Sample sample;
    const std::string envName("TestKit");
    auto kit = std::make_unique<SampleEnvironment>(
        envName, boost::make_shared<const Container>(""));
    kit->add(boost::make_shared<const CSGObject>());

    TS_ASSERT_THROWS_NOTHING(sample.setEnvironment(std::move(kit)));

    const SampleEnvironment &sampleKit = sample.getEnvironment();
    // Test that this references the correct object
    TS_ASSERT_EQUALS(sampleKit.name(), envName);
    TS_ASSERT_EQUALS(sampleKit.nelements(), 2);
  }

  void test_OrientedLattice() {
    Sample sample;
    OrientedLattice *latt = new OrientedLattice(1.0, 2.0, 3.0, 90, 90, 90);

    TS_ASSERT_THROWS_NOTHING(sample.setOrientedLattice(latt));

    const OrientedLattice &retLatt = sample.getOrientedLattice();
    // Test that this references the correct object
    // TS_ASSERT_EQUALS(&retLatt, latt);//This is no longer correct.
    // setOrientedLattice makes a copy of the OrientedLattice object
    TS_ASSERT_EQUALS(retLatt.a(), 1.0);
    TS_ASSERT_EQUALS(retLatt.b(), 2.0);
    TS_ASSERT_EQUALS(retLatt.c(), 3.0);
    delete latt;
  }

  void test_OrientedLattice_and_theCopyconstructor() {
    Sample sample;
    // const std::string envName("TestKit");
    OrientedLattice *latt = new OrientedLattice(1.0, 2.0, 3.0, 90, 90, 90);

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

    const OrientedLattice &retLatt = sample2.getOrientedLattice();
    // The copy does NOT refer to the same object
    TS_ASSERT_DIFFERS(&retLatt, latt);
    TS_ASSERT_EQUALS(retLatt.a(), 1.0);
    TS_ASSERT_EQUALS(retLatt.b(), 2.0);
    TS_ASSERT_EQUALS(retLatt.c(), 3.0);
    delete latt;
  }

  void test_clearOrientedLattice() {
    Sample sample;
    OrientedLattice *latt = new OrientedLattice(1.0, 2.0, 3.0, 90, 90, 90);
    TS_ASSERT_THROWS_NOTHING(sample.setOrientedLattice(latt));

    TS_ASSERT(sample.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sample.getOrientedLattice())

    // Now clear it.
    sample.clearOrientedLattice();

    TS_ASSERT(!sample.hasOrientedLattice())
    TS_ASSERT_THROWS(sample.getOrientedLattice(), std::runtime_error &)
    delete latt;
  }

  void test_clearOrientedLattice_and_the_copy_constructor() {
    // Create a sample with an oriented lattice.
    Sample sampleA;
    OrientedLattice *latticeA = new OrientedLattice(1.0, 2.0, 3.0, 90, 90, 90);
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
    TS_ASSERT_THROWS(sampleA.getOrientedLattice(), std::runtime_error &)
    TS_ASSERT(sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sampleB.getOrientedLattice())

    // Now clear both.
    sampleA.clearOrientedLattice(); // Not strictly necessary, but object should
                                    // be able to survive such calls.
    sampleB.clearOrientedLattice();

    // Both should be cleared.
    TS_ASSERT(!sampleA.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleA.getOrientedLattice(), std::runtime_error &)
    TS_ASSERT(!sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleB.getOrientedLattice(), std::runtime_error &)
    delete latticeA;
  }

  void test_clearOrientedLattice_and_assignment() {
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
    TS_ASSERT_THROWS(sampleA.getOrientedLattice(), std::runtime_error &)
    TS_ASSERT(sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS_NOTHING(sampleB.getOrientedLattice())

    // Now clear both.
    sampleA.clearOrientedLattice(); // Not strictly necessary, but object should
                                    // be able to survive such calls.
    sampleB.clearOrientedLattice();

    // Both should be cleared.
    TS_ASSERT(!sampleA.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleA.getOrientedLattice(), std::runtime_error &)
    TS_ASSERT(!sampleB.hasOrientedLattice())
    TS_ASSERT_THROWS(sampleB.getOrientedLattice(), std::runtime_error &)
    delete latticeA;
  }

  void test_setCrystalStructure() {
    Sample sample;
    TS_ASSERT(!sample.hasCrystalStructure());
    TS_ASSERT_THROWS(sample.getCrystalStructure(), const std::runtime_error &);

    CrystalStructure structure("3 4 5 90 90 90", "C m m m",
                               "Fe 0.12 0.23 0.121");

    TS_ASSERT_THROWS_NOTHING(sample.setCrystalStructure(structure));
    TS_ASSERT(sample.hasCrystalStructure());
    CrystalStructure fromSample = sample.getCrystalStructure();

    TS_ASSERT(fromSample.spaceGroup());
    TS_ASSERT_EQUALS(fromSample.spaceGroup()->hmSymbol(), "C m m m");
  }

  void test_clearCrystalStructure() {
    Sample sample;
    TS_ASSERT(!sample.hasCrystalStructure());
    TS_ASSERT_THROWS(sample.getCrystalStructure(), const std::runtime_error &);

    CrystalStructure structure("3 4 5 90 90 90", "C m m m",
                               "Fe 0.12 0.23 0.121");
    sample.setCrystalStructure(structure);
    TS_ASSERT(sample.hasCrystalStructure());

    TS_ASSERT_THROWS_NOTHING(sample.clearCrystalStructure());
    TS_ASSERT(!sample.hasCrystalStructure());
  }

  void test_crystalStructureCopyConstructorAndAssignment() {
    Sample sampleA;

    CrystalStructure structure("3 4 5 90 90 90", "C m m m",
                               "Fe 0.12 0.23 0.121");
    sampleA.setCrystalStructure(structure);
    TS_ASSERT(sampleA.hasCrystalStructure());

    Sample sampleB = sampleA;
    TS_ASSERT(sampleB.hasCrystalStructure());

    CrystalStructure fromA = sampleA.getCrystalStructure();
    CrystalStructure fromB = sampleB.getCrystalStructure();
    TS_ASSERT_EQUALS(fromA.spaceGroup()->hmSymbol(),
                     fromB.spaceGroup()->hmSymbol());

    Sample sampleC(sampleA);

    CrystalStructure fromC = sampleC.getCrystalStructure();
    TS_ASSERT_EQUALS(fromA.spaceGroup()->hmSymbol(),
                     fromC.spaceGroup()->hmSymbol());
  }

  void test_Material_Returns_The_Correct_Value() {
    Material vanBlock("vanBlock",
                      Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072);
    Sample sample;
    auto shape = Mantid::Geometry::ShapeFactory().createShape("");
    shape->setMaterial(vanBlock);
    sample.setShape(shape);

    const Material &mat = sample.getMaterial();
    const double lambda(2.1);
    TS_ASSERT_DELTA(mat.cohScatterXSection(lambda), 0.0184, 1e-02);
    TS_ASSERT_DELTA(mat.incohScatterXSection(lambda), 5.08, 1e-02);
    TS_ASSERT_DELTA(mat.absorbXSection(lambda), 5.93, 1e-02);
  }

  void test_Single_Sample() {
    Sample sample;
    sample.setName("test name for test_Single_Sample");
    TS_ASSERT_EQUALS(sample.size(), 1);

    // void casts are to stop the unused variable warnings.
    TS_ASSERT_THROWS_ANYTHING(Sample &sampleRef = sample[1]; (void)sampleRef;);
    TS_ASSERT_THROWS_ANYTHING(Sample &sampleRef2 = sample[999];
                              (void)sampleRef2;);
    TS_ASSERT_THROWS_ANYTHING(Sample &sampleRef3 = sample[-1];
                              (void)sampleRef3;);
    TS_ASSERT_THROWS_NOTHING(Sample &sampleRef = sample[0]; TS_ASSERT(
                                 sample.getName() == sampleRef.getName()););
  }

  void test_Multiple_Samples() {
    Sample sample;
    sample.setName("test name for test_Multiple_Sample");
    auto sample2 = boost::make_shared<Sample>();
    sample2->setName("test name for test_Multiple_Sample - 2");

    TS_ASSERT_EQUALS(sample.size(), 1);
    sample.addSample(sample2);
    TS_ASSERT_EQUALS(sample.size(), 2);
    sample.addSample(sample2);
    TS_ASSERT_EQUALS(sample.size(), 3);

    TS_ASSERT_THROWS_NOTHING(
        TS_ASSERT(sample[0].getName() == sample.getName());
        TS_ASSERT(sample[1].getName() == sample2->getName());
        TS_ASSERT(sample[2].getName() == sample2->getName()););

    TS_ASSERT_THROWS_ANYTHING(Sample &sampleRef = sample[3]; (void)sampleRef;);
  }

  void test_nexus() {
    NexusTestHelper th(true);
    th.createFile("SampleTest.nxs");

    IObject_sptr shape_sptr = ComponentCreationHelper::createCappedCylinder(
        0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
    Sample sample;
    sample.setShape(shape_sptr);
    sample.setName("NameOfASample");
    sample.setWidth(1.234);
    OrientedLattice latt(4, 5, 6, 90, 91, 92);
    sample.setOrientedLattice(&latt);
    auto sample2 = boost::make_shared<Sample>();
    sample2->setName("test name for test_Multiple_Sample - 2");
    sample.addSample(sample2);
    TS_ASSERT(
        dynamic_cast<const CSGObject &>(sample.getShape()).getShapeXML() != "");

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
    TS_ASSERT_EQUALS(loaded.getShape().getBoundingBox().xMax(),
                     sample.getShape().getBoundingBox().xMax());
    TS_ASSERT_EQUALS(
        dynamic_cast<const CSGObject &>(loaded.getShape()).getShapeXML(),
        dynamic_cast<const CSGObject &>(sample.getShape()).getShapeXML());
    // Geometry values
    TS_ASSERT_DELTA(loaded.getWidth(), sample.getWidth(), 1e-6);
  }

  void test_nexus_empty_name() {
    NexusTestHelper th(true);
    th.createFile("SampleTest.nxs");

    Sample sample;

    sample.saveNexus(th.file, "sample");
    th.reopenFile();

    Sample loaded;
    loaded.loadNexus(th.file, "sample");

    TS_ASSERT(loaded.getName().empty());
  }
};

#endif /*TESTSAMPLE_H_*/
