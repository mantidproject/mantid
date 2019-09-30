// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MASKINSTRUMENTTEST_H_
#define MANTID_ALGORITHMS_MASKINSTRUMENTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/MaskInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

#include "MantidTestHelpers/InstrumentCreationHelper.h"

using namespace Mantid;
using namespace Algorithms;
using namespace API;
using namespace DataObjects;

namespace {
MatrixWorkspace_sptr makeWorkspace() {
  MatrixWorkspace_sptr ws = create<Workspace2D>(4, HistogramData::Points(1));
  InstrumentCreationHelper::addFullInstrumentToWorkspace(*ws, false, false, "");
  return ws;
}

MatrixWorkspace_sptr maskInstrument(const MatrixWorkspace_sptr &ws,
                                    const std::vector<int> &detectorIDs) {
  MaskInstrument alg;
  alg.setRethrows(true);
  alg.initialize();
  alg.setProperty("InputWorkspace", ws);
  alg.setPropertyValue("OutputWorkspace", "out");
  alg.setProperty("DetectorIDs", detectorIDs);
  alg.execute();
  auto out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
  AnalysisDataService::Instance().remove("out");
  return out;
}

MatrixWorkspace_sptr
maskInstrumentInplace(const MatrixWorkspace_sptr &ws,
                      const std::vector<int> &detectorIDs) {
  MaskInstrument alg;
  alg.setRethrows(true);
  alg.initialize();
  alg.setProperty("InputWorkspace", ws);
  TS_ASSERT_THROWS_NOTHING(
      alg.setPropertyValue("OutputWorkspace", "_dummy_for_inplace"));
  alg.setProperty("OutputWorkspace", ws);
  alg.setProperty("DetectorIDs", detectorIDs);
  alg.execute();
  return ws;
}
} // namespace

class MaskInstrumentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskInstrumentTest *createSuite() { return new MaskInstrumentTest(); }
  static void destroySuite(MaskInstrumentTest *suite) { delete suite; }

  void test_masking() {
    const auto in = makeWorkspace();
    const auto ws = maskInstrument(in, {1, 3});
    TS_ASSERT_DIFFERS(in, ws);
    const auto &detInfo = ws->detectorInfo();
    // Note that detector IDs in workspace start at 1, so there is an offset of
    // 1 compared to the detector indices checked here.
    TS_ASSERT_EQUALS(detInfo.isMasked(0), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(detInfo.isMasked(2), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(3), false);
  }

  void test_masking_cummulative() {
    const auto in = makeWorkspace();
    const auto ws = maskInstrument(in, {1, 3});
    const auto ws2 = maskInstrument(ws, {1, 2});
    const auto &detInfo = ws->detectorInfo();
    TS_ASSERT_EQUALS(detInfo.isMasked(0), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(detInfo.isMasked(2), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(3), false);
    const auto &detInfo2 = ws2->detectorInfo();
    TS_ASSERT_EQUALS(detInfo2.isMasked(0), true);
    TS_ASSERT_EQUALS(detInfo2.isMasked(1), true);
    TS_ASSERT_EQUALS(detInfo2.isMasked(2), true);
    TS_ASSERT_EQUALS(detInfo2.isMasked(3), false);
  }

  void test_masking_inplace() {
    const auto in = makeWorkspace();
    const auto ws = maskInstrumentInplace(in, {1, 3});
    TS_ASSERT_EQUALS(in, ws);
    const auto &detInfo = ws->detectorInfo();
    TS_ASSERT_EQUALS(detInfo.isMasked(0), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(1), false);
    TS_ASSERT_EQUALS(detInfo.isMasked(2), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(3), false);
  }

  void test_masking_inplace_cummulative() {
    const auto in = makeWorkspace();
    const auto ws = maskInstrumentInplace(in, {1, 3});
    const auto ws2 = maskInstrumentInplace(in, {1, 2});
    const auto &detInfo = ws2->detectorInfo();
    TS_ASSERT_EQUALS(detInfo.isMasked(0), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(1), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(2), true);
    TS_ASSERT_EQUALS(detInfo.isMasked(3), false);
  }

  void test_out_of_range() {
    const auto in = makeWorkspace();
    TS_ASSERT_THROWS(maskInstrumentInplace(in, {0}), const std::out_of_range &);
    TS_ASSERT_THROWS(maskInstrumentInplace(in, {5}), const std::out_of_range &);
  }
};

#endif /* MANTID_ALGORITHMS_MASKINSTRUMENTTEST_H_ */
