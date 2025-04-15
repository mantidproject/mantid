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
#include "MantidAlgorithms/PaalmanPingsAbsorptionCorrection.h"
#include "MantidDataHandling/DefineGaugeVolume.h"
#include "MantidDataHandling/SetBeam.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"

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
    TS_ASSERT_DELTA(ws->readY(0)[0], expectedValues[0], 1e-7);
    TS_ASSERT_DELTA(ws->readY(1)[0], expectedValues[1], 1e-7);
    TS_ASSERT_DELTA(ws->readY(2)[0], expectedValues[2], 1e-7);
    TS_ASSERT_DELTA(ws->readY(3)[0], expectedValues[3], 1e-7);
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

    TS_ASSERT_DELTA(ass->readY(0)[0], absorptionCorrection_ass->readY(0)[0], 1e-7);
    TS_ASSERT_DELTA(ass->readY(1)[0], absorptionCorrection_ass->readY(1)[0], 1e-7);
    TS_ASSERT_DELTA(ass->readY(2)[0], absorptionCorrection_ass->readY(2)[0], 1e-7);
    TS_ASSERT_DELTA(ass->readY(3)[0], absorptionCorrection_ass->readY(3)[0], 1e-7);
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
    TS_ASSERT_DELTA(acc->readY(0)[0], absorptionCorrection_acc->readY(0)[0], 1e-7);
    TS_ASSERT_DELTA(acc->readY(1)[0], absorptionCorrection_acc->readY(1)[0], 1e-7);
    TS_ASSERT_DELTA(acc->readY(2)[0], absorptionCorrection_acc->readY(2)[0], 1e-7);
    TS_ASSERT_DELTA(acc->readY(3)[0], absorptionCorrection_acc->readY(3)[0], 1e-7);
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
    expectedValues = {0.0598354319, 0.0851820257, 0.1082668804, 0.0665718522};
    checkOutput(outWSgroup + "_ass", expectedValues);
    expectedValues = {0.0575985518, 0.0820397761, 0.1044455299, 0.0635469209};
    checkOutput(outWSgroup + "_assc", expectedValues);
    expectedValues = {0.4115165520, 0.4120483148, 0.4128312585, 0.4071160275};
    checkOutput(outWSgroup + "_acc", expectedValues);
    expectedValues = {0.1434953906, 0.2010954404, 0.2785605844, 0.1882925609911};
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
