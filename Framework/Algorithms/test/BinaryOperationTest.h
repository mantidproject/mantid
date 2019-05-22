// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef BINARYOPERATIONTEST_H_
#define BINARYOPERATIONTEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ParallelAlgorithmCreation.h"
#include "MantidTestHelpers/ParallelRunner.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_const_sptr;

class BinaryOpHelper : public Mantid::Algorithms::BinaryOperation {
public:
  /// function to return a name of the algorithm, must be overridden in all
  /// algorithms
  const std::string name() const override { return "BinaryOpHelper"; }
  /// function to return a version of the algorithm, must be overridden in all
  /// algorithms
  int version() const override { return 1; }
  /// function to return a category of the algorithm. A default implementation
  /// is provided
  const std::string category() const override { return "Helper"; }
  /// function to return the summary of the algorithm. A default implementation
  /// is provided.
  const std::string summary() const override { return "Summary of this test."; }

  std::string checkSizeCompatibility(const MatrixWorkspace_const_sptr ws1,
                                     const MatrixWorkspace_const_sptr ws2) {
    m_lhs = ws1;
    m_rhs = ws2;
    m_lhsBlocksize = ws1->blocksize();
    m_rhsBlocksize = ws2->blocksize();
    BinaryOperation::checkRequirements();
    return BinaryOperation::checkSizeCompatibility(ws1, ws2);
  }

private:
  // Unhide base class method to avoid Intel compiler warning
  using BinaryOperation::checkSizeCompatibility;
  // Overridden BinaryOperation methods
  void performBinaryOperation(const HistogramData::Histogram &,
                              const HistogramData::Histogram &,
                              HistogramData::HistogramY &,
                              HistogramData::HistogramE &) override {}
  void performBinaryOperation(const HistogramData::Histogram &, const double,
                              const double, HistogramData::HistogramY &,
                              HistogramData::HistogramE &) override {}
};

namespace {
void run_parallel(const Parallel::Communicator &comm,
                  const Parallel::StorageMode storageMode) {
  using namespace Parallel;
  auto alg = ParallelTestHelpers::create<BinaryOpHelper>(comm);
  if (comm.rank() == 0 || storageMode != StorageMode::MasterOnly) {
    Indexing::IndexInfo indexInfo(100, storageMode, comm);
    alg->setProperty("LHSWorkspace",
                     create<Workspace2D>(indexInfo, HistogramData::Points(1)));
    alg->setProperty("RHSWorkspace",
                     create<Workspace2D>(indexInfo, HistogramData::Points(1)));
  }
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  MatrixWorkspace_const_sptr out = alg->getProperty("OutputWorkspace");
  if (comm.rank() == 0 || storageMode != StorageMode::MasterOnly) {
    TS_ASSERT_EQUALS(out->storageMode(), storageMode);
  } else {
    TS_ASSERT_EQUALS(out, nullptr);
  }
}

void run_parallel_mismatch_fail(const Parallel::Communicator &comm,
                                const Parallel::StorageMode storageModeA,
                                const Parallel::StorageMode storageModeB) {
  using namespace Parallel;
  auto alg = ParallelTestHelpers::create<BinaryOpHelper>(comm);
  if (comm.rank() == 0 || storageModeA != StorageMode::MasterOnly) {
    Indexing::IndexInfo indexInfo(100, storageModeA, comm);
    alg->setProperty("LHSWorkspace",
                     create<Workspace2D>(indexInfo, HistogramData::Points(1)));
  }
  if (comm.rank() == 0 || storageModeB != StorageMode::MasterOnly) {
    Indexing::IndexInfo indexInfo(100, storageModeB, comm);
    alg->setProperty("RHSWorkspace",
                     create<Workspace2D>(indexInfo, HistogramData::Points(1)));
  }
  if (comm.size() > 1) {
    TS_ASSERT_THROWS_EQUALS(
        alg->execute(), const std::runtime_error &e, std::string(e.what()),
        "Algorithm does not support execution with input workspaces of the "
        "following storage types: \nLHSWorkspace " +
            toString(storageModeA) + "\nRHSWorkspace " +
            toString(storageModeB) + "\n.");
  } else {
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_const_sptr out = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(out->storageMode(), storageModeA);
  }
}

void run_parallel_single_value(const Parallel::Communicator &comm,
                               const Parallel::StorageMode storageModeA,
                               const Parallel::StorageMode storageModeB) {
  using namespace Parallel;
  auto alg = ParallelTestHelpers::create<BinaryOpHelper>(comm);
  if (comm.rank() == 0 || storageModeA != StorageMode::MasterOnly) {
    Indexing::IndexInfo indexInfo(100, storageModeA, comm);
    alg->setProperty("LHSWorkspace",
                     create<Workspace2D>(indexInfo, HistogramData::Points(1)));
  }
  if (comm.rank() == 0 || storageModeB != StorageMode::MasterOnly) {
    Indexing::IndexInfo indexInfo(1, storageModeB, comm);
    alg->setProperty("RHSWorkspace", create<WorkspaceSingleValue>(
                                         indexInfo, HistogramData::Points(1)));
  }
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  MatrixWorkspace_const_sptr out = alg->getProperty("OutputWorkspace");
  if (comm.rank() == 0 || storageModeA != StorageMode::MasterOnly) {
    TS_ASSERT_EQUALS(out->storageMode(), storageModeA);
  } else {
    TS_ASSERT_EQUALS(out, nullptr);
  }
}

void run_parallel_AllowDifferentNumberSpectra_fail(
    const Parallel::Communicator &comm,
    const Parallel::StorageMode storageMode) {
  using namespace Parallel;
  auto alg = ParallelTestHelpers::create<BinaryOpHelper>(comm);
  if (comm.rank() == 0 || storageMode != StorageMode::MasterOnly) {
    Indexing::IndexInfo indexInfo(100, storageMode, comm);
    alg->setProperty("LHSWorkspace",
                     create<Workspace2D>(indexInfo, HistogramData::Points(1)));
    alg->setProperty("RHSWorkspace",
                     create<Workspace2D>(indexInfo, HistogramData::Points(1)));
  } else {
    alg->setProperty("LHSWorkspace",
                     Kernel::make_unique<Workspace2D>(StorageMode::MasterOnly));
    alg->setProperty("RHSWorkspace",
                     Kernel::make_unique<Workspace2D>(StorageMode::MasterOnly));
  }
  alg->setProperty("AllowDifferentNumberSpectra", true);
  if (comm.size() > 1) {
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  } else {
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_const_sptr out = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(out->storageMode(), storageMode);
  }
}
} // namespace

