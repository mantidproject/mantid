// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_EXTRACTSPECTRA2TEST_H_
#define MANTID_ALGORITHMS_EXTRACTSPECTRA2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAlgorithms/ExtractSpectra2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidIndexing/IndexInfo.h"

#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/ParallelAlgorithmCreation.h"
#include "MantidTestHelpers/ParallelRunner.h"

using Mantid::Algorithms::ExtractSpectra2;
using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace DataObjects;
using namespace HistogramData;

namespace {
void run_parallel(const Parallel::Communicator &comm) {
  Indexing::IndexInfo indexInfo(1000, Parallel::StorageMode::Distributed, comm);
  auto alg = ParallelTestHelpers::create<ExtractSpectra2>(comm);
  alg->setProperty("InputWorkspace", create<Workspace2D>(indexInfo, Points(1)));
  alg->setProperty("InputWorkspaceIndexSet",
                   "0-" + std::to_string(comm.size()));
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  MatrixWorkspace_const_sptr out = alg->getProperty("OutputWorkspace");
  TS_ASSERT_EQUALS(out->storageMode(), Parallel::StorageMode::Distributed);
  if (0 % comm.size() == comm.rank()) {
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 2);
  } else {
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 1);
  }
}

boost::shared_ptr<Workspace2D> createWorkspace() {
  auto ws = create<Workspace2D>(5, Points(1));
  ws->setHistogram(0, Points{0.0}, Counts{1.0});
  ws->setHistogram(1, Points{1.0}, Counts{1.0});
  ws->setHistogram(2, Points{2.0}, Counts{1.0});
  ws->setHistogram(3, Points{3.0}, Counts{1.0});
  ws->setHistogram(4, Points{4.0}, Counts{1.0});
  return std::move(ws);
}
} // namespace

class ExtractSpectra2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractSpectra2Test *createSuite() {
    return new ExtractSpectra2Test();
  }
  static void destroySuite(ExtractSpectra2Test *suite) { delete suite; }

  void test_full() {
    auto input = createWorkspace();
    ExtractSpectra2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", std::move(input));
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 5);
  }

  void test_reorder() {
    auto input = createWorkspace();
    ExtractSpectra2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", std::move(input));
    alg.setProperty("InputWorkspaceIndexSet", "4,0-3");
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 5);
    const auto &indexInfo = ws->indexInfo();
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), 5);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), 1);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(2), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(3), 3);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(4), 4);
    TS_ASSERT_EQUALS(ws->getSpectrum(0).getSpectrumNo(), 5);
    TS_ASSERT_EQUALS(ws->getSpectrum(1).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(ws->getSpectrum(2).getSpectrumNo(), 2);
    TS_ASSERT_EQUALS(ws->getSpectrum(3).getSpectrumNo(), 3);
    TS_ASSERT_EQUALS(ws->getSpectrum(4).getSpectrumNo(), 4);
    TS_ASSERT_EQUALS(ws->x(0)[0], 4.0);
    TS_ASSERT_EQUALS(ws->x(1)[0], 0.0);
    TS_ASSERT_EQUALS(ws->x(2)[0], 1.0);
    TS_ASSERT_EQUALS(ws->x(3)[0], 2.0);
    TS_ASSERT_EQUALS(ws->x(4)[0], 3.0);
  }

  void test_extract() {
    auto input = createWorkspace();
    ExtractSpectra2 alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", std::move(input));
    alg.setProperty("InputWorkspaceIndexSet", "4,1-2");
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 3);
    const auto &indexInfo = ws->indexInfo();
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), 5);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(2), 3);
    TS_ASSERT_EQUALS(ws->x(0)[0], 4.0);
    TS_ASSERT_EQUALS(ws->x(1)[0], 1.0);
    TS_ASSERT_EQUALS(ws->x(2)[0], 2.0);
  }

  void test_parallel() { ParallelTestHelpers::runParallel(run_parallel); }

  void test_BinEdgeAxis() {
    auto input = createWorkspace();
    BinEdgeAxis *axis = new BinEdgeAxis(input->getNumberHistograms() + 1);
    for (size_t i = 0; i < axis->length(); ++i) {
      axis->setValue(i, -2. + static_cast<double>(i));
    }
    input->replaceAxis(1, axis);
    ExtractSpectra2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", std::move(input));
    alg.setProperty("InputWorkspaceIndexSet", "1-3");
    alg.setProperty("OutputWorkspace", "out");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    auto outAxis = out->getAxis(1);
    TS_ASSERT_DIFFERS(dynamic_cast<BinEdgeAxis *>(outAxis), nullptr)
    TS_ASSERT_EQUALS(outAxis->length(), 4)
    TS_ASSERT_EQUALS((*outAxis)(0), -1.)
    TS_ASSERT_EQUALS((*outAxis)(1), 0.)
    TS_ASSERT_EQUALS((*outAxis)(2), 1.)
    TS_ASSERT_EQUALS((*outAxis)(3), 2.)
  }

  void test_BinEdgeAxis_fails_with_non_contiguous_indices() {
    auto input = createWorkspace();
    BinEdgeAxis *axis = new BinEdgeAxis(input->getNumberHistograms() + 1);
    for (size_t i = 0; i < axis->length(); ++i) {
      axis->setValue(i, -2. + static_cast<double>(i));
    }
    input->replaceAxis(1, axis);
    ExtractSpectra2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", std::move(input));
    alg.setProperty("InputWorkspaceIndexSet", "1,3");
    alg.setProperty("OutputWorkspace", "out");
    TS_ASSERT_THROWS_EQUALS(
        alg.execute(), const std::invalid_argument &e, e.what(),
        std::string("Cannot extract non-contiguous set of spectra when the "
                    "vertical axis has bin edges."))
  }
};

#endif /* MANTID_ALGORITHMS_EXTRACTSPECTRA2TEST_H_ */
