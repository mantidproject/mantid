#ifndef MANTID_ALGORITHMS_CLEARMASKEDSPECTRATEST_H_
#define MANTID_ALGORITHMS_CLEARMASKEDSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ClearMaskedSpectra.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

#include "MantidTestHelpers/InstrumentCreationHelper.h"

using namespace Mantid;
using namespace Algorithms;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;

namespace {
MatrixWorkspace_sptr makeWorkspace() {
  MatrixWorkspace_sptr ws =
      create<Workspace2D>(4, Histogram(Points(1), Counts{1.2}));
  InstrumentCreationHelper::addFullInstrumentToWorkspace(*ws, false, false, "");
  return ws;
}

MatrixWorkspace_sptr run(const MatrixWorkspace_sptr &ws) {
  ClearMaskedSpectra alg;
  alg.setChild(true);
  alg.initialize();
  alg.setProperty("InputWorkspace", ws);
  alg.setPropertyValue("OutputWorkspace", "dummy");
  alg.execute();
  return alg.getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr runInplace(const MatrixWorkspace_sptr &ws) {
  ClearMaskedSpectra alg;
  alg.setChild(true);
  alg.initialize();
  alg.setProperty("InputWorkspace", ws);
  alg.setPropertyValue("OutputWorkspace", "dummy");
  alg.setProperty("OutputWorkspace", ws);
  alg.execute();
  return ws;
}
} // namespace

class ClearMaskedSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClearMaskedSpectraTest *createSuite() {
    return new ClearMaskedSpectraTest();
  }
  static void destroySuite(ClearMaskedSpectraTest *suite) { delete suite; }

  void test_no_instrument_leaves_data_unchanged() {
    MatrixWorkspace_sptr ws =
        create<Workspace2D>(4, Histogram(Points(1), Counts{1.2}));
    ClearMaskedSpectra alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_dummy_for_inplace"));
    alg.setProperty("OutputWorkspace", ws);
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_EQUALS(ws->y(0)[0], 1.2);
    TS_ASSERT_EQUALS(ws->y(1)[0], 1.2);
    TS_ASSERT_EQUALS(ws->y(2)[0], 1.2);
    TS_ASSERT_EQUALS(ws->y(3)[0], 1.2);
  }

  void test_no_masking() {
    auto in = makeWorkspace();
    auto out = run(in);
    TS_ASSERT_DIFFERS(in, out);
    TS_ASSERT_EQUALS(out->y(0)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(1)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(2)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(3)[0], 1.2);
  }

  void test_no_masking_inplace() {
    auto in = makeWorkspace();
    auto out = runInplace(in);
    TS_ASSERT_EQUALS(in, out);
    TS_ASSERT_EQUALS(out->y(0)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(1)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(2)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(3)[0], 1.2);
  }

  void test_masking() {
    auto in = makeWorkspace();
    auto &detInfo = in->mutableDetectorInfo();
    detInfo.setMasked(1, true);
    auto out = run(in);
    TS_ASSERT_DIFFERS(in, out);
    TS_ASSERT_EQUALS(out->y(0)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(1)[0], 0.0);
    TS_ASSERT_EQUALS(out->y(2)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(3)[0], 1.2);
  }

  void test_masking_inplace() {
    auto in = makeWorkspace();
    auto &detInfo = in->mutableDetectorInfo();
    detInfo.setMasked(1, true);
    auto out = runInplace(in);
    TS_ASSERT_EQUALS(in, out);
    TS_ASSERT_EQUALS(out->y(0)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(1)[0], 0.0);
    TS_ASSERT_EQUALS(out->y(2)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(3)[0], 1.2);
  }

  void test_does_not_clear_partially_masked() {
    auto in = makeWorkspace();
    in->getSpectrum(1).addDetectorID(3);
    auto &detInfo = in->mutableDetectorInfo();
    detInfo.setMasked(1, true);
    auto out = run(in);
    TS_ASSERT_DIFFERS(in, out);
    TS_ASSERT_EQUALS(out->y(0)[0], 1.2);
    // Only one of the associated detector IDs is masked, data preserved.
    TS_ASSERT_EQUALS(out->y(1)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(2)[0], 1.2);
    TS_ASSERT_EQUALS(out->y(3)[0], 1.2);
  }
};

#endif /* MANTID_ALGORITHMS_CLEARMASKEDSPECTRATEST_H_ */