class BinaryOperationTest : public CxxTest::TestSuite {
public:
  void testcheckSizeCompatibility1D1D() {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 =
        WorkspaceCreationHelper::create1DWorkspaceFib(10, true);
    Workspace2D_sptr work_in2 =
        WorkspaceCreationHelper::create1DWorkspaceFib(20, true);
    Workspace2D_sptr work_in3 =
        WorkspaceCreationHelper::create1DWorkspaceFib(10, true);
    Workspace2D_sptr work_in4 =
        WorkspaceCreationHelper::create1DWorkspaceFib(5, true);
    Workspace2D_sptr work_in5 =
        WorkspaceCreationHelper::create1DWorkspaceFib(3, true);
    Workspace2D_sptr work_in6 =
        WorkspaceCreationHelper::create1DWorkspaceFib(1, true);
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in6).empty());
  }

  void testcheckSizeCompatibility2D1D() {
    // Register the workspace in the data service
    const bool isHistogram(true);
    Workspace2D_sptr work_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(10, 10, isHistogram);
    Workspace2D_sptr work_in2 =
        WorkspaceCreationHelper::create1DWorkspaceFib(20, true);
    Workspace2D_sptr work_in3 =
        WorkspaceCreationHelper::create1DWorkspaceFib(10, true);
    Workspace2D_sptr work_in4 =
        WorkspaceCreationHelper::create1DWorkspaceFib(5, true);
    Workspace2D_sptr work_in5 =
        WorkspaceCreationHelper::create1DWorkspaceFib(3, true);
    Workspace2D_sptr work_in6 =
        WorkspaceCreationHelper::create1DWorkspaceFib(1, true);
    MatrixWorkspace_sptr work_inEvent1 =
        WorkspaceCreationHelper::createEventWorkspace(10, 1);
    // will not pass x array does not match
    MatrixWorkspace_sptr work_inEvent2 =
        WorkspaceCreationHelper::createEventWorkspace(1, 10);
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in6).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_inEvent1).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_inEvent2).empty());
  }

  void testcheckSizeCompatibility2D2D() {

    // Register the workspace in the data service
    Workspace2D_sptr work_in1 =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);
    Workspace2D_sptr work_in2 =
        WorkspaceCreationHelper::create2DWorkspace(10, 20);
    Workspace2D_sptr work_in3 =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);
    Workspace2D_sptr work_in4 =
        WorkspaceCreationHelper::create2DWorkspace(5, 5);
    Workspace2D_sptr work_in5 =
        WorkspaceCreationHelper::create2DWorkspace(3, 3);
    Workspace2D_sptr work_in6 =
        WorkspaceCreationHelper::create2DWorkspace(100, 1);
    MatrixWorkspace_sptr work_inEvent1 =
        WorkspaceCreationHelper::createEventWorkspace(5, 5);
    MatrixWorkspace_sptr work_inEvent2 =
        WorkspaceCreationHelper::createEventWorkspace(10, 10);
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in6).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_inEvent1).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_inEvent2).empty());
  }

  void testMaskedSpectraPropagation() {
    const int nHist = 5, nBins = 10;
    std::set<int64_t> masking;
    masking.insert(0);
    masking.insert(2);
    masking.insert(4);

    MatrixWorkspace_sptr work_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins, 0, masking);
    MatrixWorkspace_sptr work_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    BinaryOpHelper helper;
    helper.initialize();
    helper.setProperty("LHSWorkspace", work_in1);
    helper.setProperty("RHSWorkspace", work_in2);
    const std::string outputSpace("test");
    helper.setPropertyValue("OutputWorkspace", outputSpace);
    helper.setRethrows(true);
    helper.execute();

    TS_ASSERT(helper.isExecuted());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);
    TS_ASSERT(output);

    for (int i = 0; i < nHist; ++i) {
      auto &spectrumInfo = output->spectrumInfo();

      TSM_ASSERT("Detector was not found", spectrumInfo.hasDetectors(i));
      if (spectrumInfo.hasDetectors(i)) {
        if (masking.count(i) == 0) {
          TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), false);
        } else {
          TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), true);
        }
      }
    }
  }

  BinaryOperation::BinaryOperationTable_sptr
  do_test_buildBinaryOperationTable(std::vector<std::vector<int>> lhs,
                                    std::vector<std::vector<int>> rhs,
                                    bool expect_throw = false) {
    EventWorkspace_sptr lhsWS =
        WorkspaceCreationHelper::createGroupedEventWorkspace(lhs, 50, 1.0);
    EventWorkspace_sptr rhsWS =
        WorkspaceCreationHelper::createGroupedEventWorkspace(rhs, 50, 1.0);
    BinaryOperation::BinaryOperationTable_sptr table;
    Mantid::Kernel::Timer timer1;
    if (expect_throw) {
      TS_ASSERT_THROWS(
          table = BinaryOperation::buildBinaryOperationTable(lhsWS, rhsWS),
          const std::runtime_error &);
    } else {
      TS_ASSERT_THROWS_NOTHING(
          table = BinaryOperation::buildBinaryOperationTable(lhsWS, rhsWS));
      // std::cout << timer1.elapsed() << " sec to run
      // buildBinaryOperationTable\n";
      TS_ASSERT(table);
      TS_ASSERT_EQUALS(table->size(), lhsWS->getNumberHistograms());
    }
    return table;
  }

  void test_buildBinaryOperationTable_simpleLHS_by_groupedRHS() {
    std::vector<std::vector<int>> lhs(6), rhs(2);
    for (int i = 0; i < 6; i++) {
      // one detector per pixel in lhs
      lhs[i].push_back(i);
      // 3 detectors in each on the rhs
      rhs[i / 3].push_back(i);
    }
    auto table = do_test_buildBinaryOperationTable(lhs, rhs);
    for (int i = 0; i < 6; i++) {
      TS_ASSERT_EQUALS((*table)[i], i / 3);
    }
  }

  void
  test_buildBinaryOperationTable_simpleLHS_by_groupedRHS_mismatched_throws() {
    // one detector per pixel in lhs, but they start at 3
    std::vector<std::vector<int>> lhs{{3}, {4}, {5}, {6}, {7}, {8}};
    // 3 detectors in each on the rhs
    std::vector<std::vector<int>> rhs{{0, 1, 2}, {3, 4, 5}};
    auto table = do_test_buildBinaryOperationTable(lhs, rhs, false);
    TS_ASSERT_EQUALS((*table)[0], 1);
    TS_ASSERT_EQUALS((*table)[1], 1);
    TS_ASSERT_EQUALS((*table)[2], 1);
    TS_ASSERT_EQUALS((*table)[3], -1);
    TS_ASSERT_EQUALS((*table)[4], -1);
    TS_ASSERT_EQUALS((*table)[5], -1);
  }

  void test_buildBinaryOperationTable_groupedLHS_by_groupedRHS() {
    // two detectors per pixel in lhs
    std::vector<std::vector<int>> lhs{{0, 1}, {2, 3},   {4, 5},   {6, 7},
                                      {8, 9}, {10, 11}, {12, 13}, {14, 15}};
    // 4 detectors in each on the rhs
    std::vector<std::vector<int>> rhs{
        {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}};
    auto table = do_test_buildBinaryOperationTable(lhs, rhs);
    for (int i = 0; i < 8; i++) {
      TS_ASSERT_EQUALS((*table)[i], i / 2);
    }
  }

  void
  test_buildBinaryOperationTable_groupedLHS_by_groupedRHS_bad_overlap_throws() {
    // 4 detectors per pixel in lhs
    std::vector<std::vector<int>> lhs{{0, 1, 2, 3},     {4, 5, 6, 7},
                                      {8, 9, 10, 11},   {12, 13, 14, 15},
                                      {16, 17, 18, 19}, {20, 21, 22, 23}};
    // 6 detectors in each on the rhs
    std::vector<std::vector<int>> rhs{{0, 1, 2, 3, 4, 5},
                                      {6, 7, 8, 9, 10, 11},
                                      {12, 13, 14, 15, 16, 17},
                                      {18, 19, 20, 21, 22, 23}};

    auto table = do_test_buildBinaryOperationTable(lhs, rhs, false);
    TS_ASSERT_EQUALS((*table)[0], 0);  // 0-3 go into 0-5
    TS_ASSERT_EQUALS((*table)[1], -1); // 4-7 fails to go anywhere
    TS_ASSERT_EQUALS((*table)[2], 1);  // 8-11 goes into 6-11
  }

  void test_buildBinaryOperationTable_simpleLHS_by_groupedRHS_large() {
    std::vector<std::vector<int>> lhs(2000, std::vector<int>(1)),
        rhs(20, std::vector<int>(100));
    for (int i = 0; i < 2000; i++) {
      // 1 detector per pixel in lhs
      lhs[i][0] = i;
      // 1000 detectors in each on the rhs
      rhs[i / 100][i % 100] = i;
    }
    auto table = do_test_buildBinaryOperationTable(lhs, rhs);
    for (int i = 0; i < 2000; i++) {
      TS_ASSERT_EQUALS((*table)[i], i / 100);
    }
  }

  void test_parallel_Distributed() {
    ParallelTestHelpers::runParallel(run_parallel,
                                     Parallel::StorageMode::Distributed);
  }

  void test_parallel_Cloned() {
    ParallelTestHelpers::runParallel(run_parallel,
                                     Parallel::StorageMode::Cloned);
  }

  void test_parallel_MasterOnly() {
    ParallelTestHelpers::runParallel(run_parallel,
                                     Parallel::StorageMode::MasterOnly);
  }

  void test_parallel_mistmatch_fail() {
    using ParallelTestHelpers::runParallel;
    auto cloned = Parallel::StorageMode::Cloned;
    auto distri = Parallel::StorageMode::Distributed;
    auto master = Parallel::StorageMode::MasterOnly;
    runParallel(run_parallel_mismatch_fail, cloned, distri);
    runParallel(run_parallel_mismatch_fail, cloned, master);
    runParallel(run_parallel_mismatch_fail, distri, cloned);
    runParallel(run_parallel_mismatch_fail, distri, master);
    runParallel(run_parallel_mismatch_fail, master, cloned);
    runParallel(run_parallel_mismatch_fail, master, distri);
  }

  void test_parallel_Cloned_ClonedSingle() {
    ParallelTestHelpers::runParallel(run_parallel_single_value,
                                     Parallel::StorageMode::Cloned,
                                     Parallel::StorageMode::Cloned);
  }

  void test_parallel_Distributed_ClonedSingle() {
    ParallelTestHelpers::runParallel(run_parallel_single_value,
                                     Parallel::StorageMode::Distributed,
                                     Parallel::StorageMode::Cloned);
  }

  void test_parallel_MasterOnly_ClonedSingle() {
    ParallelTestHelpers::runParallel(run_parallel_single_value,
                                     Parallel::StorageMode::MasterOnly,
                                     Parallel::StorageMode::Cloned);
  }

  void test_parallel_AllowDifferentNumberSpectra_fail() {
    using ParallelTestHelpers::runParallel;
    runParallel(run_parallel_AllowDifferentNumberSpectra_fail,
                Parallel::StorageMode::Cloned);
    runParallel(run_parallel_AllowDifferentNumberSpectra_fail,
                Parallel::StorageMode::Distributed);
    runParallel(run_parallel_AllowDifferentNumberSpectra_fail,
                Parallel::StorageMode::MasterOnly);
  }
};

#endif /*BINARYOPERATIONTEST_H_*/
