#ifndef MANTID_MDALGORITHMS_LOADDNSSCDEWTEST_H_
#define MANTID_MDALGORITHMS_LOADDNSSCDEWTEST_H_

#include "MantidKernel/Strings.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidAPI/BoxController.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidMDAlgorithms/LoadDNSSCD.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class LoadDNSSCDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadDNSSCDTest *createSuite() { return new LoadDNSSCDTest(); }
  static void destroySuite( LoadDNSSCDTest *suite ) { delete suite; }

  LoadDNSSCDTest() : m_fileName("dn134011vana.d_dat") {}

  void test_Init()
  {
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
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
    TimeSeriesProperty<double> *p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Lambda"));
    TS_ASSERT_DELTA(p->firstValue(), 0.42, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Energy"));
    TS_ASSERT_DELTA(p->firstValue(), 4.640, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Speed"));
    TS_ASSERT_DELTA(p->firstValue(), 949.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("DeteRota"));
    TS_ASSERT_DELTA(p->firstValue(), -8.54, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Huber"));
    TS_ASSERT_DELTA(p->firstValue(), 79.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Flipper_precession"));
    TS_ASSERT_DELTA(p->firstValue(), 0.970, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Flipper_z_compensation"));
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
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("sample_setpoint"));
    TS_ASSERT_DELTA(p->firstValue(), 295.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Timer"));
    TS_ASSERT_DELTA(p->firstValue(), 600.0, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("Monitor"));
    TS_ASSERT_DELTA(p->firstValue(), 8332872, d);
    p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty("TOF channels"));
    TS_ASSERT_DELTA(p->firstValue(), 1.0, d);
    TimeSeriesProperty<std::string> *s = dynamic_cast<TimeSeriesProperty<std::string> *>(run.getProperty("start_time"));
    TS_ASSERT_EQUALS(s->firstValue(), "2013-04-16T16:11:02");
    s = dynamic_cast<TimeSeriesProperty<std::string> *>(run.getProperty("stop_time"));
    TS_ASSERT_EQUALS(s->firstValue(), "2013-04-16T16:21:03");
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_DataWSStructure() {
    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    TS_ASSERT_EQUALS(iws->getNumDims(), 3);
    TS_ASSERT_EQUALS(iws->getNPoints(), 24);
    TS_ASSERT_EQUALS(iws->id(), "MDEventWorkspace<MDEvent,3>");

    // test box controller
    BoxController_sptr bc = iws->getBoxController();
    TS_ASSERT(bc);
    TS_ASSERT_EQUALS(bc->getNumMDBoxes().size(), 6);

    // test dimensions
    std::vector<std::string> v = {"H", "K", "L"};
    for (auto i=0; i<3; i++) {
        auto dim = iws->getDimension(i);
        TS_ASSERT(dim);
        TS_ASSERT_EQUALS(dim->getName(), v[i]);
        TS_ASSERT_EQUALS(dim->getNBins(), 5);
        double d(1.0e-05);
        TS_ASSERT_DELTA(dim->getMinimum(), -2.991993, d);
        TS_ASSERT_DELTA(dim->getMaximum(), 2.991993, d);
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 4.77));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("hkl1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("hkl2", "0,0,1"));
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
    // there are 24 points in the data file
    TS_ASSERT_EQUALS(box->getNPoints(), 24);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 7 columns: I, err^2, run_num, det_id, h, k, l
    TS_ASSERT_EQUALS(ncols, 7);
    // 7*24 = 168
    TS_ASSERT_EQUALS(events.size(), 168);
    // reference vector
    const std::vector<coord_t> ref = {4366, 4366, 1, 0, -0.09776273, -0.09776273, 0.10005156,
                                      31461, 31461, 1, 1, -0.15959044, -0.15959044, 0.14884006,
                                      33314, 33314, 1, 2, -0.224231616093, -0.224231616093, 0.189927174618,
                                      32369, 32369, 1, 3, -0.291194311172, -0.291194311172, 0.223000198347,
                                      31851, 31851, 1, 4, -0.359968893923, -0.359968893923, 0.247807429194,
                                      30221, 30221, 1, 5, -0.430031948245, -0.430031948245, 0.264160069153,
                                      26267, 26267, 1, 6, -0.500850251989, -0.500850251989, 0.271933664761,
                                      26788, 26788, 1, 7, -0.571884835101, -0.571884835101, 0.27106905426,
                                      29729, 29729, 1, 8, -0.642595081514, -0.642595081514, 0.26157281786,
                                      30188, 30188, 1, 9, -0.712442843555, -0.712442843555, 0.243517227652,
                                      28116, 28116, 1, 10, -0.78089653758, -0.78089653758, 0.217039697581,
                                      30277, 30277, 1, 11, -0.847435189645, -0.847435189645, 0.182341737639,
                                      20231, 20231, 1, 12, -0.911552400429, -0.911552400429, 0.13968742025,
                                      24538, 24538, 1, 13, -0.972760199244, -0.972760199244, 0.089401370527,
                                      16416, 16416, 1, 14, -1.03059275778, -1.03059275778, 0.0318662956709,
                                      20225, 20225, 1, 15, -1.08460993535, -1.08460993535, -0.0324799276578,
                                      19957, 19957, 1, 16, -1.13440062862, -1.13440062862, -0.103147585846,
                                      19570, 19570, 1, 17, -1.17958590034, -1.17958590034, -0.179598855345,
                                      20743, 20743, 1, 18, -1.21982186332, -1.21982186332, -0.261251895832,
                                      22758, 22758, 1, 19, -1.25480229757, -1.25480229757, -0.347485278364,
                                      23001, 23001, 1, 20, -1.28426098088, -1.28426098088, -0.437642714831,
                                      21836, 21836, 1, 21, -1.30797371487, -1.30797371487, -0.531038052704,
                                      23877, 23877, 1, 22, -1.32576003133, -1.32576003133, -0.626960497068,
                                      13340, 13340, 1, 23, -1.33748456564, -1.33748456564, -0.724680020201};
    double d(1.0e-06);
    for (auto i=0; i<168; i++){
       TS_ASSERT_DELTA(events[i], ref[i], d);
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);

    TS_ASSERT_EQUALS(nws->getNumDims(), 3);
    TS_ASSERT_EQUALS(nws->getNPoints(), 24);
    TS_ASSERT_EQUALS(nws->id(), "MDEventWorkspace<MDEvent,3>");

    // test box controller
    BoxController_sptr bc = nws->getBoxController();
    TS_ASSERT(bc);
    TS_ASSERT_EQUALS(bc->getNumMDBoxes().size(), 6);

    // test dimensions
    std::vector<std::string> v = {"H", "K", "L"};
    for (auto i=0; i<3; i++) {
        auto dim = nws->getDimension(i);
        TS_ASSERT(dim);
        TS_ASSERT_EQUALS(dim->getName(), v[i]);
        TS_ASSERT_EQUALS(dim->getNBins(), 5);
        double d(1.0e-05);
        TS_ASSERT_DELTA(dim->getMinimum(), -2.991993, d);
        TS_ASSERT_DELTA(dim->getMaximum(), 2.991993, d);
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 4.77));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("hkl1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("hkl2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);

    std::vector<API::IMDNode *> boxes(0, NULL);
    nws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 24 points in the data file
    TS_ASSERT_EQUALS(box->getNPoints(), 24);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 7 columns: I, err^2, run_num, det_id, h, k, l
    TS_ASSERT_EQUALS(ncols, 7);
    // 7*24 = 168
    TS_ASSERT_EQUALS(events.size(), 168);
    // reference vector
    const std::vector<coord_t> ref = {8332872, 8332872, 1, 0, -0.09776273, -0.09776273, 0.10005156,
                                      8332872, 8332872, 1, 1, -0.15959044, -0.15959044, 0.14884006,
                                      8332872, 8332872, 1, 2, -0.224231616093, -0.224231616093, 0.189927174618,
                                      8332872, 8332872, 1, 3, -0.291194311172, -0.291194311172, 0.223000198347,
                                      8332872, 8332872, 1, 4, -0.359968893923, -0.359968893923, 0.247807429194,
                                      8332872, 8332872, 1, 5, -0.430031948245, -0.430031948245, 0.264160069153,
                                      8332872, 8332872, 1, 6, -0.500850251989, -0.500850251989, 0.271933664761,
                                      8332872, 8332872, 1, 7, -0.571884835101, -0.571884835101, 0.27106905426,
                                      8332872, 8332872, 1, 8, -0.642595081514, -0.642595081514, 0.26157281786,
                                      8332872, 8332872, 1, 9, -0.712442843555, -0.712442843555, 0.243517227652,
                                      8332872, 8332872, 1, 10, -0.78089653758, -0.78089653758, 0.217039697581,
                                      8332872, 8332872, 1, 11, -0.847435189645, -0.847435189645, 0.182341737639,
                                      8332872, 8332872, 1, 12, -0.911552400429, -0.911552400429, 0.13968742025,
                                      8332872, 8332872, 1, 13, -0.972760199244, -0.972760199244, 0.089401370527,
                                      8332872, 8332872, 1, 14, -1.03059275778, -1.03059275778, 0.0318662956709,
                                      8332872, 8332872, 1, 15, -1.08460993535, -1.08460993535, -0.0324799276578,
                                      8332872, 8332872, 1, 16, -1.13440062862, -1.13440062862, -0.103147585846,
                                      8332872, 8332872, 1, 17, -1.17958590034, -1.17958590034, -0.179598855345,
                                      8332872, 8332872, 1, 18, -1.21982186332, -1.21982186332, -0.261251895832,
                                      8332872, 8332872, 1, 19, -1.25480229757, -1.25480229757, -0.347485278364,
                                      8332872, 8332872, 1, 20, -1.28426098088, -1.28426098088, -0.437642714831,
                                      8332872, 8332872, 1, 21, -1.30797371487, -1.30797371487, -0.531038052704,
                                      8332872, 8332872, 1, 22, -1.32576003133, -1.32576003133, -0.626960497068,
                                      8332872, 8332872, 1, 23, -1.33748456564, -1.33748456564, -0.724680020201};
    double d(1.0e-06);
    for (auto i=0; i<168; i++){
       TS_ASSERT_DELTA(events[i], ref[i], d);
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "time"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 4.77));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("hkl1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("hkl2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr nws;
    TS_ASSERT_THROWS_NOTHING(
        nws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            normWSName));
    TS_ASSERT(nws);

    std::vector<API::IMDNode *> boxes(0, NULL);
    nws->getBoxes(boxes, 10000, false);
    TSM_ASSERT_EQUALS("Number of boxes", boxes.size(), 1);
    API::IMDNode *box = boxes[0];
    // there are 24 points in the data file
    TS_ASSERT_EQUALS(box->getNPoints(), 24);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 7 columns: I, err^2, run_num, det_id, h, k, l
    TS_ASSERT_EQUALS(ncols, 7);
    // 7*24 = 168
    TS_ASSERT_EQUALS(events.size(), 168);
    // reference vector
    const std::vector<coord_t> ref = {600, 0, 1, 0, -0.09776273, -0.09776273, 0.10005156,
                                      600, 0, 1, 1, -0.15959044, -0.15959044, 0.14884006,
                                      600, 0, 1, 2, -0.224231616093, -0.224231616093, 0.189927174618,
                                      600, 0, 1, 3, -0.291194311172, -0.291194311172, 0.223000198347,
                                      600, 0, 1, 4, -0.359968893923, -0.359968893923, 0.247807429194,
                                      600, 0, 1, 5, -0.430031948245, -0.430031948245, 0.264160069153,
                                      600, 0, 1, 6, -0.500850251989, -0.500850251989, 0.271933664761,
                                      600, 0, 1, 7, -0.571884835101, -0.571884835101, 0.27106905426,
                                      600, 0, 1, 8, -0.642595081514, -0.642595081514, 0.26157281786,
                                      600, 0, 1, 9, -0.712442843555, -0.712442843555, 0.243517227652,
                                      600, 0, 1, 10, -0.78089653758, -0.78089653758, 0.217039697581,
                                      600, 0, 1, 11, -0.847435189645, -0.847435189645, 0.182341737639,
                                      600, 0, 1, 12, -0.911552400429, -0.911552400429, 0.13968742025,
                                      600, 0, 1, 13, -0.972760199244, -0.972760199244, 0.089401370527,
                                      600, 0, 1, 14, -1.03059275778, -1.03059275778, 0.0318662956709,
                                      600, 0, 1, 15, -1.08460993535, -1.08460993535, -0.0324799276578,
                                      600, 0, 1, 16, -1.13440062862, -1.13440062862, -0.103147585846,
                                      600, 0, 1, 17, -1.17958590034, -1.17958590034, -0.179598855345,
                                      600, 0, 1, 18, -1.21982186332, -1.21982186332, -0.261251895832,
                                      600, 0, 1, 19, -1.25480229757, -1.25480229757, -0.347485278364,
                                      600, 0, 1, 20, -1.28426098088, -1.28426098088, -0.437642714831,
                                      600, 0, 1, 21, -1.30797371487, -1.30797371487, -0.531038052704,
                                      600, 0, 1, 22, -1.32576003133, -1.32576003133, -0.626960497068,
                                      600, 0, 1, 23, -1.33748456564, -1.33748456564, -0.724680020201};
    double d(1.0e-06);
    for (auto i=0; i<168; i++){
       TS_ASSERT_DELTA(events[i], ref[i], d);
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SaveHuberTo", tWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    ITableWorkspace_sptr tws;
    TS_ASSERT_THROWS_NOTHING(
        tws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(tWSName));
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
    ITableWorkspace_sptr huberWS = WorkspaceFactory::Instance().createTable("TableWorkspace");
    huberWS->addColumn("double", "Huber(degrees)");
    const std::vector<double> vals = {77.0, 92.0, 122.0};
    auto n = vals.size();
    for (size_t i=0; i< n; i++) {
        huberWS->appendRow();
        huberWS->cell<double>(i, 0) = vals[i];
    }
    AnalysisDataService::Instance().add(tWSName1, huberWS);

    // run the algorithm
    LoadDNSSCD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", m_fileName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
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

    TS_ASSERT_EQUALS(iws->getNumDims(), 3);
    // data should be replicated for each huber value
    TS_ASSERT_EQUALS(iws->getNPoints(), 24*n);

    // Retrieve the table workspace from data service.
    ITableWorkspace_sptr tws;
    TS_ASSERT_THROWS_NOTHING(
        tws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(tWSName2));
    TS_ASSERT(tws);

    // check that workspace has 1 row and 1 column
    TS_ASSERT_EQUALS(tws->rowCount(), n);
    TS_ASSERT_EQUALS(tws->columnCount(), 1);
    std::vector<std::string> columnNames = {"Huber(degrees)"};
    TS_ASSERT_EQUALS(tws->getColumnNames(), columnNames);

    // test the values
    for (size_t i=0; i<n; i++)
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
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("a", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("b", 6.84));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("c", 4.77));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("alpha", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("beta", 90.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("gamma", 120.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OmegaOffset", -43.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("hkl1", "1,1,0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("hkl2", "0,0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("2ThetaLimits", "20.0,55.0"));
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
    TS_ASSERT_EQUALS(box->getNPoints(), 7);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 7 columns: I, err^2, run_num, det_id, h, k, l
    TS_ASSERT_EQUALS(ncols, 7);
    // 7*7 = 49
    TS_ASSERT_EQUALS(events.size(), 49);
    // reference vector
    const std::vector<coord_t> ref = {32369, 32369, 1, 3, -0.291194311172, -0.291194311172, 0.223000198347,
                                      31851, 31851, 1, 4, -0.359968893923, -0.359968893923, 0.247807429194,
                                      30221, 30221, 1, 5, -0.430031948245, -0.430031948245, 0.264160069153,
                                      26267, 26267, 1, 6, -0.500850251989, -0.500850251989, 0.271933664761,
                                      26788, 26788, 1, 7, -0.571884835101, -0.571884835101, 0.27106905426,
                                      29729, 29729, 1, 8, -0.642595081514, -0.642595081514, 0.26157281786,
                                      30188, 30188, 1, 9, -0.712442843555, -0.712442843555, 0.243517227652};
    double d(1.0e-06);
    for (auto i=0; i<49; i++){
       TS_ASSERT_DELTA(events[i], ref[i], d);
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

  void test_Load2() {
    // algorithm should load one file and skip the TOF file

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");
    std::string filenames = "dn134011vana.d_dat,dnstof.d_dat";

    LoadDNSSCD alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", filenames));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));

    // algorithm should throw only if no valid files is provided
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(
        iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            outWSName));
    TS_ASSERT(iws);

    TS_ASSERT_EQUALS(iws->getNumDims(), 3);
    TS_ASSERT_EQUALS(iws->getNPoints(), 24);
    AnalysisDataService::Instance().remove(outWSName);
  }

  //-------------------- Test failure --------------------------------------
  void test_failTOF() {
    // algorithm does not load TOF files

    std::string outWSName("LoadDNSSCDTest_OutputWS");
    std::string normWSName("LoadDNSSCDTest_OutputWS_norm");

    LoadDNSSCD alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filenames", "dnstof.d_dat"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizationWorkspace", normWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Normalization", "monitor"));

    // algorithm should throw if no valid files is provided
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());
  }

private:
  std::string m_fileName;
};


#endif /* MANTID_MDALGORITHMS_LOADDNSSCDEWEST_H_ */
