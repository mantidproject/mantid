// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_INDEXINFOTEST_H_
#define MANTID_INDEXING_INDEXINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/make_cow.h"
#include "MantidParallel/Communicator.h"
#include "MantidTypes/SpectrumDefinition.h"

#include "MantidTestHelpers/ParallelRunner.h"
#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/environment.hpp>
#endif

using namespace Mantid;
using namespace Indexing;
using namespace ParallelTestHelpers;

namespace {
void run_StorageMode_Cloned(const Parallel::Communicator &comm) {
  IndexInfo i(3, Parallel::StorageMode::Cloned, comm);
  TS_ASSERT_EQUALS(i.size(), 3);
  TS_ASSERT_EQUALS(i.globalSize(), 3);
  TS_ASSERT_EQUALS(i.spectrumNumber(0), 1);
  TS_ASSERT_EQUALS(i.spectrumNumber(1), 2);
  TS_ASSERT_EQUALS(i.spectrumNumber(2), 3);
}

void run_StorageMode_Distributed(const Parallel::Communicator &comm) {
  IndexInfo i(47, Parallel::StorageMode::Distributed, comm);
  TS_ASSERT_EQUALS(i.globalSize(), 47);
  TS_ASSERT_EQUALS(i.spectrumNumbers().size(), 47);
  size_t expectedSize = 0;
  for (size_t globalIndex = 0; globalIndex < i.globalSize(); ++globalIndex) {
    SpectrumNumber specNum = static_cast<int>(globalIndex) + 1;
    TS_ASSERT_EQUALS(i.spectrumNumbers()[globalIndex], specNum);
    // Current default is RoundRobinPartitioner
    if (static_cast<int>(globalIndex) % comm.size() == comm.rank()) {
      TS_ASSERT_EQUALS(i.spectrumNumber(expectedSize), specNum);
      ++expectedSize;
    }
  }
  TS_ASSERT_EQUALS(i.size(), expectedSize);
}

void run_StorageMode_MasterOnly(const Parallel::Communicator &comm) {
  if (comm.rank() == 0) {
    IndexInfo i(3, Parallel::StorageMode::MasterOnly, comm);
    TS_ASSERT_EQUALS(i.size(), 3);
    TS_ASSERT_EQUALS(i.globalSize(), 3);
    TS_ASSERT_EQUALS(i.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(i.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(i.spectrumNumber(2), 3);
  } else {
    TS_ASSERT_THROWS(IndexInfo(3, Parallel::StorageMode::MasterOnly, comm),
                     const std::runtime_error &);
  }
}

void run_isOnThisPartition_StorageMode_Cloned(
    const Parallel::Communicator &comm) {
  IndexInfo info(47, Parallel::StorageMode::Cloned, comm);
  for (size_t i = 0; i < info.globalSize(); ++i)
    TS_ASSERT(info.isOnThisPartition(i));
}

void run_isOnThisPartition_StorageMode_Distributed(
    const Parallel::Communicator &comm) {
  IndexInfo info(47, Parallel::StorageMode::Distributed, comm);
  // Current default is RoundRobinPartitioner
  for (size_t i = 0; i < info.globalSize(); ++i) {
    if (static_cast<int>(i) % comm.size() == comm.rank()) {
      TS_ASSERT(info.isOnThisPartition(i));
    }
  }
}

void run_construct_from_parent_StorageMode_Distributed(
    const Parallel::Communicator &comm) {
  IndexInfo parent(47, Parallel::StorageMode::Distributed, comm);
  IndexInfo i(std::vector<GlobalSpectrumIndex>{10, 11, 12, 13, 14, 15, 16},
              parent);
  size_t expectedSize = 0;
  // Rank in `i` is given by rank in parent, so we iterate parent!
  for (size_t globalIndex = 0; globalIndex < parent.globalSize();
       ++globalIndex) {
    if (static_cast<int>(globalIndex) % comm.size() == comm.rank()) {
      if (globalIndex >= 10 && globalIndex <= 16) {
        TS_ASSERT_EQUALS(i.spectrumNumber(expectedSize),
                         static_cast<int>(globalIndex) + 1);
        ++expectedSize;
      }
    }
  }
  TS_ASSERT_EQUALS(i.size(), expectedSize);
}

IndexInfo makeIndexInfo(const Parallel::Communicator &comm) {
  IndexInfo i(666, Parallel::StorageMode::Distributed, comm);
  std::vector<SpectrumDefinition> specDefs(i.size());
  size_t current = 0;
  for (size_t spec = 0; spec < 200; ++spec) {
    if (i.isOnThisPartition(GlobalSpectrumIndex(spec))) {
      if (spec > 42)
        specDefs[current].add(spec + 1);
      ++current;
    }
  }
  i.setSpectrumDefinitions(specDefs);
  return i;
}

void run_globalSpectrumIndicesFromDetectorIndices_Distributed(
    const Parallel::Communicator &comm) {
  const auto &i = makeIndexInfo(comm);
  // Out of order
  std::vector<size_t> detectorIndices{100, 101, 102, 200, 199};
  const auto &indices =
      i.globalSpectrumIndicesFromDetectorIndices(detectorIndices);
  TS_ASSERT_EQUALS(indices.size(), 5);
  TS_ASSERT_EQUALS(indices[0], 99);
  TS_ASSERT_EQUALS(indices[1], 100);
  TS_ASSERT_EQUALS(indices[2], 101);
  TS_ASSERT_EQUALS(indices[3], 198);
  TS_ASSERT_EQUALS(indices[4], 199);
}

void run_globalSpectrumIndicesFromDetectorIndices_missing(
    const Parallel::Communicator &comm) {
  const auto &i = makeIndexInfo(comm);
  // No spectrum maps to 17
  std::vector<size_t> detectorIndices{100, 101, 102, 200, 199, 17};
  TS_ASSERT_THROWS_EQUALS(
      i.globalSpectrumIndicesFromDetectorIndices(detectorIndices),
      const std::runtime_error &e, std::string(e.what()),
      "Some of the requested detectors do not have a corresponding spectrum");
}
} // namespace

class IndexInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexInfoTest *createSuite() { return new IndexInfoTest(); }
  static void destroySuite(IndexInfoTest *suite) { delete suite; }

  void test_size_constructor() { TS_ASSERT_THROWS_NOTHING(IndexInfo(3)); }

  void test_size_constructor_sets_correct_indices() {
    IndexInfo info(3);
    TS_ASSERT_EQUALS(info.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(info.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(info.spectrumNumber(2), 3);
    TS_ASSERT(!info.spectrumDefinitions());
  }

  void test_vector_constructor() {
    TS_ASSERT_THROWS_NOTHING(IndexInfo({3, 2, 1}));
  }

  void test_vector_constructor_sets_correct_indices() {
    IndexInfo info({3, 2, 1});
    TS_ASSERT_EQUALS(info.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(info.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(info.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.spectrumNumbers()[0], 3);
    TS_ASSERT_EQUALS(info.spectrumNumbers()[1], 2);
    TS_ASSERT_EQUALS(info.spectrumNumbers()[2], 1);
  }

  void test_construct_from_parent_reorder() {
    IndexInfo parent({3, 2, 1});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[0].add(7);
    specDefs[2].add(8);
    parent.setSpectrumDefinitions(specDefs);

    IndexInfo i(std::vector<SpectrumNumber>{2, 1, 3}, parent);

    TS_ASSERT_EQUALS(i.size(), 3);
    TS_ASSERT_EQUALS(i.globalSize(), 3);
    TS_ASSERT_EQUALS(i.spectrumNumber(0), 2);
    TS_ASSERT_EQUALS(i.spectrumNumber(1), 1);
    TS_ASSERT_EQUALS(i.spectrumNumber(2), 3);
    TS_ASSERT(i.spectrumDefinitions());
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[0], specDefs[1]);
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[1], specDefs[2]);
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[2], specDefs[0]);
  }

  void test_construct_from_parent_filter() {
    IndexInfo parent({3, 2, 1});
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[0].add(7);
    specDefs[2].add(8);
    parent.setSpectrumDefinitions(specDefs);

    IndexInfo i(std::vector<SpectrumNumber>{1, 2}, parent);

    TS_ASSERT_EQUALS(i.size(), 2);
    TS_ASSERT_EQUALS(i.globalSize(), 2);
    TS_ASSERT_EQUALS(i.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(i.spectrumNumber(1), 2);
    TS_ASSERT(i.spectrumDefinitions());
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[0], specDefs[2]);
    TS_ASSERT_EQUALS((*i.spectrumDefinitions())[1], specDefs[1]);
  }

  void test_size() { TS_ASSERT_EQUALS(IndexInfo(3).size(), 3); }

  void test_copy() {
    IndexInfo info({3, 2, 1});
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    info.setSpectrumDefinitions(defs);
    auto copy(info);
    TS_ASSERT_EQUALS(info.size(), 3);
    TS_ASSERT_EQUALS(copy.size(), 3);
    TS_ASSERT_EQUALS(copy.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(copy.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(copy.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.spectrumDefinitions(), copy.spectrumDefinitions());
  }

  void test_move() {
    IndexInfo info({3, 2, 1});
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    info.setSpectrumDefinitions(defs);
    auto moved(std::move(info));
    TS_ASSERT_EQUALS(info.size(), 0);
    TS_ASSERT_EQUALS(moved.size(), 3);
    TS_ASSERT_EQUALS(moved.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(moved.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(moved.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.spectrumDefinitions(), nullptr);
  }

  void test_setSpectrumNumbers_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS(t.setSpectrumNumbers({1, 2}), const std::runtime_error &);
  }

  void test_setSpectrumDefinitions_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_EQUALS(
        t.setSpectrumDefinitions(std::vector<SpectrumDefinition>(2)),
        const std::runtime_error &e, std::string(e.what()),
        "IndexInfo: Size mismatch when setting new spectrum definitions");
  }

  void test_setSpectrumNumbers() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_NOTHING(t.setSpectrumNumbers({3, 4, 5}));
    TS_ASSERT_EQUALS(t.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(t.spectrumNumber(1), 4);
    TS_ASSERT_EQUALS(t.spectrumNumber(2), 5);
  }

  void test_setSpectrumDefinitions() {
    IndexInfo t(3);
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[1].add(7);
    specDefs[2].add(8);
    TS_ASSERT_THROWS_NOTHING(t.setSpectrumDefinitions(specDefs));
    TS_ASSERT(t.spectrumDefinitions());
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[0], specDefs[0]);
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[1], specDefs[1]);
    TS_ASSERT_EQUALS((*t.spectrumDefinitions())[2], specDefs[2]);
  }

  void test_setSpectrumDefinitions_setting_nullptr_fails() {
    // This might be supported in the future but is not needed now and might
    // break some things, so we forbid this for now.
    IndexInfo info(3);
    Kernel::cow_ptr<std::vector<SpectrumDefinition>> defs{nullptr};
    TS_ASSERT_THROWS(info.setSpectrumDefinitions(defs), const std::runtime_error &);
  }

  void test_setSpectrumDefinitions_size_mismatch_cow_ptr() {
    IndexInfo info(3);
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(2);
    TS_ASSERT_THROWS(info.setSpectrumDefinitions(defs), const std::runtime_error &);
  }

  void test_setSpectrumDefinitions_cow_ptr() {
    IndexInfo info(3);
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    TS_ASSERT_THROWS_NOTHING(info.setSpectrumDefinitions(defs));
    TS_ASSERT_EQUALS(info.spectrumDefinitions().get(), defs.get());
  }

  void test_globalSpectrumIndicesFromDetectorIndices_fails_without_spec_defs() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    TS_ASSERT_THROWS_EQUALS(
        info.globalSpectrumIndicesFromDetectorIndices(detectorIndices),
        const std::runtime_error &e, std::string(e.what()),
        "IndexInfo::globalSpectrumIndicesFromDetectorIndices -- no spectrum "
        "definitions available");
  }

  void test_globalSpectrumIndicesFromDetectorIndices_fails_multiple() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[1].add(7);
    specDefs[1].add(77);
    specDefs[2].add(8);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_EQUALS(
        info.globalSpectrumIndicesFromDetectorIndices(detectorIndices),
        const std::runtime_error &e, std::string(e.what()),
        "SpectrumDefinition contains multiple entries. No unique mapping from "
        "detector to spectrum possible");
  }

