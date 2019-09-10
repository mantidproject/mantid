// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_LOADDNSSCDEWTEST_H_
#define MANTID_MDALGORITHMS_LOADDNSSCDEWTEST_H_

#include "LoadDNSSCDTestReference.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMDAlgorithms/LoadDNSSCD.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

bool cmp_Events(const std::vector<coord_t> &ev1,
                const std::vector<coord_t> &ev2) {
  // event1 < event2 if it has smaller det_id and dE
  assert(ev1.size() == 8);
  assert(ev2.size() == 8);
  float eps = 1.0e-07f;
  if (std::abs(ev1[3] - ev2[3]) > eps) {
    return ev1[3] < ev2[3];
  } else {
    return ev1[7] < ev2[7];
  }
}

void sort_Events(std::vector<coord_t> &events) {
  // 1. split the events vector into 8-sized chunks
  std::vector<std::vector<coord_t>> sub_events;
  auto itr = events.cbegin();
  while (itr < events.cend()) {
    sub_events.emplace_back(std::vector<coord_t>(itr, itr + 8));
    itr += 8;
  }
  // 2. sort the vector of chunks
  std::sort(sub_events.begin(), sub_events.end(), cmp_Events);

  // 3. put the sorted array back
  events.clear();
  for (auto ev : sub_events) {
    events.insert(end(events), begin(ev), end(ev));
  }
}

class LoadDNSSCDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadDNSSCDTest *createSuite() { return new LoadDNSSCDTest(); }
  static void destroySuite(LoadDNSSCDTest *suite) { delete suite; }

  LoadDNSSCDTest() : m_fileName("dn134011vana.d_dat") {}

  void test_Init() {
    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_Name() {
    LoadDNSSCD alg;
    TS_ASSERT_EQUALS(alg.name(), "LoadDNSSCD");
  }

  void test_Metadata() {
    // test whether the metadata were loaded correctly

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadAs", "raw"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);
    TS_ASSERT_EQUALS(iws->getNumExperimentInfo(), 1);

    ExperimentInfo_sptr expinfo = iws->getExperimentInfo(0);
    auto &run = expinfo->run();
    double d(1e-05);
    TS_ASSERT_DELTA(run.getPropertyValueAsType<double>("wavelength"), 4.2, d);
    TimeSeriesProperty<double> *p =
        dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Lambda"));
    TS_ASSERT_DELTA(p->firstValue(), 0.42, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Energy"));
    TS_ASSERT_DELTA(p->firstValue(), 4.640, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Speed"));
    TS_ASSERT_DELTA(p->firstValue(), 949.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("DeteRota"));
    TS_ASSERT_DELTA(p->firstValue(), -8.54, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Huber"));
    TS_ASSERT_DELTA(p->firstValue(), 79.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getProperty("Flipper_precession"));
    TS_ASSERT_DELTA(p->firstValue(), 0.970, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getProperty("Flipper_z_compensation"));
    TS_ASSERT_DELTA(p->firstValue(), 0.400, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("C_a"));
    TS_ASSERT_DELTA(p->firstValue(), 0.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("C_b"));
    TS_ASSERT_DELTA(p->firstValue(), 0.110, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("C_c"));
    TS_ASSERT_DELTA(p->firstValue(), -0.500, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("C_z"));
    TS_ASSERT_DELTA(p->firstValue(), 0.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("T1"));
    TS_ASSERT_DELTA(p->firstValue(), 295.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("T2"));
    TS_ASSERT_DELTA(p->firstValue(), 296.477, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getProperty("sample_setpoint"));
    TS_ASSERT_DELTA(p->firstValue(), 295.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Timer"));
    TS_ASSERT_DELTA(p->firstValue(), 600.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Monitor"));
    TS_ASSERT_DELTA(p->firstValue(), 8332872, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getProperty("TOF channels"));
    TS_ASSERT_DELTA(p->firstValue(), 1.0, d);
    TimeSeriesProperty<std::string> *s =
        dynamic_cast<TimeSeriesProperty<std::string> *>(
            run.getProperty("start_time"));
    TS_ASSERT_EQUALS(s->firstValue(), "2013-04-16T16:11:02");
    s = dynamic_cast<TimeSeriesProperty<std::string> *>(
        run.getProperty("stop_time"));
    TS_ASSERT_EQUALS(s->firstValue(), "2013-04-16T16:21:03");
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_DataWSStructure() {
    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaEmin", "-2.991993"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName))
    TS_ASSERT(iws)

    TS_ASSERT_EQUALS(iws->getNumDims(), 4)
    TS_ASSERT_EQUALS(iws->getNPoints(), 24)
    TS_ASSERT_EQUALS(iws->id(), "MDEventWorkspace<MDEvent,4>")

    // test box controller
    BoxController_sptr bc = iws->getBoxController();
    TS_ASSERT(bc)
    TS_ASSERT_EQUALS(bc->getNumMDBoxes().size(), 6)

    // test dimensions
    std::vector<std::string> v = {"H", "K", "L", "DeltaE"};
    for (auto i = 0; i < 4; i++) {
      auto dim = iws->getDimension(i);
      TS_ASSERT(dim)
      TS_ASSERT_EQUALS(dim->getName(), v[i])
      TS_ASSERT_EQUALS(dim->getNBins(), 5)
      double d(1.0e-05);
      TS_ASSERT_DELTA(dim->getMinimum(), -2.991993, d)
      if (i < 3) {
        TS_ASSERT_DELTA(dim->getMaximum(), 2.991993, d)
      } else {
        TS_ASSERT_DELTA(dim->getMaximum(), 4.637426, d)
      }
    }
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_RawWSStructure() {
    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadAs", "raw"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TwoThetaLimits", "20.0,55.0"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName))
    TS_ASSERT(iws)

    TS_ASSERT_EQUALS(iws->getNumDims(), 3)
    TS_ASSERT_EQUALS(iws->getNPoints(), 7)
    TS_ASSERT_EQUALS(iws->id(), "MDEventWorkspace<MDEvent,3>")

    // test box controller
    BoxController_sptr bc = iws->getBoxController();
    TS_ASSERT(bc)
    TS_ASSERT_EQUALS(bc->getNumMDBoxes().size(), 6)

    // test dimensions
    std::vector<std::string> v = {"Scattering Angle", "Omega", "TOF"};
    std::vector<double> extentMins = {
        20.0 / 2.0, 0.0, 424.668}; // this might fail if L1 will change
    std::vector<double> extentMaxs = {55.0 / 2.0, 360.0, 20000};
    for (auto i = 0; i < 3; i++) {
      auto dim = iws->getDimension(i);
      TS_ASSERT(dim)
      TS_ASSERT_EQUALS(dim->getName(), v[i])
      TS_ASSERT_EQUALS(dim->getNBins(), 5)
      double d(1.0e-03);
      TS_ASSERT_DELTA(dim->getMinimum(), extentMins[i], d)
      TS_ASSERT_DELTA(dim->getMaximum(), extentMaxs[i], d)
    }
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_DataWS() {
    // test whether the metadata were loaded correctly

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 4.77));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    std::vector<API::IMDNode *> boxes(0, nullptr);
    iws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 24 points in the data file
    TS_ASSERT_EQUALS(box->getNPoints(), 24);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 8 columns: I, err^2, run_num, det_id, h, k, l, dE
    TS_ASSERT_EQUALS(ncols, 8);
    // 8*24 = 192
    TS_ASSERT_EQUALS(events.size(), 192);
    // reference vector
    double d(1.0e-06);
    for (auto i = 0; i < 192; i++) {
      TS_ASSERT_DELTA(events[i], test_DataWS_ref[i], d);
    }

    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_RawWS() {
    // test whether the metadata were loaded correctly

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadAs", "raw"))
    // TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    std::vector<API::IMDNode *> boxes(0, nullptr);
    iws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 24 points in the data file
    TS_ASSERT_EQUALS(box->getNPoints(), 24);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 7 columns: I, err^2, run_num, det_id, theta, omega, tof
    TS_ASSERT_EQUALS(ncols, 7);
    // 7*24 = 192
    TS_ASSERT_EQUALS(events.size(), 168);
    // reference vector
    double d(1.0e-02);
    for (auto i = 0; i < 168; i++) {
      TS_ASSERT_DELTA(events[i], test_RawWS_ref[i], d);
    }

    AnalysisDataService::Instance().remove(outWSName);
  }
  void test_NormWSStructure() {
    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaEmin", "-2.991993"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);

    TS_ASSERT_EQUALS(nws->getNumDims(), 4);
    TS_ASSERT_EQUALS(nws->getNPoints(), 24);
    TS_ASSERT_EQUALS(nws->id(), "MDEventWorkspace<MDEvent,4>");

    // test box controller
    BoxController_sptr bc = nws->getBoxController();
    TS_ASSERT(bc);
    TS_ASSERT_EQUALS(bc->getNumMDBoxes().size(), 6);

    // test dimensions
    std::vector<std::string> v = {"H", "K", "L", "DeltaE"};
    for (auto i = 0; i < 4; i++) {
      auto dim = nws->getDimension(i);
      TS_ASSERT(dim);
      TS_ASSERT_EQUALS(dim->getName(), v[i]);
      TS_ASSERT_EQUALS(dim->getNBins(), 5);
      double d(1.0e-05);
      TS_ASSERT_DELTA(dim->getMinimum(), -2.991993, d);
      if (i < 3) {
        TS_ASSERT_DELTA(dim->getMaximum(), 2.991993, d);
      } else {
        TS_ASSERT_DELTA(dim->getMaximum(), 4.637426, d);
      }
    }
    AnalysisDataService::Instance().remove(normWSName);
  }

  void test_NormMonitor() {
    // test whether the metadata were loaded correctly

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 4.77));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);

    std::vector<API::IMDNode *> boxes(0, nullptr);
    nws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 24 points in the data file
    TS_ASSERT_EQUALS(box->getNPoints(), 24);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 8 columns: I, err^2, run_num, det_id, h, k, l, dE
    TS_ASSERT_EQUALS(ncols, 8);
    // 8*24 = 192
    TS_ASSERT_EQUALS(events.size(), 192);
    double d(1.0e-06);
    for (auto i = 0; i < 192; i++) {
      TS_ASSERT_DELTA(events[i], test_NormMonitor_ref[i], d);
    }

    AnalysisDataService::Instance().remove(normWSName);
  }

  void test_NormTime() {
    // test whether the metadata were loaded correctly

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "time"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 4.77));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);

    std::vector<API::IMDNode *> boxes(0, nullptr);
    nws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 24 points in the data file
    TS_ASSERT_EQUALS(box->getNPoints(), 24);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 8 columns: I, err^2, run_num, det_id, h, k, l, dE
    TS_ASSERT_EQUALS(ncols, 8);
    // 8*24 = 192
    TS_ASSERT_EQUALS(events.size(), 192);
    double d(1.0e-06);
    for (auto i = 0; i < 192; i++) {
      TS_ASSERT_DELTA(events[i], test_NormTime_ref[i], d);
    }

    AnalysisDataService::Instance().remove(normWSName);
  }

  void test_SaveHuber() {
    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");
    std::string tWSName("LoadDNSSCDTest_Huber");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SaveHuberTo", tWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    ITableWorkspace_sptr tws;
    TS_ASSERT_THROWS_NOTHING(
        tws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            tWSName));
    TS_ASSERT(tws);

    // check that workspace has 1 row and 1 column
    TS_ASSERT_EQUALS(tws->rowCount(), 1);
    TS_ASSERT_EQUALS(tws->columnCount(), 1);
    std::vector<std::string> columnNames = {"Huber(degrees)"};
    TS_ASSERT_EQUALS(tws->getColumnNames(), columnNames);

    // test the value
    TS_ASSERT_DELTA(tws->cell<double>(0, 0), 79.0, 1.0e-06);
    AnalysisDataService::Instance().remove(tWSName);
  }

  void test_LoadHuber() {
    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");
    std::string tWSName2("LoadDNSSCDTest_Huber_save");
    std::string tWSName1("LoadDNSSCDTest_Huber_load");

    // create a test table workspace
    ITableWorkspace_sptr huberWS =
        WorkspaceFactory::Instance().createTable("TableWorkspace");
    huberWS->addColumn("double", "Huber(degrees)");
    const std::vector<double> vals = {77.0, 92.0, 122.0};
    auto n = vals.size();
    for (size_t i = 0; i < n; i++) {
      huberWS->appendRow();
      huberWS->cell<double>(i, 0) = vals[i];
    }
    AnalysisDataService::Instance().add(tWSName1, huberWS);

    // run the algorithm
    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LoadHuberFrom", tWSName1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SaveHuberTo", tWSName2));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    TS_ASSERT_EQUALS(iws->getNumDims(), 4);
    // data should be replicated for each huber value
    TS_ASSERT_EQUALS(iws->getNPoints(), 24 * n);

    // Retrieve the table workspace from data service.
    ITableWorkspace_sptr tws;
    TS_ASSERT_THROWS_NOTHING(
        tws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            tWSName2));
    TS_ASSERT(tws);

    // check that workspace has 1 row and 1 column
    TS_ASSERT_EQUALS(tws->rowCount(), n);
    TS_ASSERT_EQUALS(tws->columnCount(), 1);
    std::vector<std::string> columnNames = {"Huber(degrees)"};
    TS_ASSERT_EQUALS(tws->getColumnNames(), columnNames);

    // test the values
    for (size_t i = 0; i < n; i++)
      TS_ASSERT_DELTA(tws->cell<double>(i, 0), vals[i], 1.0e-06);
    AnalysisDataService::Instance().remove(tWSName1);
    AnalysisDataService::Instance().remove(tWSName2);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_2ThetaLimits() {
    // test whether the scattering angle limits work correctly

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 4.77));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TwoThetaLimits", "20.0,55.0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    std::vector<API::IMDNode *> boxes(0, nullptr);
    iws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 7 points (the rest is outside of 2theta limits)
    TS_ASSERT_EQUALS(box->getNPoints(), 7);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 8 columns: I, err^2, run_num, det_id, h, k, l, dE
    TS_ASSERT_EQUALS(ncols, 8);
    // 8*7 = 56
    TS_ASSERT_EQUALS(events.size(), 56);
    double d(1.0e-06);
    for (auto i = 0; i < 56; i++) {
      TS_ASSERT_DELTA(events[i], test_2ThetaLimits_ref[i], d);
    }

    AnalysisDataService::Instance().remove(outWSName);

    // test the normalization workspace as well
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);
    // there are 7 points (the rest is outside of 2theta limits)
    TS_ASSERT_EQUALS(nws->getNPoints(), 7);

    AnalysisDataService::Instance().remove(normWSName);
  }

  void test_TOFWSStructure() {
    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", "dnstof.d_dat"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaEmin", "-2.991993"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    TS_ASSERT_EQUALS(iws->getNumDims(), 4);
    TS_ASSERT_EQUALS(iws->getNPoints(), 1968);
    TS_ASSERT_EQUALS(iws->id(), "MDEventWorkspace<MDEvent,4>");
    // test some metadata
    TS_ASSERT_EQUALS(iws->getNumExperimentInfo(), 1);
    ExperimentInfo_sptr expinfo = iws->getExperimentInfo(0);
    auto &run = expinfo->run();
    TimeSeriesProperty<double> *p = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getProperty("TOF channels"));
    TS_ASSERT_DELTA(p->firstValue(), 100, 1.0e-05);
    p = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getProperty("Time per channel"));
    TS_ASSERT_DELTA(p->firstValue(), 40.1, 1.0e-05);
    // test box controller
    BoxController_sptr bc = iws->getBoxController();
    TS_ASSERT(bc);
    TS_ASSERT_EQUALS(bc->getNumMDBoxes().size(), 6);

    // test dimensions
    std::vector<std::string> v = {"H", "K", "L", "DeltaE"};
    for (auto i = 0; i < 4; i++) {
      auto dim = iws->getDimension(i);
      TS_ASSERT(dim);
      TS_ASSERT_EQUALS(dim->getName(), v[i]);
      TS_ASSERT_EQUALS(dim->getNBins(), 5);
      double d(1.0e-05);
      TS_ASSERT_DELTA(dim->getMinimum(), -2.991993, d);
      if (i < 3) {
        TS_ASSERT_DELTA(dim->getMaximum(), 2.991993, d);
      } else {
        TS_ASSERT_DELTA(dim->getMaximum(), 4.637426, d);
      }
    }
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_TOFWSData() {
    // test whether the calculation for inelastic data are correct

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", "dnstof.d_dat"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 3.55));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 3.55));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 24.778));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TwoThetaLimits", "20.0,55.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaEmin", "-3.0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    std::vector<API::IMDNode *> boxes(0, NULL);
    iws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 7 points (the rest is outside of 2theta limits)
    TS_ASSERT_EQUALS(box->getNPoints(), 574);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 8 columns: I, err^2, run_num, det_id, h, k, l, dE
    TS_ASSERT_EQUALS(ncols, 8);
    // 8*574 = 4592
    TS_ASSERT_EQUALS(events.size(), 4592);
    double d(1.0e-06);
    sort_Events(events);
    for (auto i = 0; i < 82 * 8; i++) {
      TS_ASSERT_DELTA(events[i], test_TOFWSData_ref[i], d);
    }

    AnalysisDataService::Instance().remove(outWSName);

    // test the normalization workspace as well
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);
    // there are 7 histograms (the rest is outside of 2theta limits)
    TS_ASSERT_EQUALS(nws->getNPoints(), 574);

    AnalysisDataService::Instance().remove(normWSName);
  }

  void test_TOFWSDataRotateEPP() {
    // test whether the calculation for inelastic data are correct

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", "dnstof.d_dat"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 3.55));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 3.55));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 24.778));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TwoThetaLimits", "25.0,60.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DeltaEmin", "-3.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ElasticChannel", "64"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    std::vector<API::IMDNode *> boxes(0, NULL);
    iws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 7 points (the rest is outside of 2theta limits)
    TS_ASSERT_EQUALS(box->getNPoints(), 574);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 8 columns: I, err^2, run_num, det_id, h, k, l, dE
    TS_ASSERT_EQUALS(ncols, 8);
    // 8*574 = 4592
    TS_ASSERT_EQUALS(events.size(), 4592);
    double d(1.0e-06);
    sort_Events(events);
    for (auto i = 0; i < 82 * 8; i++) {
      TS_ASSERT_DELTA(events[i], test_TOFWSDataRotateEPP_ref[i], d);
    }

    AnalysisDataService::Instance().remove(outWSName);

    // test the normalization workspace as well
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);
    // there are 7 points (the rest is outside of 2theta limits)
    TS_ASSERT_EQUALS(nws->getNPoints(), 574);

    AnalysisDataService::Instance().remove(normWSName);
  }

  //-------------------- Test failure --------------------------------------
  void test_failTOF() {
    // algorithm does not load files with different number of time channels

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");
    std::string filenames = "dn134011vana.d_dat,dnstof.d_dat";

    LoadDNSSCD alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", filenames));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));

    // algorithm should throw if no valid files is provided
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

private:
  std::string m_fileName;
};

#endif /* MANTID_MDALGORITHMS_LOADDNSSCDEWEST_H_ */
