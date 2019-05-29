// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDALGORITHMS_PREPROCESS_DETECTORS2MD_TEST_H_
#define MDALGORITHMS_PREPROCESS_DETECTORS2MD_TEST_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidMDAlgorithms/PreprocessDetectorsToMD.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::MDAlgorithms;

class PrepcocessDetectorsToMDTestHelper : public PreprocessDetectorsToMD {
public:
  boost::shared_ptr<DataObjects::TableWorkspace>
  createTableWorkspace(const API::MatrixWorkspace_const_sptr &inputWS) {
    return PreprocessDetectorsToMD::createTableWorkspace(inputWS);
  }
  void processDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,
                                 DataObjects::TableWorkspace_sptr &targWS) {
    PreprocessDetectorsToMD::processDetectorsPositions(inputWS, targWS);
  }
  void
  buildFakeDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,
                              DataObjects::TableWorkspace_sptr &targWS) {
    PreprocessDetectorsToMD::buildFakeDetectorsPositions(inputWS, targWS);
  }

  PrepcocessDetectorsToMDTestHelper() : PreprocessDetectorsToMD() {
    PreprocessDetectorsToMD::initialize();
  }
};

// Test is transformed from ConvetToQ3DdE but actually tests some aspects of
// ConvertToMD algorithm.
class PreprocessDetectorsToMDTest : public CxxTest::TestSuite {
  std::unique_ptr<PrepcocessDetectorsToMDTestHelper> pAlg;
  API::MatrixWorkspace_sptr ws2D;
  boost::shared_ptr<DataObjects::TableWorkspace> tws;

public:
  static PreprocessDetectorsToMDTest *createSuite() {
    return new PreprocessDetectorsToMDTest();
  }
  static void destroySuite(PreprocessDetectorsToMDTest *suite) { delete suite; }

  void testCreateTarget() {
    TS_ASSERT_THROWS_NOTHING(tws = pAlg->createTableWorkspace(ws2D));

    TS_ASSERT(tws);

    TS_ASSERT_EQUALS(4, tws->rowCount());
    TS_ASSERT_EQUALS(8, tws->columnCount());
  }

  void testPreprocessDetectors() {

    TS_ASSERT_THROWS_NOTHING(pAlg->processDetectorsPositions(ws2D, tws));

    size_t nVal = tws->rowCount();

    const std::vector<size_t> &spec2detMap =
        tws->getColVector<size_t>("spec2detMap");
    for (size_t i = 0; i < nVal; i++) {
      TS_ASSERT_EQUALS(i, spec2detMap[i]);
    }

    double L1(0);
    uint32_t nDet(0);
    std::string InstrName;
    bool fakeDetectrors(false);
    TS_ASSERT_THROWS_NOTHING(
        nDet = tws->getLogs()->getPropertyValueAsType<uint32_t>(
            "ActualDetectorsNum"));
    TS_ASSERT_THROWS_NOTHING(
        L1 = tws->getLogs()->getPropertyValueAsType<double>("L1"));
    TS_ASSERT_THROWS_NOTHING(
        InstrName = tws->getLogs()->getPropertyValueAsType<std::string>(
            "InstrumentName"));
    TS_ASSERT_THROWS_NOTHING(
        fakeDetectrors =
            tws->getLogs()->getPropertyValueAsType<bool>("FakeDetectors"));

    TS_ASSERT_DELTA(10, L1, 1.e-11);
    TS_ASSERT_EQUALS(4, nDet);
    TS_ASSERT_EQUALS("basic", InstrName);
    TS_ASSERT(!fakeDetectrors);
  }

