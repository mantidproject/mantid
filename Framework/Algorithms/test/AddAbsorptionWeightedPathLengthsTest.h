// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/AddAbsorptionWeightedPathLengths.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class AddAbsorptionWeightedPathLengthsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddAbsorptionWeightedPathLengthsTest *createSuite() { return new AddAbsorptionWeightedPathLengthsTest(); }
  static void destroySuite(AddAbsorptionWeightedPathLengthsTest *suite) { delete suite; }

  void test_spherical_sample_single_onbeam_detector() {
    using namespace Mantid::Kernel;

    auto peaksWS = std::make_shared<PeaksWorkspace>();
    setTestInstrument(peaksWS);
    setMaterialToVanadium(peaksWS);

    auto parametrizedInstrument = peaksWS->getInstrument();

    const int NPEAKS = 10;
    for (int i = 0; i < NPEAKS; ++i) {
      Peak peak(parametrizedInstrument, 1, i + 0.5);
      peaksWS->addPeak(peak);
    }

    // make beam v narrow so simulated paths all pass through sphere centre
    auto &paramMap = peaksWS->instrumentParameters();
    auto parametrizedSource = parametrizedInstrument->getSource();
    paramMap.addString(parametrizedSource.get(), "beam-shape", "Slit");
    paramMap.addDouble(parametrizedSource.get(), "beam-width", 0.000001);
    paramMap.addDouble(parametrizedSource.get(), "beam-height", 0.000001);

    Mantid::Algorithms::AddAbsorptionWeightedPathLengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", peaksWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EventsPerPoint", 1000));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("SeedValue", "654321"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););

    for (int i = 0; i < NPEAKS; i++) {
      Mantid::Geometry::IPeak &peak = peaksWS->getPeak(i);
      const double delta(1e-04);
      // all path lengths regardless of peak wavelength should be 2 * radius
      // ie 2mm
      TS_ASSERT_DELTA(0.2000, peak.getAbsorptionWeightedPathLength(), delta);
    }
  }

  void test_spherical_sample() {
    using namespace Mantid::Kernel;
    const int NPEAKS = 10;
    // this sets up a sample with a spherical shape of radius = 1mm
    auto peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(NPEAKS);
    auto shape = ComponentCreationHelper::createSphere(0.001, {0, 0, 0}, "sample-shape");
    peaksWS->mutableSample().setShape(shape);
    setMaterialToVanadium(peaksWS);

    Mantid::Algorithms::AddAbsorptionWeightedPathLengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", peaksWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EventsPerPoint", 1000));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););

    Mantid::Geometry::IPeak &peak = peaksWS->getPeak(0);
    const double delta(1e-04);
    // weighted path length will be less than 2mm because off centre scatter
    // points that are near the detector will have significantly shorter paths
    // than those on the opposite side of the sphere
    TS_ASSERT_DELTA(0.1508, peak.getAbsorptionWeightedPathLength(), delta);
  }

  void test_spherical_sample_lean() {
    using namespace Mantid::Kernel;
    const int NPEAKS = 10;
    // this sets up a sample with a spherical shape of radius = 1mm
    auto peaksWS = WorkspaceCreationHelper::createLeanPeaksWorkspace(NPEAKS);
    auto shape = ComponentCreationHelper::createSphere(0.001, {0, 0, 0}, "sample-shape");
    peaksWS->mutableSample().setShape(shape);
    setMaterialToVanadiumLean(peaksWS);

    Mantid::Algorithms::AddAbsorptionWeightedPathLengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", peaksWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EventsPerPoint", 1000));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););

    Mantid::Geometry::IPeak &peak = peaksWS->getPeak(0);
    const double delta(1e-04);
    // weighted path length will be less than 2mm because off centre scatter
    // points that are near the detector will have significantly shorter paths
    // than those on the opposite side of the sphere
    TS_ASSERT_DELTA(0.1508, peak.getAbsorptionWeightedPathLength(), delta);
  }

  void test_no_sample() {
    auto peaksWS = std::make_shared<PeaksWorkspace>();
    Mantid::Algorithms::AddAbsorptionWeightedPathLengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", peaksWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EventsPerPoint", 1000));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
  void test_sample_without_material() {
    auto peaksWS = std::make_shared<PeaksWorkspace>();
    setTestInstrument(peaksWS);

    Mantid::Algorithms::AddAbsorptionWeightedPathLengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", peaksWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EventsPerPoint", 1000));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
  void test_sample_with_environment() {
    auto peaksWS = std::make_shared<PeaksWorkspace>();
    setTestInstrument(peaksWS);
    setMaterialToVanadium(peaksWS);

    auto sphere = ComponentCreationHelper::createSphere(0.002, {0, 0, 0}, "environment");

    auto can = std::make_shared<Mantid::Geometry::Container>(sphere);
    auto environment = std::make_unique<SampleEnvironment>("environment", can);
    peaksWS->mutableSample().setEnvironment(std::move(environment));

    Mantid::Algorithms::AddAbsorptionWeightedPathLengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", peaksWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EventsPerPoint", 1000));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
  void test_single_path() {
    using namespace Mantid::Kernel;
    const int NPEAKS = 10;
    // this sets up a sample with a spherical shape of radius = 1mm
    auto peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(NPEAKS);
    auto shape = ComponentCreationHelper::createSphere(0.001, {0, 0, 0}, "sample-shape");
    peaksWS->mutableSample().setShape(shape);
    setMaterialToVanadium(peaksWS);

    Mantid::Algorithms::AddAbsorptionWeightedPathLengths alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", peaksWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseSinglePath", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););

    Mantid::Geometry::IPeak &peak = peaksWS->getPeak(0);
    const double delta(1e-06);
    TS_ASSERT_DELTA(0.2, peak.getAbsorptionWeightedPathLength(), delta);
  }

private:
  void setTestInstrument(std::shared_ptr<PeaksWorkspace> peaksWS) {
    // create a test instrument with a single detector on the beam line so
    // that a test case with a simple path length calculation can be created
    const auto instrName = "tbar_test";
    auto testInst = std::make_shared<Instrument>(instrName);

    const double cylRadius(0.008 / 2);
    const double cylHeight(0.0002);
    // One object
    auto pixelShape = ComponentCreationHelper::createCappedCylinder(
        cylRadius, cylHeight, V3D(0.0, -cylHeight / 2.0, 0.0), V3D(0., 1.0, 0.), "pixel-shape");

    Detector *det = new Detector("det", 1, pixelShape, nullptr);
    det->setPos(0, 0, 1);
    testInst->add(det);
    testInst->markAsDetector(det);

    ComponentCreationHelper::addSourceToInstrument(testInst, V3D(0.0, 0.0, -10.0));
    ComponentCreationHelper::addSampleToInstrument(testInst, V3D(0.0, 0.0, 0.0));

    peaksWS->setInstrument(testInst);

    auto shape = ComponentCreationHelper::createSphere(0.001, {0, 0, 0}, "sample-shape");
    peaksWS->mutableSample().setShape(shape);
  }
  void setMaterialToVanadium(std::shared_ptr<PeaksWorkspace> peaksWS) {
    auto shape = std::shared_ptr<IObject>(peaksWS->sample().getShape().cloneWithMaterial(
        Mantid::Kernel::Material("Vanadium", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072)));
    peaksWS->mutableSample().setShape(shape);
  }
  void setMaterialToVanadiumLean(std::shared_ptr<LeanElasticPeaksWorkspace> peaksWS) {
    auto shape = std::shared_ptr<IObject>(peaksWS->sample().getShape().cloneWithMaterial(
        Mantid::Kernel::Material("Vanadium", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072)));
    peaksWS->mutableSample().setShape(shape);
  }
};
