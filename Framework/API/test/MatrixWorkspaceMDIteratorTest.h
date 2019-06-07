// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_MATRIXWORKSPACEMDITERATORTEST_H_
#define MANTID_API_MATRIXWORKSPACEMDITERATORTEST_H_

#include "MantidAPI/MatrixWorkspaceMDIterator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::HistogramData;

class MatrixWorkspaceMDIteratorTest : public CxxTest::TestSuite {
public:
  boost::shared_ptr<MatrixWorkspace> makeFakeWS() {
    boost::shared_ptr<MatrixWorkspace> ws =
        boost::make_shared<WorkspaceTester>();
    // Matrix with 4 spectra, 5 bins each
    ws->initialize(4, 6, 5);
    NumericAxis *ax1 = new NumericAxis(4);
    for (size_t wi = 0; wi < 4; wi++) {
      ax1->setValue(wi, static_cast<double>(wi) * 2.0);
      for (size_t x = 0; x < 6; x++) {
        ws->dataX(wi)[x] = static_cast<double>(x) * 2.0;
        if (x < 5) {
          ws->dataY(wi)[x] = static_cast<double>(wi * 10 + x);
          ws->dataE(wi)[x] = static_cast<double>((wi * 10 + x) * 2);
        }
      }
    }
    Instrument_sptr inst(new Instrument("TestInstrument"));
    // We get a 1:1 map by default so the detector ID should match the spectrum
    // number
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      // Create a detector for each spectra
      Detector *det =
          new Detector("pixel", static_cast<detid_t>(i), inst.get());
      inst->add(det);
      inst->markAsDetector(det);
      ws->getSpectrum(i).addDetectorID(static_cast<detid_t>(i));
    }
    ws->setInstrument(inst);
    ws->replaceAxis(1, ax1);