  void testFakeDetectors() {

    TS_ASSERT_THROWS_NOTHING(pAlg->buildFakeDetectorsPositions(ws2D, tws));

    size_t nVal = tws->rowCount();

    auto &spec2detMap = tws->getColVector<size_t>("spec2detMap");
    auto &detId = tws->getColVector<int32_t>("DetectorID");
    auto &detIDMap = tws->getColVector<size_t>("detIDMap");
    auto &L2 = tws->getColVector<double>("L2");
    auto &TwoTheta = tws->getColVector<double>("TwoTheta");
    auto &Azimuthal = tws->getColVector<double>("Azimuthal");
    auto &detDir = tws->getColVector<Kernel::V3D>("DetDirections");

    TS_ASSERT_EQUALS(nVal, detDir.size());

    for (size_t i = 0; i < nVal; i++) {
      TS_ASSERT_EQUALS(i, spec2detMap[i]);
      TS_ASSERT_EQUALS(i, detId[i]);
      TS_ASSERT_EQUALS(i, detIDMap[i]);
      TS_ASSERT_DELTA(1, L2[i], 1.e-11);
      TS_ASSERT_DELTA(0, TwoTheta[i], 1.e-11);
      TS_ASSERT_DELTA(0, Azimuthal[i], 1.e-11);
    }

    double L1(0);
    uint32_t nDet(0);
    std::string InstrName;
    bool fakeDetectrors(false);
    TS_ASSERT_THROWS_NOTHING(
        nDet = tws->getLogs()->getPropertyValueAsType<uint32_t>(
            "ActualDetectorsNum"));
    TS_ASSERT_THROWS_NOTHING(
        L1 = tws->getLogs()->getPropertyValueAsType<double>("L1"));
    TS_ASSERT_THROWS_NOTHING(
        InstrName = tws->getLogs()->getPropertyValueAsType<std::string>(
            "InstrumentName"));
    TS_ASSERT_THROWS_NOTHING(
        fakeDetectrors =
            tws->getLogs()->getPropertyValueAsType<bool>("FakeDetectors"));

    TS_ASSERT_DELTA(1, L1, 1.e-11);
    TS_ASSERT_EQUALS(4, nDet);
    TS_ASSERT_EQUALS("FakeInstrument", InstrName);
    TS_ASSERT(fakeDetectrors);
  }

