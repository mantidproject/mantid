// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/CuboidGaugeVolumeAbsorption.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "SampleFrameEquivalence.h"

#include <cmath>

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;

class CuboidGaugeVolumeAbsorptionTest : public CxxTest::TestSuite {
public:
  void testBasics() {
    TS_ASSERT_EQUALS(atten.name(), "CuboidGaugeVolumeAbsorption");
    TS_ASSERT_EQUALS(atten.version(), 1);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(atten.initialize());
    TS_ASSERT(atten.isInitialized());
  }

  void testFailsIfNoInstrument() {
    // Create a simple test workspace that has no instrument
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspace(10, 5);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_THROWS(atten.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS), const std::invalid_argument &);
  }

  void testFailsIfNoSampleShape() {
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(9, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::CuboidGaugeVolumeAbsorption abs;
    abs.initialize();
    TS_ASSERT_THROWS_NOTHING(abs.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    // None of the below values matter - they just have to be set to something
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("SampleHeight", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("SampleWidth", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("SampleThickness", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("AttenuationXSection", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("ScatteringXSection", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("SampleNumberDensity", "1"));
    TS_ASSERT(!abs.execute());
  }

  void testFailsIfSampleSmallerThanGaugeVolume() {
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(9, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    // Define a sample shape
    auto sampleShape = ComponentCreationHelper::createCuboid(0.005, 0.003, 0.002);
    testWS->mutableSample().setShape(sampleShape);

    Mantid::Algorithms::CuboidGaugeVolumeAbsorption abs;
    abs.initialize();
    TS_ASSERT_THROWS_NOTHING(abs.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    // None of the below values matter - they just have to be set to something
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("SampleHeight", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("SampleWidth", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("SampleThickness", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("AttenuationXSection", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("ScatteringXSection", "1"));
    TS_ASSERT_THROWS_NOTHING(abs.setPropertyValue("SampleNumberDensity", "1"));
    TS_ASSERT(!abs.execute());
  }

  void testExec() {
    if (!atten.isInitialized())
      atten.initialize();

    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    // Define a sample shape
    auto sampleShape = ComponentCreationHelper::createCuboid(0.025, 0.03, 0.02);
    testWS->mutableSample().setShape(sampleShape);

    TS_ASSERT_THROWS_NOTHING(atten.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleHeight", "2.3"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleWidth", "1.8"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleThickness", "1.5"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("AttenuationXSection", "6.52"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("ScatteringXSection", "19.876"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleNumberDensity", "0.0093"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("NumberOfWavelengthPoints", "3"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("ExpMethod", "Normal"));
    TS_ASSERT_THROWS_NOTHING(atten.execute());
    TS_ASSERT(atten.isExecuted());

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    // This test cut and paste from FlatPlateAbsorption. Since we have a larger
    // sample now, but the
    // same integration volume, the numbers have to be smaller.
    TS_ASSERT_LESS_THAN(result->y(0).front(), 0.7235);
    TS_ASSERT_LESS_THAN(result->y(0)[1], 0.6888);
    TS_ASSERT_LESS_THAN(result->y(0).back(), 0.4603);
    TS_ASSERT_LESS_THAN(result->y(1).front(), 0.7235);
    TS_ASSERT_LESS_THAN(result->y(1)[5], 0.5616);
    TS_ASSERT_LESS_THAN(result->y(1).back(), 0.4603);

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void test_both_ways_of_orienting_the_sample_agree() {
    // A sample in its own frame with the rotation on the run, and the same sample already rotated
    // into the lab frame, describe the same experiment and must correct identically. This algorithm
    // dices the gauge volume, which is always in the lab frame, and requires every element to lie
    // inside the sample - so it is the case where the sample being in the wrong frame shows up
    // most directly.
    const auto rotation = SampleFrameEquivalence::rotationY(30.0);
    const double ownFrame = runGaugeCorrection("cuboidgauge_own", rotation, false);
    const double labFrame = runGaugeCorrection("cuboidgauge_lab", rotation, true);

    TS_ASSERT_DELTA(ownFrame, labFrame, 1e-9);
    // and the rotation actually mattered - otherwise the assertion above proves nothing
    const double unrotated = runGaugeCorrection("cuboidgauge_flat", Mantid::Kernel::Matrix<double>(3, 3, true), false);
    TS_ASSERT(std::abs(ownFrame - unrotated) > 1e-6);
  }

private:
  /// Correct a box sample held in the given frame and return the first attenuation factor. The box
  /// is comfortably larger than the gauge volume in every direction, so it still encloses it once
  /// rotated - otherwise the algorithm would fail rather than give a number to compare.
  double runGaugeCorrection(const std::string &name, const Mantid::Kernel::Matrix<double> &rotation, const bool baked) {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    const auto boxXML = ComponentCreationHelper::cuboidXML(0.025, 0.03, 0.02);
    if (baked) {
      SampleFrameEquivalence::setSampleInLabFrame(*ws, rotation, SampleFrameEquivalence::vanadium(), boxXML);
    } else {
      SampleFrameEquivalence::setSampleInOwnFrame(*ws, rotation, SampleFrameEquivalence::vanadium(), boxXML);
    }

    Mantid::Algorithms::CuboidGaugeVolumeAbsorption alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", name);
    // the gauge volume, in cm
    alg.setPropertyValue("SampleHeight", "2.3");
    alg.setPropertyValue("SampleWidth", "1.8");
    alg.setPropertyValue("SampleThickness", "1.5");
    alg.setPropertyValue("NumberOfWavelengthPoints", "3");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    auto result = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(name);
    const double value = result->y(0).front();
    Mantid::API::AnalysisDataService::Instance().remove(name);
    return value;
  }

  Mantid::Algorithms::CuboidGaugeVolumeAbsorption atten;
};
