// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <array>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/PaalmanPingsAbsorptionCorrection.h"
#include "MantidDataHandling/DefineGaugeVolume.h"
#include "MantidDataHandling/SetBeam.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/UnitFactory.h"
#include "SampleFrameEquivalence.h"

using Mantid::Algorithms::PaalmanPingsAbsorptionCorrection;
using Mantid::API::AlgorithmManager;
using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace_sptr;

class PaalmanPingsAbsorptionCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PaalmanPingsAbsorptionCorrectionTest *createSuite() { return new PaalmanPingsAbsorptionCorrectionTest(); }
  static void destroySuite(PaalmanPingsAbsorptionCorrectionTest *suite) { delete suite; }

  void createWorkspace(const std::string wsName) {
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceBinned(4, 1, 1.7981, 0.0002);

    auto testInst = ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(
        {2., 2., 2., 2.}, {10. * M_PI / 180, 90. * M_PI / 180, 170. * M_PI / 180, 90 * M_PI / 180},
        {0., 0., 0., 45 * M_PI / 180});
    testWS->setInstrument(testInst);
    testWS->rebuildSpectraMapping();
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    AnalysisDataService::Instance().addOrReplace(wsName, testWS);

    auto setSampleAlg = AlgorithmManager::Instance().createUnmanaged("SetSample");
    setSampleAlg->setRethrows(true);
    setSampleAlg->initialize();
    setSampleAlg->setPropertyValue("InputWorkspace", wsName);
    setSampleAlg->setPropertyValue("Material",
                                   R"({"ChemicalFormula": "La-(B11)5.94-(B10)0.06", "SampleNumberDensity": 0.1})");
    setSampleAlg->setPropertyValue("Geometry",
                                   R"({"Shape": "Cylinder", "Height": 5.68, "Radius": 0.295, "Center": [0., 0., 0.]})");
    setSampleAlg->setPropertyValue("ContainerMaterial", R"({"ChemicalFormula":"V", "SampleNumberDensity": 0.0721})");
    setSampleAlg->setPropertyValue(
        "ContainerGeometry",
        R"({"Shape": "HollowCylinder", "Height": 5.68, "InnerRadius": 0.295, "OuterRadius": 0.315, "Center": [0., 0., 0.]})");
    TS_ASSERT_THROWS_NOTHING(setSampleAlg->execute());
  }

  void checkOutput(const std::string workspaceName, const std::array<double, 4> expectedValues) {
    Mantid::API::MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(workspaceName)));
    TS_ASSERT_DELTA(ws->y(0)[0], expectedValues[0], 1e-7);
    TS_ASSERT_DELTA(ws->y(1)[0], expectedValues[1], 1e-7);
    TS_ASSERT_DELTA(ws->y(2)[0], expectedValues[2], 1e-7);
    TS_ASSERT_DELTA(ws->y(3)[0], expectedValues[3], 1e-7);
  }

  void checkAbsorptionCorrectionSample(const std::string inputWS, const std::string outputWS) {
    // Compare to the AbsorptionCorrection algorithm
    // A_s,s should match the ScatterFrom='Sample'

    auto absorptionCorrectionAlg = AlgorithmManager::Instance().createUnmanaged("AbsorptionCorrection");
    absorptionCorrectionAlg->setRethrows(true);
    absorptionCorrectionAlg->initialize();
    absorptionCorrectionAlg->setPropertyValue("InputWorkspace", inputWS);
    absorptionCorrectionAlg->setProperty("ElementSize", 0.1);
    absorptionCorrectionAlg->setPropertyValue("OutputWorkspace", "absorptionCorrection_ass");
    TS_ASSERT_THROWS_NOTHING(absorptionCorrectionAlg->execute());

    Mantid::API::MatrixWorkspace_sptr absorptionCorrection_ass;
    TS_ASSERT_THROWS_NOTHING(absorptionCorrection_ass = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve("absorptionCorrection_ass")));

    Mantid::API::MatrixWorkspace_sptr ass;
    TS_ASSERT_THROWS_NOTHING(ass = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outputWS + "_ass")));

    TS_ASSERT_DELTA(ass->y(0)[0], absorptionCorrection_ass->y(0)[0], 1e-7);
    TS_ASSERT_DELTA(ass->y(1)[0], absorptionCorrection_ass->y(1)[0], 1e-7);
    TS_ASSERT_DELTA(ass->y(2)[0], absorptionCorrection_ass->y(2)[0], 1e-7);
    TS_ASSERT_DELTA(ass->y(3)[0], absorptionCorrection_ass->y(3)[0], 1e-7);
  }

  void checkAbsorptionCorrectionContainer(const std::string inputWS, const std::string outputWS) {
    // Compare to the AbsorptionCorrection algorithm
    // A_c,c should match ScatterFrom='Container'

    auto absorptionCorrectionAlg = AlgorithmManager::Instance().createUnmanaged("AbsorptionCorrection");
    absorptionCorrectionAlg->setRethrows(true);
    absorptionCorrectionAlg->initialize();
    absorptionCorrectionAlg->setPropertyValue("InputWorkspace", inputWS);
    absorptionCorrectionAlg->setProperty("ElementSize", 0.1);
    absorptionCorrectionAlg->setPropertyValue("ScatterFrom", "Container");
    absorptionCorrectionAlg->setPropertyValue("OutputWorkspace", "absorptionCorrection_acc");
    TS_ASSERT_THROWS_NOTHING(absorptionCorrectionAlg->execute());
    Mantid::API::MatrixWorkspace_sptr absorptionCorrection_acc;
    TS_ASSERT_THROWS_NOTHING(absorptionCorrection_acc = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve("absorptionCorrection_acc")));
    Mantid::API::MatrixWorkspace_sptr acc;
    TS_ASSERT_THROWS_NOTHING(acc = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outputWS + "_acc")));
    TS_ASSERT_DELTA(acc->y(0)[0], absorptionCorrection_acc->y(0)[0], 1e-7);
    TS_ASSERT_DELTA(acc->y(1)[0], absorptionCorrection_acc->y(1)[0], 1e-7);
    TS_ASSERT_DELTA(acc->y(2)[0], absorptionCorrection_acc->y(2)[0], 1e-7);
    TS_ASSERT_DELTA(acc->y(3)[0], absorptionCorrection_acc->y(3)[0], 1e-7);
  }

  /// A flat plate in a matching holder. Unlike the LaB6 cylinder, tilting a plate about y changes
  /// how much material the beam crosses, so the sample's orientation is visible in the answer.
  void createPlateWorkspace(const std::string &wsName) {
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceBinned(4, 1, 1.7981, 0.0002);
    auto testInst = ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(
        {2., 2., 2., 2.}, {10. * M_PI / 180, 90. * M_PI / 180, 170. * M_PI / 180, 90 * M_PI / 180},
        {0., 0., 0., 45 * M_PI / 180});
    testWS->setInstrument(testInst);
    testWS->rebuildSpectraMapping();
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    AnalysisDataService::Instance().addOrReplace(wsName, testWS);

    auto setSampleAlg = AlgorithmManager::Instance().createUnmanaged("SetSample");
    setSampleAlg->setRethrows(true);
    setSampleAlg->initialize();
    setSampleAlg->setPropertyValue("InputWorkspace", wsName);
    setSampleAlg->setPropertyValue("Material",
                                   R"({"ChemicalFormula": "La-(B11)5.94-(B10)0.06", "SampleNumberDensity": 0.1})");
    setSampleAlg->setPropertyValue(
        "Geometry", R"({"Shape": "FlatPlate", "Height": 2.0, "Width": 2.0, "Thick": 0.2, "Center": [0., 0., 0.]})");
    setSampleAlg->setPropertyValue("ContainerMaterial", R"({"ChemicalFormula":"V", "SampleNumberDensity": 0.0721})");
    setSampleAlg->setPropertyValue("ContainerGeometry",
                                   R"({"Shape": "FlatPlateHolder", "Height": 2.0, "Width": 2.0, "Thick": 0.2,)"
                                   R"( "FrontThick": 0.1, "BackThick": 0.1, "Center": [0., 0., 0.]})");
    TS_ASSERT_THROWS_NOTHING(setSampleAlg->execute());
  }

  /// Run the correction and return the four sample self-attenuation factors.
  std::array<double, 4> runPlateCorrection(const std::string &wsName, const Mantid::Kernel::Matrix<double> &rotation,
                                           const bool baked) {
    createPlateWorkspace(wsName);
    auto ws = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
    ws->mutableRun().mutableGoniometer().setR(rotation);

    if (baked) {
      // Bake the rotation into the shape the way a user would - through CopySample onto a
      // workspace already carrying the goniometer. Deliberately the real algorithm, so this test
      // cannot pass by agreeing with the helper it is checking.
      const std::string srcName = wsName + "_src";
      createPlateWorkspace(srcName);
      auto copyAlg = AlgorithmManager::Instance().createUnmanaged("CopySample");
      copyAlg->setRethrows(true);
      copyAlg->initialize();
      copyAlg->setPropertyValue("InputWorkspace", srcName);
      copyAlg->setPropertyValue("OutputWorkspace", wsName);
      copyAlg->setProperty("CopyName", false);
      copyAlg->setProperty("CopyEnvironment", false);
      copyAlg->setProperty("CopyLattice", false);
      TS_ASSERT_THROWS_NOTHING(copyAlg->execute());
    }

    auto alg = AlgorithmManager::Instance().createUnmanaged("PaalmanPingsAbsorptionCorrection");
    alg->setRethrows(true);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", wsName);
    alg->setProperty("ElementSize", 0.4);
    alg->setPropertyValue("OutputWorkspace", wsName + "_out");
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto ass = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(wsName + "_out_ass"));
    return {ass->y(0)[0], ass->y(1)[0], ass->y(2)[0], ass->y(3)[0]};
  }

  void test_both_ways_of_orienting_the_sample_agree() {
    // A sample in its own frame with the goniometer on the run, and the same sample already rotated
    // into the lab frame by CopySample, describe the same experiment and must attenuate identically.
    const auto rotation = SampleFrameEquivalence::rotationY(30.0);
    const auto ownFrame = runPlateCorrection("pp_own", rotation, false);
    const auto labFrame = runPlateCorrection("pp_lab", rotation, true);

    for (size_t i = 0; i < 4; ++i) {
      TS_ASSERT_DELTA(ownFrame[i], labFrame[i], 1e-9);
    }
    // and the rotation actually mattered - otherwise the assertions above prove nothing
    const auto unrotated = runPlateCorrection("pp_flat", Mantid::Kernel::Matrix<double>(3, 3, true), false);
    TS_ASSERT(std::abs(ownFrame[0] - unrotated[0]) > 1e-6);
  }

  void test_missing_container() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    PaalmanPingsAbsorptionCorrection alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    std::string outWSgroup("absorption");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSgroup));
    alg.setRethrows(true);
    // Error missing container definition
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_LaB6() {
    // create the input workspace
    std::string wsname("PaalmanPingsAbsorptionCorrectionTest");
    createWorkspace(wsname);

    PaalmanPingsAbsorptionCorrection alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ElementSize", 0.1));
    std::string outWSgroup("absorption");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSgroup));
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    std::array<double, 4> expectedValues;
    expectedValues = {0.1466219, 0.1977505, 0.2517314, 0.1622546};
    checkOutput(outWSgroup + "_ass", expectedValues);
    expectedValues = {0.1406871, 0.1903367, 0.2422601, 0.1550581};
    checkOutput(outWSgroup + "_assc", expectedValues);
    expectedValues = {0.9429243, 0.9427054, 0.9434231, 0.9324084};
    checkOutput(outWSgroup + "_acc", expectedValues);
    expectedValues = {0.3251095, 0.4218324, 0.5778520, 0.4014179911};
    checkOutput(outWSgroup + "_acsc", expectedValues);

    checkAbsorptionCorrectionSample(wsname, outWSgroup);
    checkAbsorptionCorrectionContainer(wsname, outWSgroup);
  }

  void test_determineGaugeVolumeFromSetBeam() {
    std::string wsname("DetermineGaugeVolumeTest");
    createWorkspace(wsname);

    Mantid::DataHandling::SetBeam sbAlg;
    sbAlg.initialize();
    using Mantid::Kernel::PropertyManager;
    using DoubleProperty = Mantid::Kernel::PropertyWithValue<double>;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "Slit"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Width", 3.0), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 3.0), "");
    TS_ASSERT_THROWS_NOTHING(sbAlg.setProperty("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(sbAlg.setProperty("Geometry", props));
    sbAlg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(sbAlg.execute());
    TS_ASSERT(sbAlg.isExecuted());

    PaalmanPingsAbsorptionCorrection alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ElementSize", 0.1));
    std::string outWSgroup("gv_absorption");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSgroup));
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    std::array<double, 4> expectedValues;
    expectedValues = {0.1468305595, 0.1972436849, 0.2506198619, 0.1567429952};
    checkOutput(outWSgroup + "_ass", expectedValues);

    expectedValues = {0.1408542645, 0.1898304692, 0.2411614445, 0.1495443008};
    checkOutput(outWSgroup + "_assc", expectedValues);

    expectedValues = {0.9440690762, 0.9444321060, 0.9445980596, 0.9334543080};
    checkOutput(outWSgroup + "_acc", expectedValues);

    expectedValues = {0.3225373817, 0.4193655242, 0.5768073083, 0.3923031636};
    checkOutput(outWSgroup + "_acsc", expectedValues);

    std::string gaugeVolumeXML = "<cuboid id=\"some-cuboid\">  <width val=\"0.0059\" />  <height val=\"0.03\"  />  "
                                 "<depth  val=\"0.0059\" />  <centre x=\"0.0\" y=\"0.0\" z=\"0.0\"  /></cuboid>";
    Mantid::DataHandling::DefineGaugeVolume gauge;
    gauge.initialize();
    gauge.setRethrows(true);
    gauge.setPropertyValue("Workspace", wsname);
    gauge.setPropertyValue("ShapeXML", gaugeVolumeXML);
    gauge.execute();
    checkAbsorptionCorrectionSample(wsname, outWSgroup);

    gaugeVolumeXML = "<cuboid id=\"some-cuboid\">  <width val=\"0.0063\" />  <height val=\"0.03\"  />  <depth  "
                     "val=\"0.0063\" />  <centre x=\"0.0\" y=\"0.0\" z=\"0.0\"  /></cuboid>";
    gauge.setPropertyValue("ShapeXML", gaugeVolumeXML);
    gauge.execute();
    checkAbsorptionCorrectionContainer(wsname, outWSgroup);
  }
};
