// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTypes/SpectrumDefinition.h"

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"

using namespace Mantid;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;
using namespace Indexing;

class WorkspaceCreationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspaceCreationTest *createSuite() { return new WorkspaceCreationTest(); }
  static void destroySuite(WorkspaceCreationTest *suite) { delete suite; }

  WorkspaceCreationTest() {
    // 1 bank, 2x2 pixels, IDs 4,5,6,7
    m_instrument = ComponentCreationHelper::createTestInstrumentRectangular(1, 2);
  }

  IndexInfo make_indices() {
    IndexInfo indices(2);
    indices.setSpectrumNumbers({2, 4});
    std::vector<SpectrumDefinition> specDefs(2);
    specDefs[0].add(0);
    specDefs[1].add(2);
    specDefs[1].add(3);
    indices.setSpectrumDefinitions(specDefs);
    return indices;
  }

  IndexInfo make_indices_no_detectors() {
    IndexInfo indices(2);
    indices.setSpectrumNumbers({2, 4});
    indices.setSpectrumDefinitions(std::vector<SpectrumDefinition>(2));
    return indices;
  }

  Histogram make_data() {
    BinEdges edges{1, 2, 4};
    Counts counts{3.0, 5.0};
    CountStandardDeviations deviations{2.0, 4.0};
    return Histogram{edges, counts, deviations};
  }

  void check_size(const MatrixWorkspace &ws) { TS_ASSERT_EQUALS(ws.getNumberHistograms(), 2); }

  void check_default_indices(const MatrixWorkspace &ws) {
    check_size(ws);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getSpectrumNo(), 2);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getDetectorIDs(), (std::set<detid_t>{}));
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getDetectorIDs(), (std::set<detid_t>{}));
  }

  void check_indices(const MatrixWorkspace &ws) {
    check_size(ws);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getSpectrumNo(), 2);
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getSpectrumNo(), 4);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getDetectorIDs(), (std::set<detid_t>{4}));
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getDetectorIDs(), (std::set<detid_t>{6, 7}));
  }

  void check_indices_no_detectors(const MatrixWorkspace &ws) {
    check_size(ws);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getSpectrumNo(), 2);
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getSpectrumNo(), 4);
    TS_ASSERT_EQUALS(ws.getSpectrum(0).getDetectorIDs(), (std::set<detid_t>{}));
    TS_ASSERT_EQUALS(ws.getSpectrum(1).getDetectorIDs(), (std::set<detid_t>{}));
  }

  void check_zeroed_data(const MatrixWorkspace &ws) {
    TS_ASSERT_EQUALS(ws.x(0).rawData(), std::vector<double>({1, 2, 4}));
    TS_ASSERT_EQUALS(ws.x(1).rawData(), std::vector<double>({1, 2, 4}));
    TS_ASSERT_EQUALS(ws.y(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws.y(1).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws.e(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws.e(1).rawData(), std::vector<double>({0, 0}));
  }

  void check_instrument(const MatrixWorkspace &ws) {
    TS_ASSERT_EQUALS(ws.getInstrument()->baseInstrument(), m_instrument);
  }

  void check_data(const MatrixWorkspace &ws) {
    TS_ASSERT_EQUALS(ws.x(0).rawData(), std::vector<double>({1, 2, 4}));
    TS_ASSERT_EQUALS(ws.x(1).rawData(), std::vector<double>({1, 2, 4}));
    TS_ASSERT_EQUALS(ws.y(0).rawData(), std::vector<double>({3, 5}));
    TS_ASSERT_EQUALS(ws.y(1).rawData(), std::vector<double>({3, 5}));
    TS_ASSERT_EQUALS(ws.e(0).rawData(), std::vector<double>({2, 4}));
    TS_ASSERT_EQUALS(ws.e(1).rawData(), std::vector<double>({2, 4}));
  }

  void test_create_size_Histogram() {
    const auto ws = create<Workspace2D>(2, Histogram(BinEdges{1, 2, 4}));
    check_default_indices(*ws);
    check_zeroed_data(*ws);
  }

  void test_create_size_fully_specified_Histogram() {
    std::unique_ptr<Workspace2D> ws;
    TS_ASSERT_THROWS_NOTHING(ws = create<Workspace2D>(2, make_data()))
    check_data(*ws);
  }

  void test_create_parent_size_fully_specified_Histogram() {
    const auto parent = create<Workspace2D>(make_indices_no_detectors(), Histogram(BinEdges{-1, 0, 2}));
    std::unique_ptr<Workspace2D> ws;
    TS_ASSERT_THROWS_NOTHING(ws = create<Workspace2D>(*parent, 2, make_data()))
    check_indices_no_detectors(*ws);
    check_data(*ws);
  }

  void test_create_IndexInfo_Histogram() {
    const auto ws = create<Workspace2D>(make_indices_no_detectors(), Histogram(BinEdges{1, 2, 4}));
    check_indices_no_detectors(*ws);
    check_zeroed_data(*ws);
  }

  void test_create_bad_IndexInfo_Histogram_no_instrument() {
    // No instrument, so spectrum definitions created by make_indices are bad.
    TS_ASSERT_THROWS(create<Workspace2D>(make_indices(), Histogram(BinEdges{1, 2, 4})), const std::invalid_argument &);
  }

  void test_create_Instrument_size_Histogram() {
    const auto ws = create<Workspace2D>(m_instrument, 2, Histogram(BinEdges{1, 2, 4}));
    check_default_indices(*ws);
    check_zeroed_data(*ws);
    check_instrument(*ws);
  }

  void test_create_Instrument_IndexInfo_Histogram() {
    const auto ws = create<Workspace2D>(m_instrument, make_indices(), Histogram(BinEdges{1, 2, 4}));
    check_indices(*ws);
    check_zeroed_data(*ws);
    check_instrument(*ws);
  }

  void test_create_parent() {
    const auto parent = create<Workspace2D>(m_instrument, make_indices(), Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent);
    check_indices(*ws);
    check_zeroed_data(*ws);
    check_instrument(*ws);
  }

  void test_create_parent_distribution_flag() {
    Histogram hist(BinEdges{1, 2}, Counts{1});
    Histogram dist(BinEdges{1, 2}, Frequencies{1});
    const auto wsHist = create<Workspace2D>(m_instrument, make_indices(), hist);
    const auto wsDist = create<Workspace2D>(m_instrument, make_indices(), dist);
    // Distribution flag inherited from parent if not explicitly specified
    TS_ASSERT(!create<Workspace2D>(*wsHist)->isDistribution());
    TS_ASSERT(create<Workspace2D>(*wsDist)->isDistribution());
    TS_ASSERT(!create<Workspace2D>(*wsHist, BinEdges{1, 2})->isDistribution());
    TS_ASSERT(create<Workspace2D>(*wsDist, BinEdges{1, 2})->isDistribution());
    // Passing a full Histogram explicitly specifies the YMode, i.e.,
    // distribution flag.
    TS_ASSERT(!create<Workspace2D>(*wsHist, hist)->isDistribution());
    TS_ASSERT(!create<Workspace2D>(*wsDist, hist)->isDistribution());
    TS_ASSERT(create<Workspace2D>(*wsHist, dist)->isDistribution());
    TS_ASSERT(create<Workspace2D>(*wsDist, dist)->isDistribution());
  }

  void test_create_parent_without_logs() {
    const auto parent = create<Workspace2D>(m_instrument, make_indices(), Histogram(BinEdges{1, 2, 4}));

    const std::string &name0 = "Log2";
    parent->mutableRun().addProperty(name0, 3.2);
    const std::string &name1 = "Log4";
    const std::string &value1 = "6.4a";
    parent->mutableRun().addProperty(name1, value1);
    const auto ws = create<Workspace2D>(*parent);
    TS_ASSERT_EQUALS(&parent->run(), &ws->run());
    ws->setSharedRun(Kernel::make_cow<Run>());
    check_indices(*ws);
    check_zeroed_data(*ws);
    check_instrument(*ws);
    TS_ASSERT_EQUALS(ws->run().getProperties().size(), 0);
  }

  void test_create_parent_varying_bins() {
    auto parent = create<Workspace2D>(make_indices_no_detectors(), make_data());
    const double binShift = -0.54;
    parent->mutableX(1) += binShift;
    const auto ws = create<Workspace2D>(*parent);
    check_indices_no_detectors(*ws);
    TS_ASSERT_EQUALS(ws->x(0).rawData(), std::vector<double>({1, 2, 4}));
    TS_ASSERT_EQUALS(ws->x(1).rawData(), std::vector<double>({1 + binShift, 2 + binShift, 4 + binShift}));
    TS_ASSERT_EQUALS(ws->y(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->y(1).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->e(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->e(1).rawData(), std::vector<double>({0, 0}));
  }

  void test_create_parent_varying_bins_from_event() {
    auto parent = create<EventWorkspace>(make_indices_no_detectors(), BinEdges{1, 2, 4});
    const double binShift = -0.54;
    parent->mutableX(1) += binShift;
    const auto ws = create<EventWorkspace>(*parent);
    check_indices_no_detectors(*ws);
    TS_ASSERT_EQUALS(ws->x(0).rawData(), std::vector<double>({1, 2, 4}));
    TS_ASSERT_EQUALS(ws->x(1).rawData(), std::vector<double>({1 + binShift, 2 + binShift, 4 + binShift}));
    TS_ASSERT_EQUALS(ws->y(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->y(1).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->e(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->e(1).rawData(), std::vector<double>({0, 0}));
  }

  void test_create_parent_Histogram() {
    const auto parent = create<Workspace2D>(m_instrument, make_indices(), Histogram(BinEdges{0, 1}));
    const auto ws = create<Workspace2D>(*parent, Histogram(BinEdges{1, 2, 4}));
    check_indices(*ws);
    check_zeroed_data(*ws);
    check_instrument(*ws);
  }

  void test_create_parent_same_size() {
    const auto parent = create<Workspace2D>(m_instrument, make_indices(), Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent, 2, parent->histogram(0));
    // Same size -> Indices copied from parent
    check_indices(*ws);
    check_zeroed_data(*ws);
    check_instrument(*ws);
  }

  void test_create_parent_size() {
    const auto parent = create<Workspace2D>(3, Histogram(BinEdges{1, 2, 4}));
    parent->getSpectrum(0).setSpectrumNo(7);
    const auto ws = create<Workspace2D>(*parent, 2, parent->histogram(0));
    check_default_indices(*ws);
    check_zeroed_data(*ws);
  }

  void test_create_parent_same_size_does_not_ignore_IndexInfo_no_instrument() {
    const auto parent = create<Workspace2D>(2, Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent, make_indices_no_detectors(), parent->histogram(0));
    // Even if parent has same size data in IndexInfo should not be ignored
    // since it is given explicitly.
    check_indices_no_detectors(*ws);
    check_zeroed_data(*ws);
  }

  void test_create_parent_same_size_does_not_ignore_IndexInfo() {
    auto parentIndices = make_indices();
    parentIndices.setSpectrumNumbers({666, 1});
    const auto parent = create<Workspace2D>(m_instrument, parentIndices, Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent, make_indices(), parent->histogram(0));
    // Even if parent has same size data in IndexInfo should not be ignored
    // since it is given explicitly.
    check_indices(*ws);
    check_zeroed_data(*ws);
  }

  void test_create_parent_bad_IndexInfo_no_instrument() {
    const auto parent = create<Workspace2D>(3, Histogram(BinEdges{1, 2, 4}));
    // parent has no instrument set, so spectrum definitions created by
    // make_indices are bad.
    TS_ASSERT_THROWS(create<Workspace2D>(*parent, make_indices(), (BinEdges{1, 2, 4})), const std::invalid_argument &);
  }

  void test_create_parent_IndexInfo() {
    const auto parent = create<Workspace2D>(m_instrument, 3, Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<Workspace2D>(*parent, make_indices(), parent->histogram(0));
    check_indices(*ws);
    check_zeroed_data(*ws);
  }

  void test_create_parent_size_edges_from_event() {
    const auto parent = create<EventWorkspace>(make_indices_no_detectors(), Histogram(BinEdges{1, 2, 4}));
    std::unique_ptr<EventWorkspace> ws;
    TS_ASSERT_THROWS_NOTHING(ws = create<EventWorkspace>(*parent, 2, parent->binEdges(0)))
    TS_ASSERT_EQUALS(ws->id(), "EventWorkspace");
    check_indices_no_detectors(*ws);
    check_zeroed_data(*ws);
  }

  void test_create_parent_numeric_vertical_axis() {
    constexpr size_t parentNhist{3};
    const auto parent = create<Workspace2D>(parentNhist, Histogram(Points{1}));
    std::vector<double> vec{-1.5, -0.5, 2.3};
    auto parentAxis = std::make_unique<NumericAxis>(vec);
    parent->replaceAxis(1, std::move(parentAxis));
    constexpr size_t nhist{2};
    const auto ws = create<Workspace2D>(*parent, nhist, parent->histogram(0));
    auto axis = ws->getAxis(1);
    TS_ASSERT_DIFFERS(dynamic_cast<NumericAxis *>(axis), nullptr)
    TS_ASSERT_EQUALS(axis->length(), nhist);
  }

  void test_create_parent_bin_edge_vertical_axis() {
    constexpr size_t parentNhist{3};
    const auto parent = create<Workspace2D>(parentNhist, Histogram(Points{1}));
    std::vector<double> vec{-1.5, -0.5, 2.3, 3.4};
    auto parentAxis = std::make_unique<BinEdgeAxis>(vec);
    parent->replaceAxis(1, std::move(parentAxis));
    constexpr size_t nhist{2};
    const auto ws = create<Workspace2D>(*parent, nhist, parent->histogram(0));
    auto axis = ws->getAxis(1);
    TS_ASSERT_DIFFERS(dynamic_cast<BinEdgeAxis *>(axis), nullptr)
    TS_ASSERT_EQUALS(axis->length(), nhist + 1);
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

  void test_create_event_from_event() {
    const auto parent = create<Workspace2D>(2, Histogram(BinEdges{1, 2, 4}));
    const auto ws = create<EventWorkspace>(*parent);
    TS_ASSERT_EQUALS(ws->id(), "EventWorkspace");
    check_zeroed_data(*ws);
  }

  void test_create_event_from_edges() {
    std::unique_ptr<EventWorkspace> ws;
    TS_ASSERT_THROWS_NOTHING(ws = create<EventWorkspace>(2, BinEdges{1, 2, 4}))
    TS_ASSERT_EQUALS(ws->id(), "EventWorkspace");
    check_zeroed_data(*ws);
  }

  void test_create_ragged_from_parent() {
    const auto parent = create<Workspace2D>(2, Histogram(BinEdges(3)));
    MantidVec x_data{1., 2., 3., 4.};
    MantidVec y_data{1., 2., 3.};
    MantidVec e_data{1., 1., 1.};
    HistogramBuilder builder;
    builder.setX(x_data);
    builder.setY(y_data);
    builder.setE(e_data);
    parent->setHistogram(1, builder.build());
    TS_ASSERT(parent->isRaggedWorkspace());

    const auto ws = create<Workspace2D>(*parent);
    ;
    TS_ASSERT(ws->isRaggedWorkspace());
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->x(0).size(), 3);
    TS_ASSERT_EQUALS(ws->x(1).size(), 4);
    TS_ASSERT_EQUALS(ws->y(0).size(), 2);
    TS_ASSERT_EQUALS(ws->y(1).size(), 3);
    TS_ASSERT_EQUALS(ws->x(0).rawData(), std::vector<double>({0, 0, 0}));
    TS_ASSERT_EQUALS(ws->x(1).rawData(), std::vector<double>({1, 2, 3, 4}));
    TS_ASSERT_EQUALS(ws->y(0).rawData(), std::vector<double>({0, 0}));
    TS_ASSERT_EQUALS(ws->y(1).rawData(), std::vector<double>({0, 0, 0})); // y is expected to be zeroed
  }

private:
  std::shared_ptr<Geometry::Instrument> m_instrument;
};
