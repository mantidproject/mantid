// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace DataHandling;
using namespace Indexing;

class LoadEventNexusIndexSetupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadEventNexusIndexSetupTest *createSuite() { return new LoadEventNexusIndexSetupTest(); }
  static void destroySuite(LoadEventNexusIndexSetupTest *suite) { delete suite; }

  LoadEventNexusIndexSetupTest() {
    auto instrument = std::make_shared<Instrument>();
    // Create instrument with gap in detector ID range
    for (auto detID : {1, 2, 11, 12}) {
      auto *det = new Detector("det-" + std::to_string(detID), detID, nullptr);
      instrument->add(det);
      instrument->markAsDetector(det);
    }
    auto *mon = new Detector("monitor", 666, nullptr);
    instrument->add(mon);
    instrument->markAsMonitor(mon);
    m_ws = create<WorkspaceTester>(instrument, 1, HistogramData::BinEdges(2));
  }

  void test_construct() { LoadEventNexusIndexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {}); }

  void test_makeIndexInfo_no_filter() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {});
    const auto indexInfo = indexSetup.makeIndexInfo();
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, EMPTY_INT());
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, EMPTY_INT());
    TS_ASSERT_EQUALS(indexInfo.size(), 4);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(1));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(2), SpectrumNumber(3));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(3), SpectrumNumber(4));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(0));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(1));
    TS_ASSERT_EQUALS(specDefs->at(2), SpectrumDefinition(2));
    TS_ASSERT_EQUALS(specDefs->at(3), SpectrumDefinition(3));
  }

  void test_makeIndexInfo_min_out_of_range() {
    for (const auto min : {0, 3, 13}) {
      LoadEventNexusIndexSetup indexSetup(m_ws, min, EMPTY_INT(), {});
      TS_ASSERT_THROWS(indexSetup.makeIndexInfo(), const std::out_of_range &);
    }
  }

  void test_makeIndexInfo_max_out_of_range() {
    for (const auto max : {0, 3, 13}) {
      LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), max, {});
      TS_ASSERT_THROWS(indexSetup.makeIndexInfo(), const std::out_of_range &);
    }
  }

  void test_makeIndexInfo_range_out_of_range() {
    for (const auto i : {0, 3, 13}) {
      LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {i});
      TS_ASSERT_THROWS(indexSetup.makeIndexInfo(), const std::out_of_range &);
    }
  }

  void test_makeIndexInfo_range_includes_monitor() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {666});
    TS_ASSERT_THROWS(indexSetup.makeIndexInfo(), const std::out_of_range &);
  }

  void test_makeIndexInfo_min() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 11, EMPTY_INT(), {});
    const auto indexInfo = indexSetup.makeIndexInfo();
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 11);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 12);
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(3));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(4));
    const auto specDefs = indexInfo.spectrumDefinitions();
    // Old behavior would have given detector indices 1 and 2 (instead of 2 and
    // 3), mapping to detector IDs 2 and 11, instead of the requested 11 and 12.
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(2));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(3));
  }

  void test_makeIndexInfo_min_crossing_gap() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 2, EMPTY_INT(), {});
    const auto indexInfo = indexSetup.makeIndexInfo();
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 2);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 12);
    // Note that we are NOT creating spectra for the gap between IDs 2 and 11,
    // contrary to the behavior of the old index setup code.
    TS_ASSERT_EQUALS(indexInfo.size(), 3);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(3));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(2), SpectrumNumber(4));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(1));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(2));
    TS_ASSERT_EQUALS(specDefs->at(2), SpectrumDefinition(3));
  }

  void test_makeIndexInfo_min_max() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 2, 11, {});
    const auto indexInfo = indexSetup.makeIndexInfo();
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 2);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 11);
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(3));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(1));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(2));
  }

  void test_makeIndexInfo_range() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {2, 11});
    const auto indexInfo = indexSetup.makeIndexInfo();
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 2);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 11);
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(3));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(1));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(2));
  }

  void test_makeIndexInfo_min_range() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 11, EMPTY_INT(), {1});
    const auto indexInfo = indexSetup.makeIndexInfo();
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 1);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 12);
    TS_ASSERT_EQUALS(indexInfo.size(), 3);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(1));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(3));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(2), SpectrumNumber(4));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(0));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(2));
    TS_ASSERT_EQUALS(specDefs->at(2), SpectrumDefinition(3));
  }

  void test_makeIndexInfo_min_max_range() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 2, 11, {1});
    const auto indexInfo = indexSetup.makeIndexInfo();
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 1);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 11);
    TS_ASSERT_EQUALS(indexInfo.size(), 3);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(1));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(2), SpectrumNumber(3));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(0));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(1));
    TS_ASSERT_EQUALS(specDefs->at(2), SpectrumDefinition(2));
  }

  void test_makeIndexInfo_from_bank() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {});
    const auto indexInfo = indexSetup.makeIndexInfo({"det-2", "det-12"});
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, EMPTY_INT());
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, EMPTY_INT());
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(4));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(1));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(3));
  }

  /* compare this test body with test_makeIndexInfo_from_bank. The main difference is that the instrument components are
   * specifed backwards. This is consistent with VULCAN IDF */
  void test_makeIndexInfo_from_bank_backwards() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {});
    const auto indexInfo = indexSetup.makeIndexInfo({"det-12", "det-2"}); // intentionally backwards
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, EMPTY_INT());
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, EMPTY_INT());
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    // these match the spectrum numbers of the full instrument
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(4));
    // this may actually be wrong, but it appears as though the order of the spectrum definitions match the way they
    // were requested while the spectrum numbers (just above) are always in increasing order
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(3));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(1));
  }

  void test_makeIndexInfo_from_bank_filter_ignored() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 12, EMPTY_INT(), {1});
    // This variant ignores any filter in the index/workspace setup phase,
    // consistent with old behavior. Note that a filter for min/max does however
    // apply when loading actual events in ProcessBankData (range is still
    // ignored though).
    const auto indexInfo = indexSetup.makeIndexInfo({"det-2", "det-12"});
    // Filter ignored, make sure also limits are set correctly.
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, EMPTY_INT());
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, EMPTY_INT());
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(4));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(1));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(3));
  }

  void test_makeIndexInfo_from_isis_spec_udet() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {});
    auto spec = {4, 3, 2, 1};
    auto udet = {2, 1, 12, 11};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, false);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, EMPTY_INT());
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, EMPTY_INT());
    TS_ASSERT_EQUALS(indexInfo.size(), 4);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(1));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(2), SpectrumNumber(3));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(3), SpectrumNumber(4));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(2));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(3));
    TS_ASSERT_EQUALS(specDefs->at(2), SpectrumDefinition(0));
    TS_ASSERT_EQUALS(specDefs->at(3), SpectrumDefinition(1));
  }

  void test_makeIndexInfo_from_isis_spec_udet_grouped() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {});
    auto spec = {1, 2, 1, 2};
    auto udet = {1, 2, 11, 12};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, false);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, EMPTY_INT());
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, EMPTY_INT());
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(1));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(2));
    const auto specDefs = indexInfo.spectrumDefinitions();
    SpectrumDefinition group_1_11;
    group_1_11.add(0);
    group_1_11.add(2);
    TS_ASSERT_EQUALS(specDefs->at(0), group_1_11);
    SpectrumDefinition group_2_12;
    group_2_12.add(1);
    group_2_12.add(3);
    TS_ASSERT_EQUALS(specDefs->at(1), group_2_12);
  }

  void test_makeIndexInfo_from_isis_spec_udet_unknown_detector_ids() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {});
    auto spec = {1, 2};
    auto udet = {1, 100};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, false);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, EMPTY_INT());
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, EMPTY_INT());
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(1));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(2));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(0));
    // ID 100 does not exist so SpectrumDefinition is empty
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition());
  }

  void test_makeIndexInfo_from_isis_spec_udet_min() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 3, EMPTY_INT(), {});
    auto spec = {4, 3, 2, 1};
    auto udet = {2, 1, 12, 11};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, false);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 3);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 4);
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(3));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(4));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(0));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(1));
  }

  void test_makeIndexInfo_from_isis_spec_udet_min_max() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 2, 3, {});
    auto spec = {4, 3, 2, 1};
    auto udet = {2, 1, 12, 11};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, false);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 2);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 3);
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(2));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(3));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(3));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(0));
  }

  void test_makeIndexInfo_from_isis_spec_udet_range() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {1});
    auto spec = {4, 3, 2, 1};
    auto udet = {2, 1, 12, 11};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, false);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 1);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 1);
    TS_ASSERT_EQUALS(indexInfo.size(), 1);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(1));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(2));
  }

  void test_makeIndexInfo_from_isis_spec_udet_min_max_range() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 2, 2, {1});
    auto spec = {4, 3, 2, 1};
    auto udet = {2, 1, 12, 11};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, false);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 1);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 2);
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(1));
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), SpectrumNumber(2));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(2));
    TS_ASSERT_EQUALS(specDefs->at(1), SpectrumDefinition(3));
  }

  void test_makeIndexInfo_from_isis_spec_udet_range_includes_monitor() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {1});
    auto spec = {1};
    auto udet = {666};
    TS_ASSERT_THROWS(indexSetup.makeIndexInfo({spec, udet}, false), const std::out_of_range &);
  }

  void test_makeIndexInfo_from_isis_spec_udet_monitors() {
    LoadEventNexusIndexSetup indexSetup(m_ws, EMPTY_INT(), EMPTY_INT(), {});
    auto spec = {1, 2, 3, 4, 5};
    auto udet = {1, 2, 11, 12, 666};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, true);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, EMPTY_INT());
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, EMPTY_INT());
    TS_ASSERT_EQUALS(indexInfo.size(), 1);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(5));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(4));
  }

  void test_makeIndexInfo_from_isis_spec_udet_monitors_ignores_min_max_range() {
    LoadEventNexusIndexSetup indexSetup(m_ws, 2, 3, {4});
    auto spec = {1, 2, 3, 4, 5};
    auto udet = {1, 2, 11, 12, 666};
    const auto indexInfo = indexSetup.makeIndexInfo({spec, udet}, true);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().first, 2);
    TS_ASSERT_EQUALS(indexSetup.eventIDLimits().second, 3);
    TS_ASSERT_EQUALS(indexInfo.size(), 1);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), SpectrumNumber(5));
    const auto specDefs = indexInfo.spectrumDefinitions();
    TS_ASSERT_EQUALS(specDefs->at(0), SpectrumDefinition(4));
  }

private:
  MatrixWorkspace_sptr m_ws;
};