  void test_globalSpectrumIndicesFromDetectorIndices_fails_missing() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    // Nothing maps to 8
    specDefs[0].add(6);
    specDefs[1].add(7);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_EQUALS(
        info.globalSpectrumIndicesFromDetectorIndices(detectorIndices),
        const std::runtime_error &e, std::string(e.what()),
        "Some of the requested detectors do not have a corresponding spectrum");
  }

  void
  test_globalSpectrumIndicesFromDetectorIndices_scanning_different_time_indices() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    // Two indices map to the same detector at different time indices; typical
    // for time-indexed workspaces.
    specDefs[0].add(6, 1);
    specDefs[1].add(6, 2);
    specDefs[2].add(8, 1);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_NOTHING(
        info.globalSpectrumIndicesFromDetectorIndices(detectorIndices));
    const auto &indices =
        info.globalSpectrumIndicesFromDetectorIndices(detectorIndices);
    TS_ASSERT_EQUALS(indices.size(), 3);
    TS_ASSERT_EQUALS(indices[0], 0);
    TS_ASSERT_EQUALS(indices[1], 1);
    TS_ASSERT_EQUALS(indices[2], 2);
  }

  void
  test_globalSpectrumIndicesFromDetectorIndices_scanning_same_time_indices() {
    IndexInfo info(2);
    std::vector<size_t> detectorIndices{6};
    std::vector<SpectrumDefinition> specDefs(2);
    // Two indices map to the same detector at the same time index; throw
    specDefs[0].add(6, 1);
    specDefs[1].add(6, 1);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_EQUALS(
        info.globalSpectrumIndicesFromDetectorIndices(detectorIndices),
        const std::runtime_error &e, std::string(e.what()),
        "Some of the spectra map to the same detector at the same time index");
  }

  void test_globalSpectrumIndicesFromDetectorIndices_fails_conflict_miss() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    // Two indices map to same detector, but additionally one is missing.
    specDefs[0].add(6, 1);
    specDefs[1].add(6, 2);
    info.setSpectrumDefinitions(specDefs);
    TS_ASSERT_THROWS_EQUALS(
        info.globalSpectrumIndicesFromDetectorIndices(detectorIndices),
        const std::runtime_error &e, std::string(e.what()),
        "Some of the requested detectors do not have a "
        "corresponding spectrum");
  }

  void test_globalSpectrumIndicesFromDetectorIndices() {
    IndexInfo info(3);
    std::vector<size_t> detectorIndices{6, 8};
    std::vector<SpectrumDefinition> specDefs(3);
    specDefs[0].add(6);
    specDefs[1].add(7);
    specDefs[2].add(8);
    info.setSpectrumDefinitions(specDefs);
    const auto &indices =
        info.globalSpectrumIndicesFromDetectorIndices(detectorIndices);
    TS_ASSERT_EQUALS(indices.size(), detectorIndices.size());
    TS_ASSERT_EQUALS(indices[0], 0);
    TS_ASSERT_EQUALS(indices[1], 2);
  }

  void test_StorageMode_Cloned() {
    runParallel(run_StorageMode_Cloned);
    // Trivial: Run with one partition.
    run_StorageMode_Cloned(Parallel::Communicator{});
  }

  void test_StorageMode_Distributed() {
    runParallel(run_StorageMode_Distributed);
    // Trivial: Run with one partition.
    run_StorageMode_Distributed(Parallel::Communicator{});
  }

  void test_StorageMode_MasterOnly() {
    runParallel(run_StorageMode_MasterOnly);
    // Trivial: Run with one partition.
    run_StorageMode_MasterOnly(Parallel::Communicator{});
  }

  void test_isOnThisPartition_StorageMode_Cloned() {
    runParallel(run_isOnThisPartition_StorageMode_Cloned);
    // Trivial: Run with one partition.
    run_isOnThisPartition_StorageMode_Cloned(Parallel::Communicator{});
  }

  void test_isOnThisPartition_StorageMode_Distributed() {
    runParallel(run_isOnThisPartition_StorageMode_Distributed);
    // Trivial: Run with one partition.
    run_isOnThisPartition_StorageMode_Distributed(Parallel::Communicator{});
  }

  void test_construct_from_parent_StorageMode_Distributed() {
    runParallel(run_construct_from_parent_StorageMode_Distributed);
  }

  void test_globalSpectrumIndicesFromDetectorIndices_Distributed() {
    runParallel(run_globalSpectrumIndicesFromDetectorIndices_Distributed);
  }

  void test_globalSpectrumIndicesFromDetectorIndices_missing() {
    runParallel(run_globalSpectrumIndicesFromDetectorIndices_missing);
  }

private:
#ifdef MPI_EXPERIMENTAL
  boost::mpi::environment m_environment;
#endif
};

#endif /* MANTID_INDEXING_INDEXINFOTEST_H_ */
