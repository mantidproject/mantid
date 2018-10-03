// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidMDAlgorithms/ConvertToDiffractionMDWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class ConvertToDiffractionMDWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    ConvertToDiffractionMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /** Test various combinations of OutputDimensions parameter */
  void test_OutputDimensions_Parameter() {
    EventWorkspace_sptr in_ws = Mantid::DataObjects::MDEventsTestHelper::
        createDiffractionEventWorkspace(10);
    AnalysisDataService::Instance().addOrReplace("testInEW", in_ws);
    IAlgorithm *alg;

    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
                                            "InputWorkspace=testInEW;"
                                            "OutputWorkspace=testOutMD;"
                                            "OutputDimensions=Q (lab frame)",
                                            1);
    TS_ASSERT(alg->isExecuted());

    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "testOutMD"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getDimension(0)->getName(), "Q_lab_x");
    TS_ASSERT_EQUALS(ws->getSpecialCoordinateSystem(), Mantid::Kernel::QLab);
    // Test the frame type
    for (size_t dim = 0; dim < ws->getNumDims(); ++dim) {
      const auto &frame = ws->getDimension(dim)->getMDFrame();
      TSM_ASSERT_EQUALS("Should be convertible to a QSample frame",
                        Mantid::Geometry::QLab::QLabName, frame.name());
    }

    // But you can't add to an existing one of the wrong dimensions type, if you
    // choose Append
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
                                            "InputWorkspace=testInEW;"
                                            "OutputWorkspace=testOutMD;"
                                            "Append=1;"
                                            "OutputDimensions=HKL",
                                            1);
    TS_ASSERT(!alg->isExecuted());

    // If Append is False, then it does work. The workspace gets replaced
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
                                            "InputWorkspace=testInEW;"
                                            "OutputWorkspace=testOutMD;"
                                            "Append=0;"
                                            "OutputDimensions=HKL",
                                            1);
    TS_ASSERT(alg->isExecuted());

    // Let's remove the old workspace and try again - it will work.
    AnalysisDataService::Instance().remove("testOutMD");
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
                                            "InputWorkspace=testInEW;"
                                            "OutputWorkspace=testOutMD;"
                                            "Append=1;"
                                            "OutputDimensions=HKL",
                                            1);
    TS_ASSERT(alg->isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "testOutMD"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getDimension(0)->getName(), "H");
    TS_ASSERT_EQUALS(ws->getSpecialCoordinateSystem(), Mantid::Kernel::HKL);
    // Test the frame type
    for (size_t dim = 0; dim < ws->getNumDims(); ++dim) {
      const auto &frame = ws->getDimension(dim)->getMDFrame();
      TSM_ASSERT_EQUALS("Should be convertible to a HKL frame",
                        Mantid::Geometry::HKL::HKLName, frame.name());
    }

    AnalysisDataService::Instance().remove("testOutMD");
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
                                            "InputWorkspace=testInEW;"
                                            "OutputWorkspace=testOutMD;"
                                            "OutputDimensions=Q (sample frame)",
                                            1);
    TS_ASSERT(alg->isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "testOutMD"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getDimension(0)->getName(), "Q_sample_x");
    TS_ASSERT_EQUALS(ws->getSpecialCoordinateSystem(), Mantid::Kernel::QSample);
    // Test the frame type
    for (size_t dim = 0; dim < ws->getNumDims(); ++dim) {
      const auto &frame = ws->getDimension(dim)->getMDFrame();
      TSM_ASSERT_EQUALS("Should be convertible to a QSample frame",
                        Mantid::Geometry::QSample::QSampleName, frame.name());
    }
  }

  void do_test_MINITOPAZ(EventType type, int numEventsPer, int numPixels,
                         size_t numTimesToAdd = 1, bool OneEventPerBin = false,
                         bool MakeWorkspace2D = false) {

    EventWorkspace_sptr in_ws = Mantid::DataObjects::MDEventsTestHelper::
        createDiffractionEventWorkspace(numEventsPer, numPixels);
    if (type == WEIGHTED)
      in_ws *= 2.0;
    if (type == WEIGHTED_NOTIME) {
      for (size_t i = 0; i < in_ws->getNumberHistograms(); i++) {
        EventList &el = in_ws->getSpectrum(i);
        el.compressEvents(0.0, &el);
      }
    }

    // Rebin the workspace to have a manageable number bins
    AnalysisDataService::Instance().addOrReplace("inputWS", in_ws);
    FrameworkManager::Instance().exec("Rebin", 8, "InputWorkspace", "inputWS",
                                      "OutputWorkspace", "inputWS", "Params",
                                      "0, 500, 16e3", "PreserveEvents",
                                      MakeWorkspace2D ? "0" : "1");

    ConvertToDiffractionMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("InputWorkspace", "inputWS");
    alg.setProperty("OneEventPerBin", OneEventPerBin);
    alg.setPropertyValue("OutputWorkspace", "test_md3");
    TS_ASSERT_THROWS_NOTHING(alg.execute();)
    TS_ASSERT(alg.isExecuted())

    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "test_md3"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    size_t npoints = ws->getNPoints();
    // # of points != # of bins exactly because some are off the extents
    TS_ASSERT_LESS_THAN(399, npoints);

    TS_ASSERT_EQUALS(ws->getNumExperimentInfo(), 1);
    TSM_ASSERT("ExperimentInfo object is valid", ws->getExperimentInfo(0));

    // Add to an existing MDEW
    for (size_t i = 1; i < numTimesToAdd; i++) {
      std::cout << "Iteration " << i << '\n';
      TS_ASSERT_THROWS_NOTHING(alg.initialize())
      TS_ASSERT(alg.isInitialized())
      alg.setPropertyValue("InputWorkspace", "inputWS");
      alg.setProperty("Append", true);
      alg.setPropertyValue("OutputWorkspace", "test_md3");
      TS_ASSERT_THROWS_NOTHING(alg.execute();)
      TS_ASSERT(alg.isExecuted())

      TS_ASSERT_THROWS_NOTHING(
          ws =
              AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
                  "test_md3"));
      TS_ASSERT(ws);
      if (!ws)
        return;

      TS_ASSERT_EQUALS(
          npoints * (i + 1),
          ws->getNPoints()); // There are now twice as many points as before
      TS_ASSERT_EQUALS(ws->getNumExperimentInfo(), (i + 1));
      TSM_ASSERT("ExperimentInfo object is valid",
                 ws->getExperimentInfo(static_cast<uint16_t>(i)));
    }

    AnalysisDataService::Instance().remove("test_md3");
  }

  void test_MINITOPAZ() { do_test_MINITOPAZ(TOF, 100, 400); }

  void test_MINITOPAZ_Weighted() { do_test_MINITOPAZ(WEIGHTED, 100, 400); }

  void test_MINITOPAZ_addToExistingWorkspace() {
    do_test_MINITOPAZ(TOF, 100, 400, 2);
  }

  void test_MINITOPAZ_OneEventPerBin_fromEventWorkspace() {
    do_test_MINITOPAZ(TOF, 100, 400, 1, true, false);
  }

  void test_MINITOPAZ_OneEventPerBin_fromWorkspace2D() {
    do_test_MINITOPAZ(TOF, 100, 400, 1, true, true);
  }

  void test_MINITOPAZ_fromWorkspace2D() {
    do_test_MINITOPAZ(TOF, 100, 400, 1, false, true);
  }
};

class CTDMDWorkspaceTestPerformance : public CxxTest::TestSuite {
public:
  static CTDMDWorkspaceTestPerformance *createSuite() {
    return new CTDMDWorkspaceTestPerformance();
  }
  static void destroySuite(CTDMDWorkspaceTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    in_ws = MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer,
                                                                numPixels);

    // Rebin the workspace to have a manageable number bins
    AnalysisDataService::Instance().addOrReplace("inputWS", in_ws);
    FrameworkManager::Instance().exec("Rebin", 8, "InputWorkspace", "inputWS",
                                      "OutputWorkspace", "inputWS", "Params",
                                      "0, 500, 16e3", "PreserveEvents", "1");

    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "inputWS");
    alg.setProperty("OneEventPerBin", false);
    alg.setPropertyValue("OutputWorkspace", "test_md3");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testConvertToDiffractionMDWorkspacePerformance() {
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

private:
  ConvertToDiffractionMDWorkspace alg;
  EventWorkspace_sptr in_ws;
  int numEventsPer = 500;
  int numPixels = 2000;
};

#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */
