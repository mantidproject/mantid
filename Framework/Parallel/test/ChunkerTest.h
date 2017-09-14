#ifndef MANTID_PARALLEL_CHUNKERTEST_H_
#define MANTID_PARALLEL_CHUNKERTEST_H_

#include <cxxtest/TestSuite.h>

#include <algorithm>

#include <H5Cpp.h>

#include "MantidParallel/IO/Chunker.h"

using namespace Mantid::Parallel::IO;

namespace {
bool operator==(const Chunker::LoadRange &a, const Chunker::LoadRange &b) {
  return a.bankIndex == b.bankIndex && a.eventOffset == b.eventOffset &&
         a.eventCount == b.eventCount;
}
}

class ChunkerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChunkerTest *createSuite() { return new ChunkerTest(); }
  static void destroySuite(ChunkerTest *suite) { delete suite; }

  void test_makeRankGroups_4_ranks() {
    const int ranks = 4;
    const int rank = 1;
    const std::vector<size_t> bankSizes{6, 1, 4, 2};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto &groups = chunker.makeRankGroups();
    TS_ASSERT_EQUALS(groups.size(), 2);
    TS_ASSERT_EQUALS(groups[0][0], 0);
    TS_ASSERT_EQUALS(groups[0][1], 1);
    TS_ASSERT_EQUALS(groups[1][0], 2);
    TS_ASSERT_EQUALS(groups[1][1], 3);
  }

  void test_makeRankGroups_4_ranks_different_group_sizes() {
    const int ranks = 4;
    const int rank = 1;
    const std::vector<size_t> bankSizes{9, 1, 1, 1};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto &groups = chunker.makeRankGroups();
    TS_ASSERT_EQUALS(groups.size(), 2);
    TS_ASSERT_EQUALS(groups[0][0], 0);
    TS_ASSERT_EQUALS(groups[0][1], 1);
    TS_ASSERT_EQUALS(groups[0][2], 2);
    TS_ASSERT_EQUALS(groups[1][0], 3);
  }

  void test_makeLoadRanges_1_rank() {
    const int ranks = 1;
    const int rank = 0;
    const std::vector<size_t> bankSizes{7, 2, 4, 1};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto ranges = chunker.makeLoadRanges();
    TS_ASSERT_EQUALS(ranges.size(), 1 + 1 + 2 + 4);
    size_t bank = 0;
    TS_ASSERT_EQUALS(ranges[0], (Chunker::LoadRange{bank, 0, 2}));
    TS_ASSERT_EQUALS(ranges[1], (Chunker::LoadRange{bank, 2, 2}));
    TS_ASSERT_EQUALS(ranges[2], (Chunker::LoadRange{bank, 4, 2}));
    TS_ASSERT_EQUALS(ranges[3], (Chunker::LoadRange{bank, 6, 1}));
    bank = 2;
    TS_ASSERT_EQUALS(ranges[4], (Chunker::LoadRange{bank, 0, 2}));
    TS_ASSERT_EQUALS(ranges[5], (Chunker::LoadRange{bank, 2, 2}));
    bank = 1;
    TS_ASSERT_EQUALS(ranges[6], (Chunker::LoadRange{bank, 0, 2}));
    bank = 3;
    TS_ASSERT_EQUALS(ranges[7], (Chunker::LoadRange{bank, 0, 1}));
  }

  void test_makeLoadRanges_2_ranks_rank0() {
    const int ranks = 2;
    const int rank = 0;
    const std::vector<size_t> bankSizes{6, 1, 4, 2};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto ranges = chunker.makeLoadRanges();
    TS_ASSERT_EQUALS(ranges.size(), 4);
    size_t bank = 0;
    TS_ASSERT_EQUALS(ranges[0], (Chunker::LoadRange{bank, 0, 2}));
    TS_ASSERT_EQUALS(ranges[1], (Chunker::LoadRange{bank, 2, 2}));
    TS_ASSERT_EQUALS(ranges[2], (Chunker::LoadRange{bank, 4, 2}));
    bank = 1;
    // Note that bank is not 3, which would be the next largest fitting into the
    // partition, but internally math is done based on chunks so 2 == 1.
    TS_ASSERT_EQUALS(ranges[3], (Chunker::LoadRange{bank, 0, 1}));
  }

  void test_makeLoadRanges_2_ranks_rank1() {
    const int ranks = 2;
    const int rank = 1;
    const std::vector<size_t> bankSizes{6, 1, 4, 2};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto ranges = chunker.makeLoadRanges();
    TS_ASSERT_EQUALS(ranges.size(), 4);
    size_t bank = 2;
    TS_ASSERT_EQUALS(ranges[0], (Chunker::LoadRange{bank, 0, 2}));
    TS_ASSERT_EQUALS(ranges[1], (Chunker::LoadRange{bank, 2, 2}));
    bank = 3;
    TS_ASSERT_EQUALS(ranges[2], (Chunker::LoadRange{bank, 0, 2}));
    // Last range is padding (size 0)
    bank = 0;
    TS_ASSERT_EQUALS(ranges[3], (Chunker::LoadRange{bank, 0, 0}));
  }

  void test_makeLoadRanges_4_ranks_rank1() {
    const int ranks = 4;
    const int rank = 1;
    const std::vector<size_t> bankSizes{6, 1, 4, 2};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto ranges = chunker.makeLoadRanges();
    TS_ASSERT_EQUALS(ranges.size(), 2);
    size_t bank = 0;
    TS_ASSERT_EQUALS(ranges[0], (Chunker::LoadRange{bank, 2, 2}));
    bank = 1;
    TS_ASSERT_EQUALS(ranges[1], (Chunker::LoadRange{bank, 0, 1}));
  }

  void test_makeBalancedPartitioning_1_worker() {
    const size_t workers = 1;
    const std::vector<size_t> sizes{7, 1, 3};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    TS_ASSERT_EQUALS(result.size(), 1);
    TS_ASSERT_EQUALS(result[0].first, 1);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0, 2, 1}));
  }

  void test_makeBalancedPartitioning_2_workers_striping() {
    const size_t workers = 2;
    const std::vector<size_t> sizes{7, 1, 3};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    // Largest size is more than 50% of total, so striping is used
    TS_ASSERT_EQUALS(result.size(), 1);
    TS_ASSERT_EQUALS(result[0].first, 2);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0, 2, 1}));
  }

  void test_makeBalancedPartitioning_2_workers_no_striping() {
    const size_t workers = 2;
    const std::vector<size_t> sizes{7, 1, 6};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    // Largest size is equal to 50% of total, i.e., no striping necessary
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result[0].first, 1);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0}));
    TS_ASSERT_EQUALS(result[1].first, 1);
    TS_ASSERT_EQUALS(result[1].second, (std::vector<size_t>{2, 1}));
  }

  void test_makeBalancedPartitioning_2_workers_tied_sizes() {
    const size_t workers = 2;
    const std::vector<size_t> sizes{7, 1, 7};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result[0].first, 1);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0, 1}));
    TS_ASSERT_EQUALS(result[1].first, 1);
    TS_ASSERT_EQUALS(result[1].second, (std::vector<size_t>{2}));
  }

  void test_makeBalancedPartitioning_3_workers_striping() {
    const size_t workers = 3;
    const std::vector<size_t> sizes{9, 1, 3};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    // Largest size is more than 2/3 of total, so striping is used
    TS_ASSERT_EQUALS(result.size(), 1);
    TS_ASSERT_EQUALS(result[0].first, 3);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0, 2, 1}));
  }

  void test_makeBalancedPartitioning_3_workers_partial_striping() {
    const size_t workers = 3;
    const std::vector<size_t> sizes{8, 1, 3};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    // Largest size is 2/3 of total, so striping for largest, no striping for
    // others.
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result[0].first, 2);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0}));
    TS_ASSERT_EQUALS(result[1].first, 1);
    TS_ASSERT_EQUALS(result[1].second, (std::vector<size_t>{2, 1}));
  }

  void test_makeBalancedPartitioning_4_workers_striping() {
    const size_t workers = 4;
    const std::vector<size_t> sizes{13, 1, 3};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    // Largest size is more than 3/4 of total, so striping is used
    TS_ASSERT_EQUALS(result.size(), 1);
    TS_ASSERT_EQUALS(result[0].first, 4);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0, 2, 1}));
  }

  void test_makeBalancedPartitioning_4_workers_partial_striping() {
    const size_t workers = 4;
    const std::vector<size_t> sizes{12, 1, 3};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    // Largest size is 3/4 of total, so striping for largest, no striping for
    // others.
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result[0].first, 3);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0}));
    TS_ASSERT_EQUALS(result[1].first, 1);
    TS_ASSERT_EQUALS(result[1].second, (std::vector<size_t>{2, 1}));
  }

  void test_makeBalancedPartitioning_4_workers_partial_independent_striping() {
    const size_t workers = 4;
    const std::vector<size_t> sizes{4, 1, 3};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    // Largest size is 2/4 of total, so striping for largest with half of
    // workers and striping for others with the other half.
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result[0].first, 2);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0}));
    TS_ASSERT_EQUALS(result[1].first, 2);
    TS_ASSERT_EQUALS(result[1].second, (std::vector<size_t>{2, 1}));
  }
};

#endif /* MANTID_PARALLEL_CHUNKERTEST_H_ */
