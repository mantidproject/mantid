#ifndef MANTID_MDALGORITHMS_MERGEMDTEST_H_
#define MANTID_MDALGORITHMS_MERGEMDTEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidMDAlgorithms/MergeMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;

using Mantid::DataObjects::MDEventsTestHelper::makeAnyMDEW;

class MergeMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MergeMDTest *createSuite() { return new MergeMDTest(); }
  static void destroySuite(MergeMDTest *suite) { delete suite; }

  void setUp() override {
    makeAnyMDEW<MDEvent<3>, 3>(2, 5., 10., 1, "mde3");
    makeAnyMDEW<MDEvent<4>, 4>(2, 5., 10., 1, "mde4");
    makeAnyMDEW<MDLeanEvent<3>, 3>(2, 5., 10., 1, "mdle3");
    // Several compatible 2D workspaces
    makeAnyMDEW<MDLeanEvent<2>, 2>(2, 0., 10., 1, "ws0");
    makeAnyMDEW<MDLeanEvent<2>, 2>(6, -5., 10., 1, "ws1");
    makeAnyMDEW<MDLeanEvent<2>, 2>(10, 0., 20., 1, "ws2");
  }

  void test_Init() {
    MergeMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_failures() {
    do_test_fails("mde3, mde4");
    do_test_fails("mde3, mdle3");
  }

  /** Run MergeMD, expect it to fail */
  void do_test_fails(const std::string &inputs) {
    MergeMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspaces", inputs));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "failed_output"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(!alg.isExecuted());
  }

  IMDEventWorkspace_sptr execute_merge(const std::string &wsName) {
    MergeMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaces", "ws0,ws1,ws2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
            wsName));
    TS_ASSERT(ws);
    return ws;
  }

  void test_exec() {
    // Name of the output workspace.
    std::string outWSName("MergeMDTest_OutputWS");

    auto ws = execute_merge(outWSName); // cannot be nullptr

    // Number of events is the sum of the 3 input ones.
    TS_ASSERT_EQUALS(ws->getNPoints(), 2 * 2 + 6 * 6 + 10 * 10);
    for (size_t d = 0; d < 2; d++) {
      IMDDimension_const_sptr dim = ws->getDimension(d);
      TS_ASSERT_DELTA(dim->getMinimum(), -5.0, 1e-3);
      TS_ASSERT_DELTA(dim->getMaximum(), +20.0, 1e-3);
    }

    TS_ASSERT_EQUALS(3, ws->getNumExperimentInfo());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_masked_data_omitted() {
    // Name of the output workspace.
    std::string outWSName("MergeMDTest_OutputWS");

    // Mask half of ws2
    FrameworkManager::Instance().exec("MaskMD", 6, "Workspace", "ws2",
                                      "Dimensions", "Axis0,Axis1", "Extents",
                                      "0,10,0,20");

    auto ws = execute_merge(outWSName); // cannot be nullptr

    // Number of events is the sum of the 3 input ones, minus the masked events
    TS_ASSERT_EQUALS(ws->getNPoints(), 2 * 2 + 6 * 6 + 10 * 10 - 5 * 10);
    for (size_t d = 0; d < 2; d++) {
      IMDDimension_const_sptr dim = ws->getDimension(d);
      TS_ASSERT_DELTA(dim->getMinimum(), -5.0, 1e-3);
      TS_ASSERT_DELTA(dim->getMaximum(), +20.0, 1e-3);
    }

    TS_ASSERT_EQUALS(3, ws->getNumExperimentInfo());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_displayNormalization() {
    // Name of the output workspace.
    std::string outWSName("MergeMDTest_OutputWS");
    auto ws0 =
        AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("ws0");
    ws0->setDisplayNormalization(API::MDNormalization::NoNormalization);
    ws0->setDisplayNormalizationHisto(
        API::MDNormalization::NumEventsNormalization);
    auto ws = execute_merge(outWSName); // cannot be nullptr

    TS_ASSERT_EQUALS(API::MDNormalization::NoNormalization,
                     ws->displayNormalization());
    TS_ASSERT_EQUALS(API::MDNormalization::NumEventsNormalization,
                     ws->displayNormalizationHisto());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_runIndex() {
    // Name of the output workspace.
    std::string outWSName("MergeMDTest_OutputWS");

    MergeMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaces", "mde3,mde3,mde3"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MDEventWorkspace3::sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3>(
            outWSName);)
    TS_ASSERT(ws);

    typename std::vector<API::IMDNode *> boxes;
    ws->getBox()->getBoxes(boxes, 10, true);

    API::IMDNode *box = boxes[0];
    TS_ASSERT_EQUALS(box->getNPoints(), 3);
    std::vector<coord_t> events;
    size_t ncols;
    box->getEventsData(events, ncols);
    // 7 columns: I, err^2, run_num, det_id, x, y, z
    TS_ASSERT_EQUALS(ncols, 7);
    // 7*3 = 21
    TS_ASSERT_EQUALS(events.size(), 21);

    // reference vector, 3 identical events except for incremented run numbers
    const std::vector<coord_t> ref = {1, 1, 0, 0, 6.25, 6.25, 6.25,
                                      1, 1, 1, 0, 6.25, 6.25, 6.25,
                                      1, 1, 2, 0, 6.25, 6.25, 6.25};

    for (auto i = 0; i < 21; i++) {
      TS_ASSERT_EQUALS(events[i], ref[i]);
    }

    // Merge merged ws with self
    MergeMD alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue(
        "InputWorkspaces", "MergeMDTest_OutputWS,MergeMDTest_OutputWS"));
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg2.execute(););
    TS_ASSERT(alg2.isExecuted());

    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3>(
            outWSName);)
    TS_ASSERT(ws);

    typename std::vector<API::IMDNode *> boxes2;
    ws->getBox()->getBoxes(boxes2, 10, true);

    API::IMDNode *box2 = boxes2[0];
    TS_ASSERT_EQUALS(box2->getNPoints(), 6);
    box2->getEventsData(events, ncols);
    // 7 columns: I, err^2, run_num, det_id, x, y, z
    TS_ASSERT_EQUALS(ncols, 7);
    // 7*6 = 42
    TS_ASSERT_EQUALS(events.size(), 42);

    // reference vector, 6 identical events except for incremented run numbers
    const std::vector<coord_t> ref2 = {
        1, 1, 0, 0, 6.25, 6.25, 6.25, 1, 1, 1, 0, 6.25, 6.25, 6.25,
        1, 1, 2, 0, 6.25, 6.25, 6.25, 1, 1, 3, 0, 6.25, 6.25, 6.25,
        1, 1, 4, 0, 6.25, 6.25, 6.25, 1, 1, 5, 0, 6.25, 6.25, 6.25};

    for (auto i = 0; i < 42; i++) {
      TS_ASSERT_EQUALS(events[i], ref2[i]);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};

#endif /* MANTID_MDALGORITHMS_MERGEMDTEST_H_ */
