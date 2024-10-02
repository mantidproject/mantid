// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/CreatePeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::MantidVec;
using Mantid::Kernel::V3D;

namespace {
std::string const PROPERTY_VALUE_TRUE("1");
std::string const PROPERTY_VALUE_FALSE("0");
} // namespace

class CompareWorkspacesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompareWorkspacesTest *createSuite() { return new CompareWorkspacesTest(); }
  static void destroySuite(CompareWorkspacesTest *suite) { delete suite; }

  CompareWorkspacesTest() : ws1(WorkspaceCreationHelper::create2DWorkspace123(2, 2)) { FrameworkManager::Instance(); }

  void testName() { TS_ASSERT_EQUALS(checker.name(), "CompareWorkspaces"); }

  void testVersion() { TS_ASSERT_EQUALS(checker.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(checker.initialize());
    TS_ASSERT(checker.isInitialized());
  }

  void testMatches() {
    if (!checker.isInitialized())
      checker.initialize();

    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(10, 100);
    // A workspace had better match itself!
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws));

    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Same, using the Mantid::API::equals() function
    TS_ASSERT(Mantid::API::equals(ws, ws));
  }

  void testNotMatches() {
    if (!checker.isInitialized())
      checker.initialize();

    WorkspaceSingleValue_sptr ws1 = WorkspaceCreationHelper::createWorkspaceSingleValue(1.0);
    WorkspaceSingleValue_sptr ws2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.0);
    //
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);
    // Same, using the Mantid::API::equals() function
    TS_ASSERT(!Mantid::API::equals(ws1, ws2));
    // cleanup
    checker.resetProperties();
  }

  void testMatchesRelative_large() {
    if (!checker.isInitialized())
      checker.initialize();

    WorkspaceSingleValue_sptr wks1 = WorkspaceCreationHelper::createWorkspaceSingleValue(100000.0);
    WorkspaceSingleValue_sptr wks2 = WorkspaceCreationHelper::createWorkspaceSingleValue(100001.0);

    // Ensure they are NOT equal within absolute tolerance
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", wks1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", wks2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 0.01));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("ToleranceRelErr", false));
    TS_ASSERT_THROWS_NOTHING(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);
    // Ensure they ARE equal within relative tolerance
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", wks1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", wks2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 0.01));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("ToleranceRelErr", true));
    TS_ASSERT_THROWS_NOTHING(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMatchesRelative_small() {
    if (!checker.isInitialized())
      checker.initialize();

    WorkspaceSingleValue_sptr ws1 = WorkspaceCreationHelper::createWorkspaceSingleValue(0.000001);
    WorkspaceSingleValue_sptr ws2 = WorkspaceCreationHelper::createWorkspaceSingleValue(0.000002);
    WorkspaceSingleValue_sptr ws3 = WorkspaceCreationHelper::createWorkspaceSingleValue(0.00000201);

    // Ensure ws1, ws2 ARE equal within absolute tolerance
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 0.1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("ToleranceRelErr", false));
    TS_ASSERT_THROWS_NOTHING(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Ensure ws1, ws2 ARE NOT equal within relative tolerance
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 0.1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("ToleranceRelErr", true));
    TS_ASSERT_THROWS_NOTHING(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);
    // Ensure ws2, ws3 ARE equal within absolute tolerance
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws3));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 0.1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("ToleranceRelErr", false));
    TS_ASSERT_THROWS_NOTHING(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Ensure ws2, ws3 ARE equal within relative tolerance
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws3));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 0.1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("ToleranceRelErr", true));
    TS_ASSERT_THROWS_NOTHING(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // cleanup
    checker.resetProperties();
  }

  void testNotMatchesRelative() {
    if (!checker.isInitialized())
      checker.initialize();

    WorkspaceSingleValue_sptr ws1 = WorkspaceCreationHelper::createWorkspaceSingleValue(1.1);
    WorkspaceSingleValue_sptr ws2 = WorkspaceCreationHelper::createWorkspaceSingleValue(2.2);
    //
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 0.1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("ToleranceRelErr", true));
    //
    TS_ASSERT_THROWS_NOTHING(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);
    // cleanup
    checker.resetProperties();
  }

  void testCheckErrorMatches() {
    if (!checker.isInitialized())
      checker.initialize();

    WorkspaceSingleValue_sptr ws1 = WorkspaceCreationHelper::createWorkspaceSingleValueWithError(1.1, 2.0);
    WorkspaceSingleValue_sptr ws2 = WorkspaceCreationHelper::createWorkspaceSingleValueWithError(1.1, 2.0);
    //
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("CheckUncertainty", true));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));
    //
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Same, using the Mantid::API::equals() function
    TS_ASSERT(Mantid::API::equals(ws1, ws2));
    // cleanup
    checker.resetProperties();
  }

  void testCheckErrorNotMatches() {
    if (!checker.isInitialized())
      checker.initialize();

    WorkspaceSingleValue_sptr ws1 = WorkspaceCreationHelper::createWorkspaceSingleValueWithError(1.1, 2.0);
    WorkspaceSingleValue_sptr ws2 = WorkspaceCreationHelper::createWorkspaceSingleValueWithError(1.1, 4.0);
    // make sure ARE equal if errors NOT checked
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("CheckUncertainty", false));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // make sure are NOT equal if errors ARE checked
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("CheckUncertainty", true));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);
    // cleanup
    checker.resetProperties();
  }

  void test_NaNsEqual_true() {
    if (!checker.isInitialized())
      checker.initialize();

    double const anan = std::numeric_limits<double>::quiet_NaN();

    // a real and NaN are never equal
    WorkspaceSingleValue_sptr ws1 = WorkspaceCreationHelper::createWorkspaceSingleValue(1.1);
    WorkspaceSingleValue_sptr ws2 = WorkspaceCreationHelper::createWorkspaceSingleValue(anan);
    // is not equal if NaNsEqual set true
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("NaNsEqual", true));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);
    // is not equal if NaNsEqual set false
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("NaNsEqual", false));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);

    // NaNs only compare equal if flag set
    WorkspaceSingleValue_sptr ws3 = WorkspaceCreationHelper::createWorkspaceSingleValue(anan);
    // is NOT equal if NaNsEqual set FALSE
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("NaNsEqual", false));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws3));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);
    // ARE equal if NaNsEqual set TRUE
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("NaNsEqual", true));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws3));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testPeaks_matches() {
    if (!checker.isInitialized())
      checker.initialize();

    std::string outWS1Name("CreatePeaks1WorkspaceTest_OutputWS");
    std::string outWS2Name("CreatePeaks2WorkspaceTest_OutputWS");

    Workspace2D_sptr instws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);

    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InstrumentWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(instws)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWS1Name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfPeaks", 13));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InstrumentWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(instws)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWS2Name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfPeaks", 13));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr pws1, pws2;
    TS_ASSERT_THROWS_NOTHING(
        pws1 = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWS1Name)));
    TS_ASSERT_THROWS_NOTHING(
        pws2 = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWS2Name)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<Workspace>(pws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<Workspace>(pws2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void test_LeanPeaksWorkspaceMatches() {
    // generate a lean elastic peak workspace with two peaks
    auto lpws = std::make_shared<LeanElasticPeaksWorkspace>();
    // add peaks
    LeanElasticPeak pk1(V3D(0.0, 0.0, 6.28319), 2.0);     // (100)
    LeanElasticPeak pk2(V3D(6.28319, 0.0, 6.28319), 1.0); // (110)
    lpws->addPeak(pk1);
    lpws->addPeak(pk2);

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<Workspace>(lpws)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<Workspace>(lpws)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void test_LeanPeaksWithModulationVectorsMatch() {
    // generate a lean elastic peak workspace with two peaks
    auto lpws = std::make_shared<LeanElasticPeaksWorkspace>();
    // add peaks
    LeanElasticPeak pk1(V3D(0.0, 0.0, 6.28319), 2.0);     // (100)
    LeanElasticPeak pk2(V3D(6.28319, 0.0, 6.28319), 1.0); // (110)
    pk1.setIntHKL(V3D(1, 0, 0));
    pk2.setIntHKL(V3D(1, 1, 0));
    pk2.setIntMNP(V3D(1, 2, 3));
    pk2.setIntMNP(V3D(3, 2, 1));
    lpws->addPeak(pk1);
    lpws->addPeak(pk2);

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<Workspace>(lpws)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<Workspace>(lpws)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void test_RelativeErrorInPeaksWorkspace() {
    if (!checker.isInitialized())
      checker.initialize();

    const double tol = checker.getProperty("Tolerance");
    auto pws1 = std::make_shared<LeanElasticPeaksWorkspace>();
    auto pws2 = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak pk1(V3D(4.0, 0.0, 0.0));
    pws1->addPeak(pk1);
    LeanElasticPeak pk2(V3D(4.0 + 2.0 * tol, 0.0, 0.0));
    pws2->addPeak(pk2);

    // check matches with relative error
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<Workspace>(pws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<Workspace>(pws2)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("ToleranceRelErr", true));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // cleanup
    checker.resetProperties();
  }

  void testPeaks_extrapeak() {
    if (!checker.isInitialized())
      checker.initialize();

    std::string outWS3Name("CreatePeaks3WorkspaceTest_OutputWS");
    std::string outWS4Name("CreatePeaks4WorkspaceTest_OutputWS");

    Workspace2D_sptr instws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);

    CreatePeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InstrumentWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(instws)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWS3Name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfPeaks", 13));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InstrumentWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(instws)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWS4Name));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfPeaks", 14));
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr pws1, pws2;
    TS_ASSERT_THROWS_NOTHING(
        pws1 = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWS3Name)));
    TS_ASSERT_THROWS_NOTHING(
        pws2 = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWS4Name)));
    TS_ASSERT_EQUALS(pws1->getNumberPeaks(), 13);
    TS_ASSERT_EQUALS(pws2->getNumberPeaks(), 14);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<Workspace>(pws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<Workspace>(pws2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testEvent_matches() {
    if (!checker.isInitialized())
      checker.initialize();

    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<MatrixWorkspace>(ews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<MatrixWorkspace>(ews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    // Same, using the Mantid::API::equals() function
    TS_ASSERT(Mantid::API::equals(ews1, ews2));
  }

  void testEvent_different_type() {
    if (!checker.isInitialized())
      checker.initialize();

    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<MatrixWorkspace>(ews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    // Same, using the Mantid::API::equals() function
    TS_ASSERT(!Mantid::API::equals(ws1, ews2));
  }

  void testEvent_different_number_histograms() {
    if (!checker.isInitialized())
      checker.initialize();

    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::createEventWorkspace(15, 20, 30);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<MatrixWorkspace>(ews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<MatrixWorkspace>(ews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ews1, ews2)));
  }

  void testEvent_differentEventLists() {
    if (!checker.isInitialized())
      checker.initialize();
    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30, 0.0, 1.0, 2);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<MatrixWorkspace>(ews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<MatrixWorkspace>(ews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ews1, ews2)));
  }

  void testEvent_differentEventWeights() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    ews1 *= 1.1;
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    ews2 *= 1.2;
    alg.setProperty("Workspace1", std::dynamic_pointer_cast<MatrixWorkspace>(ews1));
    alg.setProperty("Workspace2", std::dynamic_pointer_cast<MatrixWorkspace>(ews2));
    alg.setProperty("CheckAllData", true);
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ews1, ews2)));
    auto result = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    auto message = result->String(0, 0);
    TS_ASSERT_EQUALS(message, "Total 300 (in 300) events are differrent. 0 have different "
                              "TOF; 0 have different pulse time; 0 have different in both "
                              "TOF and pulse time; 300 have different weights.\nMismatched "
                              "event lists include 10 of total 10 spectra. \n0, ");
  }

  void testEvent_differentEventWeightsNoTime() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    ews1 *= 1.1;
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30);
    ews2 *= 1.2;
    for (size_t i = 0; i < ews1->getNumberHistograms(); ++i) {
      ews1->getSpectrum(i).switchTo(Mantid::API::EventType::WEIGHTED_NOTIME);
      ews2->getSpectrum(i).switchTo(Mantid::API::EventType::WEIGHTED_NOTIME);
    }
    alg.setProperty("Workspace1", std::dynamic_pointer_cast<MatrixWorkspace>(ews1));
    alg.setProperty("Workspace2", std::dynamic_pointer_cast<MatrixWorkspace>(ews2));
    alg.setProperty("CheckAllData", true);
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ews1, ews2)));
    auto result = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    auto message = result->String(0, 0);
    TS_ASSERT_EQUALS(message, "Total 300 (in 300) events are differrent. 0 have different "
                              "TOF; 0 have different pulse time; 0 have different in both "
                              "TOF and pulse time; 300 have different weights.\nMismatched "
                              "event lists include 10 of total 10 spectra. \n0, ");
  }

  void testEvent_differentBinBoundaries() {
    if (!checker.isInitialized())
      checker.initialize();
    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30, 15.0, 10.0);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::createEventWorkspace(10, 20, 30, 5.0, 10.0);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<MatrixWorkspace>(ews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<MatrixWorkspace>(ews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ews1, ews2)));
  }

  void testMDEvents_matches() {
    if (!checker.isInitialized())
      checker.initialize();
    MDEventWorkspace3Lean::sptr mdews1 = MDEventsTestHelper::makeFakeMDEventWorkspace("mdev1");
    MDEventWorkspace3Lean::sptr mdews2 = MDEventsTestHelper::makeFakeMDEventWorkspace("mdev2");
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDEvents_different_eventtypes() {
    if (!checker.isInitialized())
      checker.initialize();
    MDEventWorkspace3Lean::sptr mdews1 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "A");
    MDEventWorkspace3::sptr mdews2 = MDEventsTestHelper::makeAnyMDEW<MDEvent<3>, 3>(2, 0.0, 10.0, 1000, "B");
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDEvents_different_dims() {
    if (!checker.isInitialized())
      checker.initialize();
    MDEventWorkspace4Lean::sptr mdews1 = MDEventsTestHelper::makeMDEW<4>(5, -10.0, 10.0, 1);
    MDEventWorkspace3Lean::sptr mdews2 = MDEventsTestHelper::makeMDEW<3>(5, -10.0, 10.0, 1);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDEvents_different_dimnames() {
    if (!checker.isInitialized())
      checker.initialize();
    MDEventWorkspace3Lean::sptr mdews1 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "A");
    MDEventWorkspace3Lean::sptr mdews2 =
        MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "B", "X%d");
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDEvents_different_dimmin() {
    if (!checker.isInitialized())
      checker.initialize();
    MDEventWorkspace3Lean::sptr mdews1 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "A");
    MDEventWorkspace3Lean::sptr mdews2 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 1.0, 10.0, 1000, "B");
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDEvents_different_numdata() {
    if (!checker.isInitialized())
      checker.initialize();
    MDEventWorkspace3Lean::sptr mdews1 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "A");
    MDEventWorkspace3Lean::sptr mdews2 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 5000, "B");
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDEvents_different_data() {
    if (!checker.isInitialized())
      checker.initialize();
    MDEventWorkspace3Lean::sptr mdews1 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "A");
    MDEventWorkspace3Lean::sptr mdews2 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "B");
    MDBoxBase<MDLeanEvent<3>, 3> *parentBox = dynamic_cast<MDBoxBase<MDLeanEvent<3>, 3> *>(mdews2->getBox());
    std::vector<IMDNode *> boxes;
    parentBox->getBoxes(boxes, 1000, true);
    MDBox<MDLeanEvent<3>, 3> *box = dynamic_cast<MDBox<MDLeanEvent<3>, 3> *>(boxes[0]);
    std::vector<MDLeanEvent<3>> &events = box->getEvents();
    const float offset = 0.1f;
    events[0].setSignal(events[0].getSignal() + offset);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDEvents_different_error() {
    if (!checker.isInitialized())
      checker.initialize();
    MDEventWorkspace3Lean::sptr mdews1 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "A");
    MDEventWorkspace3Lean::sptr mdews2 = MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0.0, 10.0, 1000, "B");
    MDBoxBase<MDLeanEvent<3>, 3> *parentBox = dynamic_cast<MDBoxBase<MDLeanEvent<3>, 3> *>(mdews2->getBox());
    std::vector<IMDNode *> boxes;
    parentBox->getBoxes(boxes, 1000, true);
    MDBox<MDLeanEvent<3>, 3> *box = dynamic_cast<MDBox<MDLeanEvent<3>, 3> *>(boxes[0]);
    std::vector<MDLeanEvent<3>> &events = box->getEvents();
    const float offset = 0.1f;
    events[0].setErrorSquared(events[0].getErrorSquared() + offset);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdews1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdews2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDHisto_matches() {
    if (!checker.isInitialized())
      checker.initialize();
    MDHistoWorkspace_sptr mdhws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 4);
    MDHistoWorkspace_sptr mdhws2 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 4);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdhws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdhws2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_EQUALS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDHist_different_dims() {
    if (!checker.isInitialized())
      checker.initialize();
    MDHistoWorkspace_sptr mdhws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 4);
    MDHistoWorkspace_sptr mdhws2 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 3);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdhws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdhws2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDHist_different_dimnames() {
    if (!checker.isInitialized())
      checker.initialize();
    MDHistoWorkspace_sptr mdhws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 3);
    const int dims = 3;
    std::size_t numBins[dims] = {10, 10, 10};
    Mantid::coord_t min[dims] = {0.0, 0.0, 0.0};
    Mantid::coord_t max[dims] = {10.0, 10.0, 10.0};
    std::vector<std::string> names{"h", "k", "l"};
    MDHistoWorkspace_sptr mdhws2 =
        MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(3, 5.0, 1.0, numBins, min, max, names);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdhws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdhws2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDHist_different_dimbins() {
    if (!checker.isInitialized())
      checker.initialize();
    MDHistoWorkspace_sptr mdhws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 3);
    MDHistoWorkspace_sptr mdhws2 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 3, 5);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdhws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdhws2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDHist_different_dimmax() {
    if (!checker.isInitialized())
      checker.initialize();
    MDHistoWorkspace_sptr mdhws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 3);
    Mantid::coord_t max = static_cast<Mantid::coord_t>(10.1);
    MDHistoWorkspace_sptr mdhws2 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 3, 10, max);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdhws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdhws2)));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testMDHist_different_data() {
    if (!checker.isInitialized())
      checker.initialize();
    MDHistoWorkspace_sptr mdhws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 4);
    MDHistoWorkspace_sptr mdhws2 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.1, 4);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdhws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdhws2)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 1.0e-5));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // cleanup
    checker.resetProperties();
  }

  void testMDHist_different_error() {
    if (!checker.isInitialized())
      checker.initialize();
    MDHistoWorkspace_sptr mdhws1 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 4);
    MDHistoWorkspace_sptr mdhws2 = MDEventsTestHelper::makeFakeMDHistoWorkspace(5.0, 4, 10, 10.0, 1.1);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", std::dynamic_pointer_cast<IMDWorkspace>(mdhws1)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", std::dynamic_pointer_cast<IMDWorkspace>(mdhws2)));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Tolerance", 1.0e-5));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    // cleanup
    checker.resetProperties();
  }

  void testDifferentSize() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create1DWorkspaceFib(2, true);

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Size mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testHistNotHist() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2, true);

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Histogram/point-like mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDistNonDist() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->setDistribution(true);

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Distribution flag mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDifferentAxisType() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    auto newAxis = std::make_unique<Mantid::API::NumericAxis>(2);
    ws2->replaceAxis(1, std::move(newAxis));

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Axis 1 type mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDifferentAxisTitles() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->getAxis(0)->title() = "blah";

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Axis 0 title mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDifferentAxisUnit() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Axis 0 unit mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDifferentAxisValues() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws1local = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    // Put numeric axes on these workspaces as checkAxes won't test values on
    // spectra axes
    auto newAxisWS1 = std::make_unique<NumericAxis>(ws1local->getAxis(1)->length());
    newAxisWS1->setValue(0, 1);
    newAxisWS1->setValue(1, 2);
    auto newAxisWS2 = std::make_unique<NumericAxis>(ws2->getAxis(1)->length());
    newAxisWS2->setValue(0, 1);
    newAxisWS2->setValue(1, 2);
    ws1local->replaceAxis(1, std::move(newAxisWS1));
    ws2->replaceAxis(1, std::move(newAxisWS2));

    // Check that it's all good
    TS_ASSERT((Mantid::API::equals(ws1local, ws2)));

    // Now change a value in one axis
    ws2->getAxis(1)->setValue(1, 99);

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1local));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Axis 1 values mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1local, ws2)));
  }

  void testDifferentYUnit() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->setYUnit("blah");

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "YUnit mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDifferentSpectraMap() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->getSpectrum(0).setSpectrumNo(1234);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Spectrum number mismatch");

    ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->getSpectrum(0).setDetectorID(99);
    ws2->getSpectrum(1).setDetectorID(98);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Detector IDs mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDifferentInstruments() {
    if (!checker.isInitialized())
      checker.initialize();
    Workspace2D_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 2, false, false, true, "original", false);
    AnalysisDataService::Instance().addOrReplace("original", ws);
    // test different names
    Workspace2D_sptr ws2 =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 2, false, false, true, "distorted", false);
    AnalysisDataService::Instance().addOrReplace("distorted", ws2);
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Instrument name mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));

    // test different source position
    Workspace2D_sptr ws3 = ws->clone(); // shared to unique ptr conversion
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws3));
    auto &info3 = ws3->mutableComponentInfo();
    info3.setPosition(info3.source(), info3.sourcePosition() + V3D(0, 0, 1e-6));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT(table->cell<std::string>(0, 0).find("Source mismatch") != std::string::npos);

    // Compare different sample position
    Workspace2D_sptr ws4 = ws->clone(); // shared to unique ptr conversion
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws4));
    auto &info4 = ws4->mutableComponentInfo();
    info4.setPosition(info4.sample(), info4.samplePosition() + V3D(0, 0, 1e-6));
    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
    table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT(table->cell<std::string>(0, 0).find("Sample mismatch") != std::string::npos);
  }

  void testDifferentParameterMaps() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    auto component = std::make_unique<Mantid::Geometry::Component>();
    ws2->instrumentParameters().addBool(component.get(), "myParam", true);

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0),
                     "Instrument ParameterMap mismatch (differences in ordering ignored)");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDifferentMasking() {
    if (!checker.isInitialized())
      checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->maskBin(0, 0);
    ws2->dataY(0)[0] = 2;
    ws2->dataE(0)[0] = 3;

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Masking mismatch");

    Mantid::API::MatrixWorkspace_sptr ws3 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws3->maskBin(0, 1);
    ws3->dataY(0)[1] = 2;
    ws3->dataE(0)[1] = 3;

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws3));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Masking mismatch");

    // Same, using the !Mantid::API::equals() function
    TS_ASSERT((!Mantid::API::equals(ws1, ws2)));
  }

  void testDifferentSampleName() {
    if (!checker.isInitialized())
      checker.initialize();
    checker.setProperty("CheckSample", true);

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->mutableSample().setName("different");

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Sample name mismatch");
  }

  void testDifferentProtonCharge() {
    if (!checker.isInitialized())
      checker.initialize();
    checker.setProperty("CheckSample", true);

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->mutableRun().setProtonCharge(99.99);

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Proton charge mismatch");
  }

  void testDifferentLogs() {
    if (!checker.isInitialized())
      checker.initialize();
    checker.setProperty("CheckSample", true);

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws2->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop1", 99));

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws2));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Different numbers of logs");

    Mantid::API::MatrixWorkspace_sptr ws3 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws3->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop2", 99));

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws3));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Log mismatch");

    Mantid::API::MatrixWorkspace_sptr ws4 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    ws4->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop1", 100));

    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace1", ws2));
    TS_ASSERT_THROWS_NOTHING(checker.setProperty("Workspace2", ws4));

    TS_ASSERT(checker.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Log mismatch");
  }

  void testSameLogsButInDifferentOrder() {
    MatrixWorkspace_sptr ws1 = WorkspaceCreationHelper::create2DWorkspace123(1, 1);
    MatrixWorkspace_sptr ws2 = ws1->clone();
    ws1->mutableRun().addProperty("property1", 1);
    ws1->mutableRun().addProperty("property2", 2);
    // Add same properties to ws2 but in reverse order.
    ws2->mutableRun().addProperty("property2", 2);
    ws2->mutableRun().addProperty("property1", 1);
    CompareWorkspaces compare;
    compare.initialize();
    compare.setChild(true);
    compare.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace1", ws1))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace2", ws2))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckType", false))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckAxes", false))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckSpectraMap", false))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckInstrument", false))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckMasking", false))
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckSample", true))
    TS_ASSERT_THROWS_NOTHING(compare.execute())
    TS_ASSERT(compare.isExecuted())
    const bool workspacesMatch = compare.getProperty("Result");
    TS_ASSERT(workspacesMatch)
  }

  void test_Input_With_Two_Groups_That_Are_The_Same_Matches() {
    // Create a group
    const std::string groupName("TestGroup");
    WorkspaceGroup_sptr group = WorkspaceCreationHelper::createWorkspaceGroup(2, 2, 2, groupName);

    doGroupTest(groupName, groupName, PROPERTY_VALUE_TRUE);

    cleanupGroup(group);
  }

  void test_Input_With_Two_Groups_That_Are_Different_Sizes_Fails() {
    // Create a group
    const std::string groupOneName("TestGroupOne");
    WorkspaceGroup_sptr groupOne = WorkspaceCreationHelper::createWorkspaceGroup(2, 2, 2, groupOneName);
    const std::string groupTwoName("TestGroupTwo");
    WorkspaceGroup_sptr groupTwo = WorkspaceCreationHelper::createWorkspaceGroup(3, 2, 2, groupTwoName);

    doGroupTest(groupOneName, groupTwoName, "GroupWorkspaces size mismatch.", std::map<std::string, std::string>(),
                true);

    cleanupGroup(groupOne);
    cleanupGroup(groupTwo);
  }

  void test_Input_With_A_Group_And_A_Single_Workspace_Gives_Type_Mismatch() {
    const std::string groupName("CheckWorkspacesMatch_TestGroup");
    WorkspaceGroup_sptr group = WorkspaceCreationHelper::createWorkspaceGroup(2, 2, 2, groupName);
    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(2, 2);
    const std::string wsName("CheckWorkspacesMatch_TestWS");
    Mantid::API::AnalysisDataService::Instance().add(wsName, ws2);

    doGroupTest(groupName, wsName, "Type mismatch. One workspace is a group, the other is not.");

    // Cleanup
    cleanupGroup(group);
    Mantid::API::AnalysisDataService::Instance().remove(wsName);
  }

  void test_Input_With_Two_Groups_When_Single_Item_Checking_Is_Disabled() {
    Mantid::API::AnalysisDataService::Instance().clear();
    // Create a group
    const std::string groupOneName("TestGroupOne");
    WorkspaceGroup_sptr groupOne = WorkspaceCreationHelper::createWorkspaceGroup(2, 2, 2, groupOneName);
    const std::string groupTwoName("TestGroupTwo");
    WorkspaceGroup_sptr groupTwo = WorkspaceCreationHelper::createWorkspaceGroup(2, 2, 2, groupTwoName);
    Mantid::API::AnalysisDataServiceImpl &dataStore = Mantid::API::AnalysisDataService::Instance();
    // Extract the zeroth element of groupTwo and add a spurious log
    MatrixWorkspace_sptr zero = std::dynamic_pointer_cast<MatrixWorkspace>(dataStore.retrieve(groupTwo->getNames()[0]));
    TS_ASSERT(zero);
    using Mantid::Kernel::PropertyWithValue;
    zero->mutableRun().addProperty(new PropertyWithValue<double>("ExtraLog", 10));

    doGroupTest(groupOneName, groupTwoName, "Different numbers of logs", {{"CheckSample", "1"}});

    // Cleanup
    cleanupGroup(groupOne);
    cleanupGroup(groupTwo);
  }

  void test_empty_tableworkspaces_match() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", WorkspaceFactory::Instance().createTable()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", WorkspaceFactory::Instance().createTable()));
    TS_ASSERT(alg.execute());
    TS_ASSERT_EQUALS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void test_tableworkspace_different_number_of_columns_fails() {
    auto table1 = WorkspaceFactory::Instance().createTable();
    auto table2 = WorkspaceFactory::Instance().createTable();
    table1->addColumns("int", "aColumn", 2);
    table2->addColumns("int", "aColumn", 3);

    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));

    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Number of columns mismatch");
  }

  void test_tableworkspace_different_number_of_rows_fails() {
    auto table1 = WorkspaceFactory::Instance().createTable();
    auto table2 = WorkspaceFactory::Instance().createTable();
    table1->addColumn("double", "aColumn");
    table1->appendRow();
    table1->appendRow();
    table2->addColumn("double", "aColumn");
    table2->appendRow();

    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));

    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Number of rows mismatch");
  }

  void test_tableworkspace_matches_itself() {
    auto table = setupTableWorkspace();
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table));

    TS_ASSERT(alg.execute());
    TS_ASSERT_EQUALS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void test_equal_tableworkspaces_match() {
    std::string const col_type("double"), col_name("aColumn");
    std::vector<double> col_values{1.0, 2.0, 3.0};
    // create the table workspaces
    Mantid::API::ITableWorkspace_sptr table1 = WorkspaceFactory::Instance().createTable();
    table1->addColumn(col_type, col_name);
    for (double val : col_values) {
      TableRow newrow = table1->appendRow();
      newrow << val;
    }
    Mantid::API::ITableWorkspace_sptr table2 = WorkspaceFactory::Instance().createTable();
    table2->addColumn(col_type, col_name);
    for (double val : col_values) {
      TableRow newrow = table2->appendRow();
      newrow << val;
    }

    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));
    TS_ASSERT(alg.execute());
    TS_ASSERT_EQUALS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void test_tableworkspace_NaNs_passes_with_flag() {
    std::string const col_type("double"), col_name("aColumn");
    std::vector<double> col_values{1.0, 2.0, std::numeric_limits<double>::quiet_NaN()};
    // create the table workspaces
    Mantid::API::ITableWorkspace_sptr table1 = WorkspaceFactory::Instance().createTable();
    Mantid::API::ITableWorkspace_sptr table2 = WorkspaceFactory::Instance().createTable();
    table1->addColumn(col_type, col_name);
    table2->addColumn(col_type, col_name);
    for (double val : col_values) {
      TableRow newrow1 = table1->appendRow();
      newrow1 << val;
      TableRow newrow2 = table2->appendRow();
      newrow2 << val;
    }
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NaNsEqual", true));
    TS_ASSERT(alg.execute());
    TS_ASSERT_EQUALS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void test_tableworkspace_NaNs_fails() {
    std::string const col_type("double"), col_name("aColumn");
    std::vector<double> col_values1{1.0, 2.0, 3.0};
    std::vector<double> col_values2{1.0, 2.0, std::numeric_limits<double>::quiet_NaN()};
    // create the table workspaces
    Mantid::API::ITableWorkspace_sptr table1 = WorkspaceFactory::Instance().createTable();
    table1->addColumn(col_type, col_name);
    for (double val : col_values1) {
      TableRow newrow = table1->appendRow();
      newrow << val;
    }
    Mantid::API::ITableWorkspace_sptr table2 = WorkspaceFactory::Instance().createTable();
    table2->addColumn(col_type, col_name);
    for (double val : col_values2) {
      TableRow newrow = table2->appendRow();
      newrow << val;
    }

    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));
    TS_ASSERT(alg.execute());
    TS_ASSERT_EQUALS(alg.getPropertyValue("Result"), PROPERTY_VALUE_FALSE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Table data mismatch");
  }

  void test_tableworkspace_different_column_names_fails() {
    auto table1 = setupTableWorkspace();
    table1->getColumn(5)->setName("SomethingElse");
    auto table2 = setupTableWorkspace();
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));

    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Column name mismatch");
  }

  void test_tableworkspace_different_column_types_fails() {
    auto table1 = setupTableWorkspace();
    auto table2 = setupTableWorkspace();
    table2->removeColumn("V3D");
    table2->addColumn("int", "V3D");
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));

    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Column type mismatch");
  }

  void test_tableworkspace_different_data_fails() {
    auto table1 = setupTableWorkspace();
    auto table2 = setupTableWorkspace();
    table2->cell<size_t>(1, 3) = 123;
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Table data mismatch");

    table2 = setupTableWorkspace();
    table1->cell<std::string>(2, 7) = "?";
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Table data mismatch");

    table1 = setupTableWorkspace();
    table2->cell<Mantid::Kernel::V3D>(0, 8) = Mantid::Kernel::V3D(9.9, 8.8, 7.7);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", table1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", table2));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Table data mismatch");
  }

  void test_mixing_peaks_and_table_workspaces_fails() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", WorkspaceFactory::Instance().createTable()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", WorkspaceFactory::Instance().createPeaks()));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "One workspace is a PeaksWorkspace and the other is not.");
  }

  void test_mixing_matrix_and_table_workspaces_fails() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", WorkspaceFactory::Instance().createTable()));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("Workspace2", WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1)));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(checker.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "One workspace is a TableWorkspace and the other is not.");
  }

  MatrixWorkspace_sptr create_RaggedWorkspace(int version = 0) {
    // create workspace with 2 histograms
    MatrixWorkspace_sptr raggedWS = WorkspaceCreationHelper::create2DWorkspace(2, 1);

    // create and replace histograms with ragged ones
    MantidVec x_data{100., 200., 300., 400.};
    MantidVec y_data{1., 1., 1.};
    MantidVec e_data{1., 1., 1.};
    Mantid::HistogramData::HistogramBuilder builder;
    builder.setX(x_data);
    builder.setY(y_data);
    builder.setE(e_data);
    raggedWS->setHistogram(0, builder.build());

    MantidVec x_data2{200., 400., 600.};
    MantidVec y_data2{1., 1.};
    MantidVec e_data2{1., 1.};
    if (version == 1) {
      // different number of bins
      x_data2 = {200., 400.};
      y_data2 = {1.};
      e_data2 = {1.};
    } else if (version == 2) {
      // same number of bins but different y values
      y_data2 = {1., 2.};
    } else if (version == 3) {
      // same number of bins but different x values
      x_data2 = {200., 500., 600.};
    }

    Mantid::HistogramData::HistogramBuilder builder2;
    builder2.setX(x_data2);
    builder2.setY(y_data2);
    builder2.setE(e_data2);
    raggedWS->setHistogram(1, builder2.build());

    // quick check of the workspace
    TS_ASSERT(raggedWS->isRaggedWorkspace());
    return raggedWS;
  }

  void test_ragged_workspace() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", create_RaggedWorkspace()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", create_RaggedWorkspace()));
    TS_ASSERT(alg.execute());
    TS_ASSERT_EQUALS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void test_ragged_workspace_fail_ragged_and_not() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", create_RaggedWorkspace()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", ws1));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Size mismatch");
  }

  void test_ragged_workspace_fail_number_of_bins() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", create_RaggedWorkspace()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", create_RaggedWorkspace(1)));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Mismatch in spectra length");
  }

  void test_ragged_workspace_fail_different_y_value() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", create_RaggedWorkspace()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", create_RaggedWorkspace(2)));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Data mismatch");
  }

  void test_ragged_workspace_fail_different_x_value() {
    Mantid::Algorithms::CompareWorkspaces alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace1", create_RaggedWorkspace()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace2", create_RaggedWorkspace(3)));
    TS_ASSERT(alg.execute());
    TS_ASSERT_DIFFERS(alg.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Data mismatch");
  }

