// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <string>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class ConjoinWorkspacesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConjoinWorkspacesTest *createSuite() { return new ConjoinWorkspacesTest(); }
  static void destroySuite(ConjoinWorkspacesTest *suite) { delete suite; }

  ConjoinWorkspacesTest() : ws1Name("ConjoinWorkspacesTest_grp1"), ws2Name("ConjoinWorkspacesTest_grp2") {}

  MatrixWorkspace_sptr getWSFromADS(const std::string &wsName) {
    auto out = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
    TS_ASSERT(out);
    return out;
  }

  void setupWS() {
    IAlgorithm *loader;
    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", "top");
    loader->setPropertyValue("SpectrumMin", "1");
    loader->setPropertyValue("SpectrumMax", "10");
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());
    delete loader;

    loader = new Mantid::DataHandling::LoadRaw3;
    loader->initialize();
    loader->setPropertyValue("Filename", "OSI11886.raw");
    loader->setPropertyValue("OutputWorkspace", "bottom");
    loader->setPropertyValue("SpectrumMin", "11");
    loader->setPropertyValue("SpectrumMax", "25");
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());
    delete loader;
  }

  void testTheBasics() {
    ConjoinWorkspaces conj;
    TS_ASSERT_EQUALS(conj.name(), "ConjoinWorkspaces");
    TS_ASSERT_EQUALS(conj.version(), 1);
    TS_ASSERT_THROWS_NOTHING(conj.initialize());
    TS_ASSERT(conj.isInitialized());
  }

  //----------------------------------------------------------------------------------------------
  void testExec() {
    setupWS();

    ConjoinWorkspaces conj;
    if (!conj.isInitialized())
      conj.initialize();

    // Get the two input workspaces for later
    MatrixWorkspace_sptr in1 =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("top"));
    MatrixWorkspace_sptr in2 =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("bottom"));

    // Mask a spectrum and check it is carried over
    const size_t maskTop(5), maskBottom(10);
    in1->getSpectrum(maskTop).clearData();
    in2->getSpectrum(maskBottom).clearData();
    in1->mutableSpectrumInfo().setMasked(maskTop, true);
    in2->mutableSpectrumInfo().setMasked(maskBottom, true);

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS(conj.execute(), const std::runtime_error &);
    TS_ASSERT(!conj.isExecuted());

    // Check it fails if input overlap
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace1", "top"));
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace2", "top"));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckMatchingBins", false));
    TS_ASSERT_THROWS_NOTHING(conj.execute());
    TS_ASSERT(!conj.isExecuted());

    // Now it should succeed
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace1", "top"));
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace2", "bottom"));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckMatchingBins", false));
    TS_ASSERT_THROWS_NOTHING(conj.execute());
    TS_ASSERT(conj.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = getWSFromADS("top"););
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 25);
    // Check a few values
    TS_ASSERT_EQUALS(output->readX(0)[0], in1->readX(0)[0]);
    TS_ASSERT_EQUALS(output->readX(15)[444], in2->readX(5)[444]);
    TS_ASSERT_EQUALS(output->readY(3)[99], in1->readY(3)[99]);
    TS_ASSERT_EQUALS(output->readE(7)[700], in1->readE(7)[700]);
    TS_ASSERT_EQUALS(output->readY(19)[55], in2->readY(9)[55]);
    TS_ASSERT_EQUALS(output->readE(10)[321], in2->readE(0)[321]);
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(5), in1->getAxis(1)->spectraNo(5));
    TS_ASSERT_EQUALS(output->getAxis(1)->spectraNo(12), in2->getAxis(1)->spectraNo(2));

    // Check masking
    const auto &spectrumInfo = output->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(maskTop), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(10 + maskBottom), true);

    // Check that 2nd input workspace no longer exists
    TS_ASSERT_THROWS(AnalysisDataService::Instance().retrieve("bottom"), const Exception::NotFoundError &);

    // Check that th workspace has the correct number of history entries
    TS_ASSERT_EQUALS(output->getHistory().size(), 3);
  }

  //----------------------------------------------------------------------------------------------
  void testExecMismatchedWorkspaces() {
    MatrixWorkspace_sptr ews = WorkspaceCreationHelper::createEventWorkspace(10, 10);

    // Check it fails if input overlap
    ConjoinWorkspaces conj;
    conj.initialize();
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("InputWorkspace1", ews));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("InputWorkspace2", ews));
    conj.execute();
    TS_ASSERT(!conj.isExecuted());

    // Check it fails if mixing event workspaces and workspace 2Ds
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("InputWorkspace1", ews));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("InputWorkspace2", WorkspaceCreationHelper::create2DWorkspace(10, 10)));
    conj.execute();
    TS_ASSERT(!conj.isExecuted());
  }

  void testMismatchedEventWorkspace() {
    setupMismatchedWorkspace("testMismatchedEventWorkspace1", 0, 2, "100,200,700");
    setupMismatchedWorkspace("testMismatchedEventWorkspace2", 3, 5, "100,200,1000");

    ConjoinWorkspaces conj;
    conj.initialize();
    conj.setProperty("CheckMatchingBins", false);
    conj.setRethrows(true);

    conj.setProperty("InputWorkspace1", "testMismatchedEventWorkspace1");
    conj.setProperty("InputWorkspace2", "testMismatchedEventWorkspace2");

    TS_ASSERT_THROWS(conj.execute(), const std::invalid_argument &);
    TS_ASSERT(!conj.isExecuted());
  }

  void testCheckMatchingBinsError() {
    MatrixWorkspace_sptr ws1, ws2;
    ws1 = WorkspaceCreationHelper::createEventWorkspace(10, 5);
    ws2 = WorkspaceCreationHelper::createEventWorkspace(10, 10);

    ConjoinWorkspaces conj;
    conj.initialize();
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("InputWorkspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("InputWorkspace2", ws2));
    conj.setRethrows(true);

    try {
      conj.execute();
      TS_FAIL("Expected an exception but none was thrown.");
    } catch (const std::invalid_argument &e) {
      std::string expectedMessage = "The bins do not match in the input workspaces. "
                                    "Consider using RebinToWorkspace to preprocess "
                                    "the workspaces before conjoining them.";
      TS_ASSERT_EQUALS(std::string(e.what()), expectedMessage);
      TS_ASSERT(!conj.isExecuted());
    }
  }

  void testDoCheckForOverlap() {
    MatrixWorkspace_sptr ws1, ws2;
    int numPixels = 10;
    int numBins = 20;
    ws1 = WorkspaceCreationHelper::createEventWorkspace(numPixels, numBins);

    AnalysisDataService::Instance().add(ws1Name, ws1);
    ws2 = WorkspaceCreationHelper::createEventWorkspace(5, numBins);

    ConjoinWorkspaces conj;
    conj.initialize();
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace1", ws1Name));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("InputWorkspace2", ws2));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckOverlapping", true));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckMatchingBins", false));
    TS_ASSERT_THROWS_NOTHING(conj.execute());
    // Falls over as they overlap
    TS_ASSERT(!conj.isExecuted());

    // Adjust second workspace
    Mantid::specnum_t start = ws1->getSpectrum(numPixels - 1).getSpectrumNo() + 10;
    for (int i = 0; i < 5; ++i) {
      auto &spec = ws2->getSpectrum(i);
      spec.setSpectrumNo(start + i);
      spec.clearDetectorIDs();
      spec.addDetectorID(start + i);
    }

    TS_ASSERT_THROWS_NOTHING(conj.setProperty("InputWorkspace2", ws2));
    TS_ASSERT_THROWS_NOTHING(conj.execute());
    TS_ASSERT(conj.isExecuted());

    // Test output
    MatrixWorkspace_sptr output = getWSFromADS(ws1Name);
    // Check the first spectrum has the correct ID
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 15);
    TS_ASSERT_EQUALS(output->getSpectrum(0).getSpectrumNo(), ws1->getSpectrum(0).getSpectrumNo());
    // and the joining point
    TS_ASSERT_EQUALS(output->getSpectrum(10).getSpectrumNo(), start);
    TS_ASSERT(!output->getSpectrum(11).getDetectorIDs().empty());

    AnalysisDataService::Instance().remove(ws1Name);
  }

  void performTestNoOverlap(bool event) {
    MatrixWorkspace_sptr ws1, ws2, out;
    const int numBins = 20;

    if (event) {
      ws1 = WorkspaceCreationHelper::createEventWorkspace2(10, numBins); // 2 events per bin
      ws2 = WorkspaceCreationHelper::createEventWorkspace2(5, numBins);
    } else {
      ws1 = WorkspaceCreationHelper::create2DWorkspace(10, numBins);
      ws2 = WorkspaceCreationHelper::create2DWorkspace(5, numBins);
    }
    AnalysisDataService::Instance().addOrReplace(ws1Name, ws1);
    AnalysisDataService::Instance().addOrReplace(ws2Name, ws2);

    ConjoinWorkspaces conj;
    conj.initialize();
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace1", ws1Name));
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace2", ws2Name));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckOverlapping", false));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckMatchingBins", false));
    TS_ASSERT_THROWS_NOTHING(conj.execute();)
    TS_ASSERT(conj.isExecuted());

    TS_ASSERT_THROWS_NOTHING(out = getWSFromADS(ws1Name););
    if (!out)
      return;

    TS_ASSERT_EQUALS(out->getNumberHistograms(), 15);
    TS_ASSERT_EQUALS(out->blocksize(), numBins);

    for (size_t wi = 0; wi < out->getNumberHistograms(); wi++) {
      const auto &y = out->y(wi);
      for (const auto value : y)
        TS_ASSERT_DELTA(value, 2.0, 1e-5);
    }
  }

  void test_DONTCheckForOverlap_Events() { performTestNoOverlap(true); }
  void test_DONTCheckForOverlap_2D() { performTestNoOverlap(false); }

  void performNonConstantBins(bool event) {
    MatrixWorkspace_sptr ws1, ws2, out;
    const int numBins = 20;

    if (event) {
      ws1 = WorkspaceCreationHelper::createEventWorkspace2(10, numBins); // 2 events per bin
      ws2 = WorkspaceCreationHelper::createEventWorkspace2(5, numBins + 1);
    } else {
      ws1 = WorkspaceCreationHelper::create2DWorkspace(10, numBins);
      ws2 = WorkspaceCreationHelper::create2DWorkspace(5, numBins + 1);
    }
    AnalysisDataService::Instance().addOrReplace(ws1Name, ws1);
    AnalysisDataService::Instance().addOrReplace(ws2Name, ws2);

    ConjoinWorkspaces conj;
    conj.initialize();
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace1", ws1Name));
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("InputWorkspace2", ws2Name));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckOverlapping", false));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckMatchingBins", false));
    TS_ASSERT_THROWS_NOTHING(conj.execute();)
    TS_ASSERT(conj.isExecuted());

    TS_ASSERT_THROWS_NOTHING(out = getWSFromADS(ws1Name););
    if (!out)
      return;

    TS_ASSERT_EQUALS(out->getNumberHistograms(), 15);

    for (size_t wi = 0; wi < out->getNumberHistograms(); wi++) {
      const auto &y = out->y(wi);
      if (wi < 10) {
        TS_ASSERT_EQUALS(y.size(), numBins);
      } else {
        TS_ASSERT_EQUALS(y.size(), numBins + 1);
      }
      for (const auto value : y)
        TS_ASSERT_DELTA(value, 2.0, 1e-5);
    }
  }

  void test_NonConstantBins_Events() { performNonConstantBins(true); }
  void test_NonConstantBins_2D() { performNonConstantBins(false); }

  void setupAlgForSetYUnitAndLabel(ConjoinWorkspaces &conj) {
    MatrixWorkspace_sptr ws1, ws2, out;
    int numBins = 20;

    ws1 = WorkspaceCreationHelper::create2DWorkspace(10, numBins);
    ws2 = WorkspaceCreationHelper::create2DWorkspace(5, numBins);

    AnalysisDataService::Instance().addOrReplace(ws1Name, ws1);
    AnalysisDataService::Instance().addOrReplace(ws2Name, ws2);

    conj.initialize();
    conj.setRethrows(true);
    conj.setPropertyValue("InputWorkspace1", ws1Name);
    conj.setPropertyValue("InputWorkspace2", ws2Name);
    conj.setProperty("CheckOverlapping", false);
  }

  void test_setYUnitAndLabel() {
    ConjoinWorkspaces conj;
    setupAlgForSetYUnitAndLabel(conj);
    const std::string label = "Modified y label";
    const std::string unit = "Modified y unit";

    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("YAxisLabel", label));
    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("YAxisUnit", unit));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckMatchingBins", false));

    TS_ASSERT_THROWS_NOTHING(conj.execute());

    auto out = getWSFromADS(ws1Name);
    if (!out)
      return;

    TS_ASSERT_EQUALS(label, out->YUnitLabel());
    TS_ASSERT_EQUALS(unit, out->YUnit());
  }

  void test_setYUnit() {
    ConjoinWorkspaces conj;
    setupAlgForSetYUnitAndLabel(conj);

    auto out = getWSFromADS(ws1Name);
    if (!out)
      return;

    const std::string label = "Should be unmodified";
    out->setYUnitLabel(label);

    const std::string unit = "Modified y unit";

    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("YAxisUnit", unit));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckMatchingBins", false));
    TS_ASSERT_THROWS_NOTHING(conj.execute());

    auto result = getWSFromADS(ws1Name);

    TSM_ASSERT_EQUALS("YUnitLabel was not reset after YUnit changed", result->YUnitLabel(), label);
    TS_ASSERT_EQUALS(unit, result->YUnit());
  }

  void test_setYLabel() {
    ConjoinWorkspaces conj;
    setupAlgForSetYUnitAndLabel(conj);

    auto out = getWSFromADS(ws1Name);
    if (!out)
      return;

    const std::string unit = "Should be unmodified";
    out->setYUnit(unit);

    const std::string label = "Modified y label";

    TS_ASSERT_THROWS_NOTHING(conj.setPropertyValue("YAxisLabel", label));
    TS_ASSERT_THROWS_NOTHING(conj.setProperty("CheckMatchingBins", false));
    TS_ASSERT_THROWS_NOTHING(conj.execute());

    auto result = getWSFromADS(ws1Name);

    TS_ASSERT_EQUALS(unit, result->YUnit());
    TS_ASSERT_EQUALS(label, result->YUnitLabel());
  }

private:
  const std::string ws1Name{"ws1name"};
  const std::string ws2Name{"ws2name"};

  void setupMismatchedWorkspace(const std::string &name, int startIndex, int endIndex,
                                const std::string &rebinParams) const {
    MatrixWorkspace_sptr ews = WorkspaceCreationHelper::createEventWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(name, ews);

    // Crop ews to have first 3 spectra, ews2 to have second 3
    CropWorkspace crop;
    crop.setChild(true);
    crop.initialize();
    crop.setProperty("InputWorkspace", ews);
    crop.setProperty("StartWorkspaceIndex", startIndex);
    crop.setProperty("EndWorkspaceIndex", endIndex);
    crop.setProperty("OutputWorkspace", name);
    crop.execute();

    Rebin rebin;
    rebin.setChild(true);
    rebin.initialize();
    rebin.setProperty("InputWorkspace", name);
    rebin.setProperty("Params", rebinParams);
    rebin.setProperty("OutputWorkspace", name);
    rebin.execute();
  }
};
