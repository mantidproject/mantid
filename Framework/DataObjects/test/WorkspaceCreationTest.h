#ifndef MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_
#define MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::DataObjects;
using Mantid::API::HistoWorkspace;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;

class WorkspaceCreationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceCreationTest *createSuite() {
    return new WorkspaceCreationTest();
  }
  static void destroySuite(WorkspaceCreationTest *suite) { delete suite; }

  void test_create_size_histogram() {
    auto ws = create<Workspace2D>(2, Histogram(BinEdges(3)));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->x(0).size(), 3);
    TS_ASSERT_EQUALS(ws->y(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->y(1).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->e(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->e(1).rawData(), std::vector<double>({0, 0}));
  }

  void test_create_drop_events() {
    auto eventWS = create<EventWorkspace>(1, Histogram(BinEdges(3)));
    auto ws = create<HistoWorkspace>(*eventWS);
    TS_ASSERT_EQUALS(ws->id(), "Workspace2D");
  }

  void test_EventWorkspace_MRU_is_empty() {
    const auto ws1 = create<EventWorkspace>(1, Histogram(BinEdges(3)));
    const auto ws2 = create<EventWorkspace>(*ws1, 1, Histogram(BinEdges(3)));
    TS_ASSERT_EQUALS(ws2->MRUSize(), 0);
  }
};

#endif /* MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_ */