private:
  ITableWorkspace_sptr setupTableWorkspace() {
    auto table = WorkspaceFactory::Instance().createTable();
    // One column of each type
    table->addColumn("int", "int");
    table->addColumn("uint", "uint");
    table->addColumn("long64", "int64");
    table->addColumn("size_t", "size_t");
    table->addColumn("float", "float");
    table->addColumn("double", "double");
    table->addColumn("bool", "bool");
    table->addColumn("str", "string");
    table->addColumn("V3D", "V3D");

    // A few rows
    TableRow row1 = table->appendRow();
    row1 << -1 << static_cast<uint32_t>(0) << static_cast<int64_t>(1) << static_cast<size_t>(10) << 5.5f << -9.9 << true
         << "Hello" << Mantid::Kernel::V3D();
    TableRow row2 = table->appendRow();
    row2 << 1 << static_cast<uint32_t>(2) << static_cast<int64_t>(-2) << static_cast<size_t>(100) << 0.0f << 101.0
         << false << "World" << Mantid::Kernel::V3D(-1, 3, 4);
    TableRow row3 = table->appendRow();
    row3 << 6 << static_cast<uint32_t>(3) << static_cast<int64_t>(0) << static_cast<size_t>(0) << -99.0f << 0.0 << false
         << "!" << Mantid::Kernel::V3D(1, 6, 10);

    return table;
  }

  void doGroupTest(const std::string &inputWSOne, const std::string &inputWSTwo, const std::string &expectedResult,
                   const std::map<std::string, std::string> &otherProps = std::map<std::string, std::string>(),
                   bool expectFail = false) {
    Mantid::Algorithms::CompareWorkspaces matcher;
    matcher.initialize();
    matcher.setPropertyValue("Workspace1", inputWSOne);
    matcher.setPropertyValue("Workspace2", inputWSTwo);
    std::map<std::string, std::string>::const_iterator iend = otherProps.end();
    std::map<std::string, std::string>::const_iterator itr = otherProps.begin();
    for (; itr != iend; ++itr) {
      matcher.setPropertyValue(itr->first, itr->second);
    }

    TS_ASSERT_THROWS_NOTHING(matcher.execute());
    if (expectFail) {
      TS_ASSERT_EQUALS(matcher.isExecuted(), false);
      return;
    }
    TS_ASSERT_EQUALS(matcher.isExecuted(), true);

    if (expectedResult == PROPERTY_VALUE_TRUE) {
      TS_ASSERT_EQUALS(matcher.getPropertyValue("Result"), expectedResult);
    } else {
      TS_ASSERT_DIFFERS(matcher.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);

      ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("compare_msgs");
      TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), expectedResult);
    }
  }

  void cleanupGroup(const WorkspaceGroup_sptr &group) {
    // group->deepRemoveAll();
    const std::string name = group->getName();
    Mantid::API::AnalysisDataService::Instance().deepRemoveGroup(name);
  }

private:
  Mantid::Algorithms::CompareWorkspaces checker;
  const Mantid::API::MatrixWorkspace_sptr ws1;
};