  void testTheAlg() {
    auto pAlg =
        Mantid::Kernel::make_unique<PrepcocessDetectorsToMDTestHelper>();

    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("InputWorkspace", "testMatrWS"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "PreprocDetectorsWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("UpdateMasksInfo", "1"));

    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    TSM_ASSERT("Should be successful ", pAlg->isExecuted());

    API::Workspace_sptr wsOut =
        API::AnalysisDataService::Instance().retrieve("PreprocDetectorsWS");
    TSM_ASSERT("can not retrieve table worksapce from analysis data service ",
               wsOut);
    DataObjects::TableWorkspace_sptr tws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(wsOut);
    TSM_ASSERT("can not interpet the workspace as table workspace", tws);

    double L1(0);
    uint32_t nDet(0);
    std::string InstrName;
    bool fakeDetectrors(false);
    TS_ASSERT_THROWS_NOTHING(
        nDet = tws->getLogs()->getPropertyValueAsType<uint32_t>(
            "ActualDetectorsNum"));
    TS_ASSERT_THROWS_NOTHING(
        L1 = tws->getLogs()->getPropertyValueAsType<double>("L1"));
    TS_ASSERT_THROWS_NOTHING(
        InstrName = tws->getLogs()->getPropertyValueAsType<std::string>(
            "InstrumentName"));
    TS_ASSERT_THROWS_NOTHING(
        fakeDetectrors =
            tws->getLogs()->getPropertyValueAsType<bool>("FakeDetectors"));

    TS_ASSERT_DELTA(10, L1, 1.e-11);
    TS_ASSERT_EQUALS(4, nDet);
    TS_ASSERT_EQUALS("basic", InstrName);
    TS_ASSERT(!fakeDetectrors);
  }

  void testCreateWSWithEfixed() {
    auto pAlg =
        Mantid::Kernel::make_unique<PrepcocessDetectorsToMDTestHelper>();

    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("InputWorkspace", "testMatrWS"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "PreprocDetectorsWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("GetEFixed", "1"));

    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    TSM_ASSERT("Should be successful ", pAlg->isExecuted());

    API::Workspace_sptr wsOut =
        API::AnalysisDataService::Instance().retrieve("PreprocDetectorsWS");
    TSM_ASSERT("can not retrieve table worksapce from analysis data service ",
               wsOut);
    DataObjects::TableWorkspace_sptr tws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(wsOut);
    TSM_ASSERT("can not interpet the workspace as table workspace", tws);

    auto &Efixed = tws->getColVector<float>("eFixed");
    for (size_t i = 0; i < Efixed.size(); i++) {
      TS_ASSERT_DELTA(13.f, Efixed[i], 1.e-6);
    }
  }

  void testUpdateMasks() {
    auto pAlg =
        Mantid::Kernel::make_unique<PrepcocessDetectorsToMDTestHelper>();
    // do first run which generates first masks
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("InputWorkspace", "testMatrWS"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "PreprocDetectorsWSMasks"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("GetMaskState", "1"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("UpdateMasksInfo", "1"));

    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    TSM_ASSERT("Should be successful ", pAlg->isExecuted());

    API::Workspace_sptr wsOut = API::AnalysisDataService::Instance().retrieve(
        "PreprocDetectorsWSMasks");
    TSM_ASSERT("can not retrieve table worksapce from analysis data service ",
               wsOut);
    DataObjects::TableWorkspace_sptr tws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(wsOut);
    TSM_ASSERT("can not interpet the workspace as table workspace", tws);

    auto &maskCol = tws->getColVector<int>("detMask");
    for (size_t i = 0; i < maskCol.size(); i++) {
      TS_ASSERT_EQUALS(0, maskCol[i]);
    }
    // now mask a detector and check if masks are updated;
    auto inputWS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
        API::AnalysisDataService::Instance().retrieve("testMatrWS"));
    const size_t nRows = inputWS->getNumberHistograms();

    // Now mask all detectors in the workspace
    auto &spectrumInfo = inputWS->mutableSpectrumInfo();
    for (size_t i = 0; i < nRows; i++) {
      if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
        continue;
      inputWS->getSpectrum(i).clearData();
      spectrumInfo.setMasked(i, true);
    }

    // let's retrieve masks now

    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    TSM_ASSERT("Should be successful ", pAlg->isExecuted());

    // old table ws sp still should be valid amd mask column should still be the
    // same
    // auto &maskCol     = tws->getColVector<int>("detMask");
    // all detectors should be masked now
    for (size_t i = 0; i < maskCol.size(); i++) {
      TS_ASSERT_EQUALS(1, maskCol[i]);
    }

    API::AnalysisDataService::Instance().remove("PreprocDetectorsWSMasks");
  }
  void testNoMasksColumnTrhows() {
    auto pAlg =
        Mantid::Kernel::make_unique<PrepcocessDetectorsToMDTestHelper>();
    // do first run which generates first masks
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("InputWorkspace", "testMatrWS"));
    TS_ASSERT_THROWS_NOTHING(
        pAlg->setPropertyValue("OutputWorkspace", "PreprocDetectorsWSMasks"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("GetMaskState", "0"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("UpdateMasksInfo", "0"));

    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    TSM_ASSERT("Should be successful ", pAlg->isExecuted());

    API::Workspace_sptr wsOut = API::AnalysisDataService::Instance().retrieve(
        "PreprocDetectorsWSMasks");
    TSM_ASSERT("can not retrieve table worksapce from analysis data service ",
               wsOut);
    DataObjects::TableWorkspace_sptr tws =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(wsOut);
    TSM_ASSERT("can not interpet the workspace as table workspace", tws);

    TSM_ASSERT_THROWS("No such column", tws->getColVector<int>("detMask"),
                      const std::runtime_error &);

    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("GetMaskState", "1"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("UpdateMasksInfo", "1"));
  }

  PreprocessDetectorsToMDTest() {
    pAlg = Mantid::Kernel::make_unique<PrepcocessDetectorsToMDTestHelper>();

    ws2D = WorkspaceCreationHelper::
        createProcessedWorkspaceWithCylComplexInstrument(4, 10, true);
    // rotate the crystal by twenty degrees back;
    ws2D->mutableRun().mutableGoniometer().setRotationAngle(0, 20);
    // add workspace energy
    ws2D->mutableRun().addProperty("Ei", 13., "meV", true);

    API::AnalysisDataService::Instance().addOrReplace("testMatrWS", ws2D);
  }
};

#endif
