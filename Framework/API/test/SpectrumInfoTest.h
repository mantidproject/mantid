#ifndef MANTID_API_SPECTRUMINFOTEST_H_
#define MANTID_API_SPECTRUMINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/make_unique.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::Geometry;
using namespace Mantid::API;

class SpectrumInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumInfoTest *createSuite() { return new SpectrumInfoTest(); }
  static void destroySuite(SpectrumInfoTest *suite) { delete suite; }

  void test_constructor() {
    auto ws = makeWorkspace(3);
    TS_ASSERT_THROWS_NOTHING(SpectrumInfo(*ws));
  }

  void test_isMasked() {
    size_t count = 1000;
    auto ws = makeWorkspace(count);
    SpectrumInfo info(*ws);
    for (size_t i = 0; i < count; ++i)
      TS_ASSERT_EQUALS(info.isMasked(i), i % 2 == 0);
  }

  void test_isMasked_threaded() {
    size_t count = 1000;
    auto ws = makeWorkspace(count);
    SpectrumInfo info(*ws);
    // This attempts to test threading, but probably it is not really exercising
    // much.
    PARALLEL_FOR1(ws)
    for (size_t i = 0; i < count; ++i)
      TS_ASSERT_EQUALS(info.isMasked(i), i % 2 == 0);
  }

private:
  std::unique_ptr<MatrixWorkspace> makeWorkspace(size_t numSpectra) {
    auto ws = Kernel::make_unique<WorkspaceTester>();
    ws->initialize(numSpectra, 1, 1);
    auto inst = boost::make_shared<Instrument>("TestInstrument");
    ws->setInstrument(inst);
    auto &pmap = ws->instrumentParameters();
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      auto det = new Detector("pixel", static_cast<detid_t>(i), inst.get());
      inst->add(det);
      inst->markAsDetector(det);
      ws->getSpectrum(i).addDetectorID(static_cast<detid_t>(i));
      if (i % 2 == 0)
        pmap.addBool(det->getComponentID(), "masked", true);
    }
    return std::move(ws);
  }
};

#endif /* MANTID_API_SPECTRUMINFOTEST_H_ */
