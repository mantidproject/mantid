#ifndef MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_
#define MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidIndexing/IndexInfo.h"

using namespace Mantid;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;
using Mantid::Indexing::IndexInfo;

class WorkspaceCreationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceCreationTest *createSuite() {
    return new WorkspaceCreationTest();
  }
  static void destroySuite(WorkspaceCreationTest *suite) { delete suite; }

  IndexInfo make_indices() {
    IndexInfo indices(2);
    indices.setSpectrumNumbers({2, 4});
    indices.setDetectorIDs({{0}, {2, 3}});
    return indices;
  }

  void check_size(const MatrixWorkspace &ws) {
    TS_ASSERT_EQUALS(ws.getNumberHistograms(), 2);
  }

  void check_default_indices(const MatrixWorkspace &ws) {
    check_size(ws);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getSpectrumNo(), 2);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getDetectorIDs(),
                     (std::set<detid_t>{1}));
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getDetectorIDs(),
                     (std::set<detid_t>{2}));
  }

  void check_indices(const MatrixWorkspace &ws) {
    check_size(ws);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getSpectrumNo(), 2);
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getSpectrumNo(), 4);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getDetectorIDs(),
                     (std::set<detid_t>{0}));
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getDetectorIDs(),
                     (std::set<detid_t>{2, 3}));
  }

  void check_data(const MatrixWorkspace &ws) {
    TS_ASSERT_EQUALS(ws.x(0).rawData(), std::vector<double>({1, 2, 4}));
    TS_ASSERT_EQUALS(ws.x(1).rawData(), std::vector<double>({1, 2, 4}));
    TS_ASSERT_EQUALS(ws.y(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws.y(1).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws.e(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws.e(1).rawData(), std::vector<double>({0, 0}));
  }

  void test_create_size_Histogram() {
    const auto ws = create<Workspace2D>(2, Histogram(BinEdges{1, 2, 4}));
    check_default_indices(*ws);
    check_data(*ws);
  }

  void test_create_IndexInfo_Histogram() {
    const auto ws =
        create<Workspace2D>(make_indices(), Histogram(BinEdges{1, 2, 4}));
    check_indices(*ws);
    check_data(*ws);
  }

  void test_create_parent() {
    const auto parent =
        create<Workspace2D>(make_indices(), Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent);
    check_indices(*ws);
    check_data(*ws);
  }

  void test_create_parent_Histogram() {
    const auto parent =
        create<Workspace2D>(make_indices(), Histogram(BinEdges{0, 1}));
    const auto ws = create<Workspace2D>(*parent, Histogram(BinEdges{1, 2, 4}));
    check_indices(*ws);
    check_data(*ws);
  }

  void test_create_parent_same_size() {
    const auto parent =
        create<Workspace2D>(make_indices(), Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent, 2);
    // Same size -> Indices copied from parent
    check_indices(*ws);
    check_data(*ws);
  }

  void test_create_parent_size() {
    const auto parent = create<Workspace2D>(3, Histogram(BinEdges{1, 2, 4}));
    parent->getSpectrum(0).setSpectrumNo(7);
    const auto ws = create<Workspace2D>(*parent, 2);
    check_default_indices(*ws);
    check_data(*ws);
  }

  void test_create_parent_IndexInfo_same_size() {
    const auto parent = create<Workspace2D>(2, Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent, make_indices());
    // If parent has same size, data in IndexInfo is ignored
    check_default_indices(*ws);
    check_data(*ws);
  }

  void test_create_parent_IndexInfo() {
    const auto parent = create<Workspace2D>(3, Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent, make_indices());
    check_indices(*ws);
    check_data(*ws);
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

  void test_create_from_more_derived() {
    const auto parent = create<SpecialWorkspace2D>(2, Histogram(Points(1)));
    const auto ws = create<Workspace2D>(*parent);
    TS_ASSERT_EQUALS(ws->id(), "SpecialWorkspace2D");
  }

  void test_create_from_less_derived() {
    const auto parent = create<Workspace2D>(2, Histogram(Points(1)));
    const auto ws = create<SpecialWorkspace2D>(*parent);
    TS_ASSERT_EQUALS(ws->id(), "SpecialWorkspace2D");
  }

  void test_create_event_from_histo() {
    const auto parent = create<Workspace2D>(2, Histogram(BinEdges(2)));
    const auto ws = create<EventWorkspace>(*parent);
    TS_ASSERT_EQUALS(ws->id(), "EventWorkspace");
  }
};

#endif /* MANTID_DATAOBJECTS_WORKSPACECREATIONTEST_H_ */
