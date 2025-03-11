// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/LoadTOFRawNexus.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;

class LoadTOFRawNexusTest : public CxxTest::TestSuite {
public:
  void test_init() {
    LoadTOFRawNexus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_confidence() {
    LoadTOFRawNexus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("Filename", "REF_L_32035.nxs");
    Mantid::Kernel::NexusDescriptor descr(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(alg.confidence(descr), 80);

    alg.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    Mantid::Kernel::NexusDescriptor descr2(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(alg.confidence(descr2), 20);

    alg.setPropertyValue("Filename", "PG3_733.nxs");
    Mantid::Kernel::NexusDescriptor descr4(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(alg.confidence(descr4), 0);
  }

  void test_exec() {
    Mantid::DataHandling::LoadTOFRawNexus ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "REF_L_32035.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    Mantid::API::MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve("outWS")););
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->blocksize(), 501);
    TS_ASSERT_EQUALS(ws->getInstrument()->getName(), "REF_L");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 77824);

    auto &spec = ws->getSpectrum(27955);
    TS_ASSERT_EQUALS(spec.getSpectrumNo(), 27956);
    TS_ASSERT_EQUALS(spec.getDetectorIDs().size(), 1);
    TS_ASSERT(spec.hasDetectorID(27955));
    auto &X = spec.x();
    auto &Y = spec.y();
    auto &E = spec.e();
    TS_ASSERT_EQUALS(X.size(), 502);
    TS_ASSERT_EQUALS(Y.size(), 501);
    TS_ASSERT_EQUALS(E.size(), 501);

    TS_ASSERT_DELTA(X[0], 0, 1e-4);
    TS_ASSERT_DELTA(X[201], 40200, 1e-4);

    // Data is pretty sparse, look for a place with something
    TS_ASSERT_DELTA(Y[94], 1.0, 1e-4);
    TS_ASSERT_DELTA(E[94], 1.0, 1e-4);

    // More data in this spectrum
    auto &spec2 = ws->getSpectrum(38019);
    TS_ASSERT_EQUALS(spec2.getSpectrumNo(), 38020);
    TS_ASSERT_EQUALS(spec2.getDetectorIDs().size(), 1);
    TS_ASSERT(spec2.hasDetectorID(38019));
    TS_ASSERT_DELTA(spec2.y()[105], 23.0, 1e-4);
    TS_ASSERT_DELTA(spec2.e()[105], sqrt(23.0), 1e-4);

    TS_ASSERT_EQUALS(ws->getAxis(1)->length(), 77824);
    TS_ASSERT_EQUALS(ws->getAxis(0)->length(), 502);
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->caption(), "Time-of-flight");
    TS_ASSERT_EQUALS(ws->YUnit(), "Counts");
    TS_ASSERT_EQUALS(ws->getTitle(), "JAA-I-103B2-1_No4Rep0");
  }

  /** Compare to LoadEventNexus */
  void xtest_compare_to_event() ///< DISABLED because it takes ~ 4 seconds.
  {
    FrameworkManager::Instance().exec("LoadTOFRawNexus", 4, "Filename", "CNCS_7860.nxs", "OutputWorkspace", "outWS");

    FrameworkManager::Instance().exec("LoadEventNexus", 4, "Filename", "CNCS_7860_event.nxs", "OutputWorkspace",
                                      "outWS_event");

    // Convert to 2D
    FrameworkManager::Instance().exec("Rebin", 8, "InputWorkspace", "outWS_event", "Params",
                                      "43000, 100, 63000, 1, 63001", "OutputWorkspace", "outWS_event_2D",
                                      "PreserveEvents", "0");

    // Compare workspaces
    auto alg = FrameworkManager::Instance().exec("CompareWorkspaces", 8, "Workspace1", "outWS", "Workspace2",
                                                 "outWS_event_2D", "Tolerance", "1e-4", "CheckAxes", "0");
    // We skip Axis check because of floating point imprecision makes a false
    // negative.

    TS_ASSERT(alg->getProperty("Result"));

    Mantid::API::MatrixWorkspace_sptr ws1, ws2;
    TS_ASSERT_THROWS_NOTHING(ws1 = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve("outWS")););
    TS_ASSERT_THROWS_NOTHING(ws2 = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve("outWS_event_2D")););
    TS_ASSERT(ws1);
    if (!ws1)
      return;
    TS_ASSERT(ws2);
    if (!ws2)
      return;

