// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/MaskSpectra.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"

using namespace Mantid;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;
using Mantid::DataHandling::MaskSpectra;

namespace {
std::unique_ptr<Workspace2D> makeWorkspace() {
  auto ws = create<Workspace2D>(4, Points(1));
  ws->setHistogram(0, Points{1.0}, Counts{2.0});
  ws->setHistogram(1, Points{1.1}, Counts{2.1});
  ws->setHistogram(2, Points{1.2}, Counts{2.2});
  ws->setHistogram(3, Points{1.3}, Counts{2.3});
  return ws;
}

void checkWorkspace(const MatrixWorkspace &ws) {
  TS_ASSERT_EQUALS(ws.x(0)[0], 1.0);
  TS_ASSERT_EQUALS(ws.x(1)[0], 1.1);
  TS_ASSERT_EQUALS(ws.x(2)[0], 1.2);
  TS_ASSERT_EQUALS(ws.x(3)[0], 1.3);
  TS_ASSERT_EQUALS(ws.y(0)[0], 2.0);
  TS_ASSERT_EQUALS(ws.y(1)[0], 0.0);
  TS_ASSERT_EQUALS(ws.y(2)[0], 2.2);
  TS_ASSERT_EQUALS(ws.y(3)[0], 0.0);
  TS_ASSERT_EQUALS(ws.e(0)[0], sqrt(2.0));
  TS_ASSERT_EQUALS(ws.e(1)[0], 0.0);
  TS_ASSERT_EQUALS(ws.e(2)[0], sqrt(2.2));
  TS_ASSERT_EQUALS(ws.e(3)[0], 0.0);
}

MatrixWorkspace_sptr runMaskSpectra(const MatrixWorkspace_sptr &inputWS) {
  MaskSpectra alg;
  alg.setChild(true);
  alg.initialize();
  alg.setWorkspaceInputProperties("InputWorkspace", inputWS, IndexType::WorkspaceIndex, std::vector<int64_t>{1, 3});

  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
  TS_ASSERT_THROWS_NOTHING(alg.execute(););
  TS_ASSERT(alg.isExecuted());

  MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
  return outputWS;
}
} // namespace

class MaskSpectraTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskSpectraTest *createSuite() { return new MaskSpectraTest(); }
  static void destroySuite(MaskSpectraTest *suite) { delete suite; }

  void test_init() {
    MaskSpectra alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    MatrixWorkspace_sptr inputWS = makeWorkspace();
    auto outputWS = runMaskSpectra(inputWS);

    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), inputWS->getNumberHistograms());
    checkWorkspace(*outputWS);
  }

  void test_exec_in_place() {
    MatrixWorkspace_sptr inputWS = makeWorkspace();
    MaskSpectra alg;
    alg.setChild(true);
    alg.initialize();
    alg.setWorkspaceInputProperties("InputWorkspace", inputWS, IndexType::WorkspaceIndex, std::vector<int64_t>{1, 3});

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outputWS, inputWS);
    checkWorkspace(*outputWS);
  }

  void test_exec_with_instrument() {
    MatrixWorkspace_sptr inputWS = makeWorkspace();
    InstrumentCreationHelper::addFullInstrumentToWorkspace(*inputWS, false, false, "");
    auto outputWS = runMaskSpectra(inputWS);

    TS_ASSERT(outputWS);
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), inputWS->getNumberHistograms());
    checkWorkspace(*outputWS);
    const auto &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(0), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(1), true);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(2), false);
    TS_ASSERT_EQUALS(spectrumInfo.isMasked(3), true);
  }
};
