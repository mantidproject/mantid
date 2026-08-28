// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/XrayAbsorptionCorrection.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/AttenuationProfile.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/VectorHelper.h"
#include "SampleFrameEquivalence.h"
#include <cxxtest/TestSuite.h>
#include <math.h>
#include <string>

using namespace Mantid;
using namespace Algorithms;
class XrayAbsorptionCorrectionTest : public CxxTest::TestSuite {

public:
  static XrayAbsorptionCorrectionTest *createSuite() { return new XrayAbsorptionCorrectionTest(); }
  static void destroySuite(XrayAbsorptionCorrectionTest *suite) { delete suite; }
  void test_CalculateDetectorPos() {
    TestableXrayAbsorptionCorrection alg;
    Kernel::V3D pos = alg.calculateDetectorPos(45, 10);
    Kernel::V3D correctPos = {0.1, 0, 0.1};

    TS_ASSERT_DELTA(pos[0], correctPos[0], 0.001);
    TS_ASSERT_DELTA(pos[1], correctPos[1], 0.001);
    TS_ASSERT_DELTA(pos[2], correctPos[2], 0.001);
  }
  void test_CalculateMuonPos() {
    TestableXrayAbsorptionCorrection alg;
    API::MatrixWorkspace_sptr muonProfile = createWorkspace(1.0);
    API::MatrixWorkspace_sptr inputWS = createWorkspaceWithDummyShape(20.0);
    std::vector<Kernel::V3D> muonPos = alg.calculateMuonPos(muonProfile, inputWS, 100.0);
    double z = 1.0;
    for (auto x : muonPos) {
      z -= 0.01;
      TS_ASSERT_DELTA(x[0], 0, 1.0e-6);
      TS_ASSERT_DELTA(x[1], 0, 1.0e-6);
      TS_ASSERT_DELTA(x[2], z, 1.0e-6);
    }
  }
  void test_NormaliseMounIntensity() {
    TestableXrayAbsorptionCorrection alg;
    API::MatrixWorkspace_sptr muonProfile = createWorkspace(1);
    std::vector<double> normalisedIntensity = alg.normaliseMuonIntensity(muonProfile->y(0));
    for (auto intensity : normalisedIntensity) {
      TS_ASSERT_DELTA(intensity, 0.1, 1.0e-6);
    }
  }

  void test_exec_with_no_shape() {
    API::MatrixWorkspace_sptr muonProfile = createWorkspace(1);
    API::MatrixWorkspace_sptr inputWS = createWorkspace(1);
    Algorithms::XrayAbsorptionCorrection algo;
    algo.initialize();
    algo.setProperty("InputWorkspace", inputWS);
    algo.setProperty("MuonImplantationProfile", muonProfile);
    algo.setProperty("OutputWorkspace", "outputWS");
    algo.setProperty("DetectorDistance", 10.0);
    algo.setProperty("DetectorAngle", 45.0);
    TS_ASSERT_THROWS(algo.execute(), const std::runtime_error &);
  }

  void test_exec_with_valid_shape() {
    API::MatrixWorkspace_sptr muonProfile = createWorkspace(100.0);
    auto &muonDepth = muonProfile->mutableX(0);
    for (size_t i = 0; i < muonDepth.size(); i++) {
      muonDepth[i] = 100.0;
    }
    API::MatrixWorkspace_sptr inputWS = createWorkspaceWithDummyShape(20.0);
    Algorithms::XrayAbsorptionCorrection algo;
    algo.initialize();
    algo.setProperty("InputWorkspace", inputWS);
    algo.setProperty("MuonImplantationProfile", muonProfile);
    algo.setProperty("OutputWorkspace", "outputWS");
    algo.setProperty("DetectorDistance", 1000.0);
    algo.setProperty("DetectorAngle", 45.0);
    algo.execute();
    API::MatrixWorkspace_sptr outputWS = algo.getProperty("OutputWorkspace");
    auto &yData = inputWS->mutableY(0);
    for (size_t i = 0; i < yData.size(); i++) {
      yData[i] = std::exp(-1);
    }
    auto comparison = algo.createChildAlgorithm("CompareWorkspaces");
    comparison->setProperty("Workspace1", inputWS);
    comparison->setProperty("Workspace2", "outputWS");
    comparison->setProperty("Tolerance", 1.0e-05);
    comparison->setProperty("ToleranceRelErr", true);
    comparison->execute();
    bool result = comparison->getProperty("Result");
    TS_ASSERT(result);
  }