    return ws;
  }

  void test_iterating() {
    boost::shared_ptr<MatrixWorkspace> ws = makeFakeWS();
    std::unique_ptr<IMDIterator> it;
    TS_ASSERT_THROWS_NOTHING(it = ws->createIterator(nullptr));
    TS_ASSERT_EQUALS(it->getDataSize(), 20);
    TS_ASSERT_DELTA(it->getSignal(), 0.0, 1e-5);
    it->next();
    TS_ASSERT_DELTA(it->getSignal(), 1.0, 1e-5);
    TS_ASSERT_DELTA(it->getError(), 2.0, 1e-5);

    it->setNormalization(NoNormalization);
    TS_ASSERT_DELTA(it->getNormalizedSignal(), 1.0, 1e-5);
    TS_ASSERT_DELTA(it->getNormalizedError(), 2.0, 1e-5);
    // Area of each bin is 4.0
    it->setNormalization(VolumeNormalization);
    TS_ASSERT_DELTA(it->getNormalizedSignal(), 1.0 / 4.0, 1e-5);
    TS_ASSERT_DELTA(it->getNormalizedError(), 2.0 / 4.0, 1e-5);
    it->setNormalization(NumEventsNormalization);
    TS_ASSERT_DELTA(it->getNormalizedSignal(), 1.0, 1e-5);
    TS_ASSERT_DELTA(it->getNormalizedError(), 2.0, 1e-5);

    it->next();
    it->next();
    it->next();
    TS_ASSERT_DELTA(it->getSignal(), 4.0, 1e-5);
    TS_ASSERT_DELTA(it->getError(), 8.0, 1e-5);
    it->next();
    it->next();
    // Workspace index 1, x index 1
    TS_ASSERT_DELTA(it->getSignal(), 11.0, 1e-5);
    TS_ASSERT_DELTA(it->getError(), 22.0, 1e-5);
    TS_ASSERT_DELTA(it->getCenter()[0], 3.0, 1e-5);
    TS_ASSERT_DELTA(it->getCenter()[1], 2.0, 1e-5);
  }

  /** Create a set of iterators that can be applied in parallel */
  void test_parallel_iterators() {
    boost::shared_ptr<MatrixWorkspace> ws = makeFakeWS();
    // The number of output cannot be larger than the number of histograms
    auto it = ws->createIterators(10, nullptr);
    TS_ASSERT_EQUALS(it.size(), 4);

    // Split in 4 iterators
    auto iterators = ws->createIterators(4, nullptr);
    TS_ASSERT_EQUALS(iterators.size(), 4);

    for (size_t i = 0; i < iterators.size(); i++) {
      IMDIterator *it = iterators[i].get();
      const double i_d = static_cast<double>(i);
      // Only 5 elements per each iterator
      TS_ASSERT_EQUALS(it->getDataSize(), 5);
      TS_ASSERT_DELTA(it->getSignal(), i_d * 10 + 0.0, 1e-5);
      it->next();
      TS_ASSERT_DELTA(it->getSignal(), i_d * 10 + 1.0, 1e-5);
      TS_ASSERT_DELTA(it->getError(), i_d * 20 + 2.0, 1e-5);
      // Coordinates at X index = 1
      TS_ASSERT_DELTA(it->getCenter()[0], 3.0, 1e-5);
      // And this coordinate is the spectrum number
      TS_ASSERT_DELTA(it->getCenter()[1], i_d * 2, 1e-5);
      TS_ASSERT(it->next());
      TS_ASSERT(it->next());
      TS_ASSERT(it->next());
      TS_ASSERT(!it->next());
    }
  }

  void test_get_is_masked() {
    boost::shared_ptr<MatrixWorkspace> ws = makeFakeWS();
    auto it = ws->createIterator(nullptr);
    const auto &spectrumInfo = ws->spectrumInfo();
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), it->getIsMasked());
      it->next();
    }
  }

  void testUnequalBins() {
    boost::shared_ptr<MatrixWorkspace> ws = makeFakeWS();
    // set the first spectrum to be different
    ws->setHistogram(0, BinEdges({0, 1, 2}), Counts({10, 20}));

    // quick checks to make sure things are returning the expected values
    TS_ASSERT(!(ws->isCommonBins()));
    TS_ASSERT_THROWS(ws->blocksize(), const std::logic_error &);
    TS_ASSERT_EQUALS(ws->size(), 17);
    // Split in 4 iterators
    auto iterators = ws->createIterators(4, nullptr);
    TS_ASSERT_EQUALS(iterators.size(), 4);

    for (size_t i = 0; i < iterators.size(); i++) {
      auto it = iterators[i].get();
      const double i_d = static_cast<double>(i);
      if (i == 0) {
        // Only 5 elements per each iterator
        TS_ASSERT_EQUALS(it->getDataSize(), 2);
        TS_ASSERT_DELTA(it->getSignal(), 10.0, 1e-5);
        it->next();
        TS_ASSERT_DELTA(it->getSignal(), 20., 1e-5);
        TS_ASSERT_DELTA(it->getError(), std::sqrt(20), 1e-5);
        // Coordinates at X index = 1
        TS_ASSERT_DELTA(it->getCenter()[0], 1.5, 1e-5);
        // And this coordinate is the spectrum number
        TS_ASSERT_DELTA(it->getCenter()[1], 0.0, 1e-5);
      } else {
        // Only 5 elements per each iterator
        TS_ASSERT_EQUALS(it->getDataSize(), 5);
        TS_ASSERT_DELTA(it->getSignal(), i_d * 10 + 0.0, 1e-5);
        it->next();
        TS_ASSERT_DELTA(it->getSignal(), i_d * 10 + 1.0, 1e-5);
        TS_ASSERT_DELTA(it->getError(), i_d * 20 + 2.0, 1e-5);
        // Coordinates at X index = 1
        TS_ASSERT_DELTA(it->getCenter()[0], 3.0, 1e-5);
        // And this coordinate is the spectrum number
        TS_ASSERT_DELTA(it->getCenter()[1], i_d * 2, 1e-5);
        TS_ASSERT(it->next()); // more elements in i != 0
        TS_ASSERT(it->next());
        TS_ASSERT(it->next());
      }
      TS_ASSERT(!it->next());
    }
  }
};

#endif /* MANTID_API_MATRIXWORKSPACEMDITERATORTEST_H_ */