    // Quick axes check.
    TS_ASSERT_EQUALS(ws1->getAxis(0)->length(), ws2->getAxis(0)->length())
    TS_ASSERT_EQUALS(ws1->getAxis(1)->length(), ws2->getAxis(1)->length())
  }

  void test_bad_signal_fails() {
    Mantid::API::IAlgorithm_sptr alg;
    // Number points to a 2D data set
    alg = FrameworkManager::Instance().exec("LoadTOFRawNexus", 6, "Filename", "REF_L_32035.nxs", "Signal", "2",
                                            "OutputWorkspace", "outWS");
    TS_ASSERT(!alg->isExecuted());

    // Number is too big
    alg = FrameworkManager::Instance().exec("LoadTOFRawNexus", 6, "Filename", "REF_L_32035.nxs", "Signal", "6",
                                            "OutputWorkspace", "outWS");
    TS_ASSERT(!alg->isExecuted());
  }

  void test_old_file() {
    // Just need to make sure that it runs
    Mantid::DataHandling::LoadTOFRawNexus ld;
    ld.initialize();
    ld.setProperty("Filename", "REF_L_7139.nxs");
    ld.setProperty("OutputWorkspace", "REF_L_7139");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
  }

  /** Refs #3716: Different signals (binned in q-space, d-space, tof)
   * File is rather large (and slow to load) so not in SVN.
   * Test passes if file is not found.
   *
   * @param signal :: signal number to load
   * @param expectedXLength :: # of bins
   * */
  Mantid::API::MatrixWorkspace_sptr do_test_signal(int signal, size_t expectedXLength) {
    Mantid::API::AnalysisDataService::Instance().remove("outWS");
    std::string filename = "NOM_2011_09_15T16_17_30Z_histo.nxs";
    Mantid::API::FrameworkManager::Instance();
    Mantid::DataHandling::LoadTOFRawNexus ld;
    ld.initialize();
    try {
      ld.setPropertyValue("Filename", filename);
    } catch (...) {
      std::cout << "Test not completed due to missing data file " << filename << '\n';
      return Mantid::API::MatrixWorkspace_sptr();
    }
    ld.setProperty("Signal", signal);
    ld.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    Mantid::API::MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 Mantid::API::AnalysisDataService::Instance().retrieve("outWS")););
    TS_ASSERT(ws);
    if (!ws)
      return ws;
    TS_ASSERT_EQUALS(ws->getAxis(0)->length(), expectedXLength);
    TS_ASSERT_EQUALS(ws->blocksize(), expectedXLength - 1);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 99 * 8 * 128);
    return ws;
  }

  void test_signal_1() {
    Mantid::API::MatrixWorkspace_sptr ws = do_test_signal(1, 168);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->getAxis(0)->operator()(0, 0), 0, 1e-6);
    TS_ASSERT_DELTA(ws->getAxis(0)->operator()(1, 0), 1000, 1e-6);
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->unitID(), "TOF");
  }

  void test_signal_5() {
    Mantid::API::MatrixWorkspace_sptr ws = do_test_signal(5, 2501);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->getAxis(0)->operator()(0, 0), 0.02, 1e-6);
    TS_ASSERT_DELTA(ws->getAxis(0)->operator()(1, 0), 0.04, 1e-6);
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->unitID(), "MomentumTransfer");
  }

  /** Disabled because it is slow */
  void xtest_signal_6() {
    Mantid::API::MatrixWorkspace_sptr ws = do_test_signal(6, 2521);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->getAxis(0)->operator()(0, 0), 0.125, 1e-6);
    TS_ASSERT_DELTA(ws->getAxis(0)->operator()(1, 0), 0.250, 1e-6);
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->unitID(), "dSpacing");
  }

  class LoadTOFRawNexusExposed : public LoadTOFRawNexus {
  public:
    void doExec() { this->exec(); }
  };

  void xtest_SNAP_3893() ///< DISABLED because it takes > 60 seconds.
  {
    LoadTOFRawNexusExposed alg;
    alg.initialize();
    alg.setPropertyValue("Filename", "SNAP_3893.nxs");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.doExec();
    TS_ASSERT(alg.isExecuted());
  };
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadTOFRawNexusTestPerformance : public CxxTest::TestSuite {
public:
  void testDefaultLoad() {
    LoadTOFRawNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "REF_L_32035.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }
};