  void test_both_ways_of_orienting_the_sample_agree() {
    // A sample in its own frame with the goniometer on the run, and the same sample already rotated
    // into the lab frame, describe the same experiment and must attenuate identically.
    const auto rotation = SampleFrameEquivalence::rotationY(30.0);
    const auto ownFrame = createWorkspaceWithPlate(rotation, false);
    const auto labFrame = createWorkspaceWithPlate(rotation, true);

    const auto ownY = runXrayCorrection(ownFrame)->y(0);
    const auto labY = runXrayCorrection(labFrame)->y(0);

    TS_ASSERT_EQUALS(ownY.size(), labY.size());
    for (size_t i = 0; i < ownY.size(); ++i) {
      TS_ASSERT_DELTA(ownY[i], labY[i], 1.0e-10);
    }
    // and the rotation actually mattered - otherwise the assertion above proves nothing
    const auto unrotatedY = runXrayCorrection(createWorkspaceWithPlate(rotationIdentity(), false))->y(0);
    TS_ASSERT(std::abs(ownY[0] - unrotatedY[0]) > 1.0e-6);
  }

  void test_exec_with_non_valid_shape() {
    API::MatrixWorkspace_sptr muonProfile = createWorkspace(100);
    API::MatrixWorkspace_sptr inputWS = createWorkspaceWithDummyShape(20, false);
    Algorithms::XrayAbsorptionCorrection algo;
    algo.initialize();
    algo.setProperty("InputWorkspace", inputWS);
    algo.setProperty("MuonImplantationProfile", muonProfile);
    algo.setProperty("OutputWorkspace", "outputWS");
    algo.setProperty("DetectorDistance", 10.0);
    algo.setProperty("DetectorAngle", 45.0);
    TS_ASSERT_THROWS(algo.execute(), std::runtime_error &);
  }

private:
  static Kernel::Matrix<double> rotationIdentity() { return Kernel::Matrix<double>(3, 3, true); }

  /// A plate sample, either in its own frame with the rotation on the run, or already baked into
  /// the lab frame. Both carry the same goniometer, so only the shape distinguishes them.
  API::MatrixWorkspace_sptr createWorkspaceWithPlate(const Kernel::Matrix<double> &rotation, const bool baked) {
    API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create1DWorkspaceConstant(10, 20.0, 0.0, true);

    Kernel::Material sampleMaterial;
    Kernel::AttenuationProfile sampleProfile;
    sampleProfile.setAttenuationCoefficient(1.0, 1.0);
    sampleProfile.setAttenuationCoefficient(10.0, 1.0);
    sampleProfile.setAttenuationCoefficient(100.0, 1.0);
    sampleProfile.setAttenuationCoefficient(1000.0, 1.0);
    sampleMaterial.setXRayAttenuationProfile(sampleProfile);

    if (baked) {
      SampleFrameEquivalence::setSampleInLabFrame(*ws, rotation, sampleMaterial);
    } else {
      SampleFrameEquivalence::setSampleInOwnFrame(*ws, rotation, sampleMaterial);
    }
    return ws;
  }

  API::MatrixWorkspace_sptr runXrayCorrection(const API::MatrixWorkspace_sptr &inputWS) {
    API::MatrixWorkspace_sptr muonProfile = createWorkspace(100.0);
    auto &muonDepth = muonProfile->mutableX(0);
    for (size_t i = 0; i < muonDepth.size(); i++) {
      muonDepth[i] = 0.1;
    }
    Algorithms::XrayAbsorptionCorrection algo;
    algo.initialize();
    algo.setChild(true);
    algo.setProperty("InputWorkspace", inputWS);
    algo.setProperty("MuonImplantationProfile", muonProfile);
    algo.setProperty("OutputWorkspace", "outputWS");
    algo.setProperty("DetectorDistance", 1000.0);
    algo.setProperty("DetectorAngle", 45.0);
    algo.execute();
    return algo.getProperty("OutputWorkspace");
  }

  API::MatrixWorkspace_sptr createWorkspaceWithDummyShape(double value, bool hasXrayAttenuationProfile = true) {
    API::MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create1DWorkspaceConstant(10, value, 0.0, true);

    Kernel::Material sampleMaterial;
    Kernel::AttenuationProfile sampleProfile;
    sampleProfile.setAttenuationCoefficient(1.0, 1.0);
    sampleProfile.setAttenuationCoefficient(10.0, 1.0);
    sampleProfile.setAttenuationCoefficient(100.0, 1.0);
    sampleProfile.setAttenuationCoefficient(1000.0, 1.0);
    sampleMaterial.setXRayAttenuationProfile(sampleProfile);

    auto shape = ComponentCreationHelper::createSphere(1.0, {0.0, 0.0, 0.0}, "sample-shape");

    if (hasXrayAttenuationProfile) {
      shape->setMaterial(sampleMaterial);
    }
    inputWS->mutableSample().setShape(shape);
    return inputWS;
  }

  API::MatrixWorkspace_sptr createWorkspace(double value) {

    API::MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create1DWorkspaceConstant(10, value, 0.0, true);

    return inputWS;
  }

private:
  class TestableXrayAbsorptionCorrection : public XrayAbsorptionCorrection {
    friend class XrayAbsorptionCorrectionTest;
  };
};
