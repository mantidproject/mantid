#ifndef MANTID_PARALLEL_CHUNKERTEST_H_
#define MANTID_PARALLEL_CHUNKERTEST_H_

#include <cxxtest/TestSuite.h>

#include <algorithm>
#include <numeric>

#include "MantidParallel/IO/Chunker.h"

using namespace Mantid::Parallel::IO;

namespace Mantid {
namespace Parallel {
namespace IO {
bool operator==(const Chunker::LoadRange &a, const Chunker::LoadRange &b) {
  return a.bankIndex == b.bankIndex && a.eventOffset == b.eventOffset &&
         a.eventCount == b.eventCount;
}
} // namespace IO
} // namespace Parallel
} // namespace Mantid

class ChunkerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ChunkerTest *createSuite() { return new ChunkerTest(); }
  static void destroySuite(ChunkerTest *suite) { delete suite; }

  void test_chunkSize() {
    const size_t chunkSize = 17;
    const Chunker chunker(1, 0, {}, chunkSize);
    TS_ASSERT_EQUALS(chunker.chunkSize(), chunkSize);
  }

  void test_makeWorkerGroups_4_ranks() {
    const int ranks = 4;
    const int rank = 1;
    const std::vector<size_t> bankSizes{6, 1, 4, 2};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto &groups = chunker.makeWorkerGroups();
    TS_ASSERT_EQUALS(groups.size(), 2);
    TS_ASSERT_EQUALS(groups[0][0], 0);
    TS_ASSERT_EQUALS(groups[0][1], 1);
    TS_ASSERT_EQUALS(groups[1][0], 2);
    TS_ASSERT_EQUALS(groups[1][1], 3);
  }

  void test_makeWorkerGroups_4_ranks_different_group_sizes() {
    const int ranks = 4;
    const int rank = 1;
    const std::vector<size_t> bankSizes{9, 1, 1, 1};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto &groups = chunker.makeWorkerGroups();
    TS_ASSERT_EQUALS(groups.size(), 2);
    TS_ASSERT_EQUALS(groups[0][0], 0);
    TS_ASSERT_EQUALS(groups[0][1], 1);
    TS_ASSERT_EQUALS(groups[0][2], 2);
    TS_ASSERT_EQUALS(groups[1][0], 3);
  }

  void test_makeRankGroups_4_ranks_zero_size_bank() {
    const int ranks = 4;
    const int rank = 1;
    const std::vector<size_t> bankSizes{9, 0, 1, 1};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto &groups = chunker.makeWorkerGroups();
    TS_ASSERT_EQUALS(groups.size(), 2);
    TS_ASSERT_EQUALS(groups[0].size(), 3);
    TS_ASSERT_EQUALS(groups[1].size(), 1);
    TS_ASSERT_EQUALS(groups[0][0], 0);
    TS_ASSERT_EQUALS(groups[0][1], 1);
    // This should be the size-zero bank. It is currently added to the first
    // group, but in principle this could be changed.
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

  void test_makeLoadRanges_zero_size_bank() {
    const int ranks = 1;
    const int rank = 0;
    const std::vector<size_t> bankSizes{7, 0, 4, 1};
    const size_t chunkSize = 2;
    const Chunker chunker(ranks, rank, bankSizes, chunkSize);
    const auto ranges = chunker.makeLoadRanges();
    TS_ASSERT_EQUALS(ranges.size(), 4 + 0 + 2 + 1);
    size_t bank = 0;
    TS_ASSERT_EQUALS(ranges[0], (Chunker::LoadRange{bank, 0, 2}));
    TS_ASSERT_EQUALS(ranges[1], (Chunker::LoadRange{bank, 2, 2}));
    TS_ASSERT_EQUALS(ranges[2], (Chunker::LoadRange{bank, 4, 2}));
    TS_ASSERT_EQUALS(ranges[3], (Chunker::LoadRange{bank, 6, 1}));
    bank = 2;
    TS_ASSERT_EQUALS(ranges[4], (Chunker::LoadRange{bank, 0, 2}));
    TS_ASSERT_EQUALS(ranges[5], (Chunker::LoadRange{bank, 2, 2}));
    bank = 3;
    TS_ASSERT_EQUALS(ranges[6], (Chunker::LoadRange{bank, 0, 1}));
    // Note: No entry for bank = 1.
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

  void test_makeLoadRange_many_random_banks() {
    for (int workers = 1; workers < 100; ++workers) {
      for (int worker = 0; worker < workers; ++worker) {
        // The following bank sizes come from actual files which have cause
        // trouble so this also servers as a regression test.
        for (const auto &bankSizes :
             {std::vector<size_t>{2091281, 520340,  841355,  912704,  1435110,
                                  567885,  1850044, 1333453, 1507522, 1396560,
                                  1699092, 1484645, 515805,  474417,  633111,
                                  600780,  638784,  572031,  741562,  593741,
                                  546107,  552800,  556607},
              std::vector<size_t>{
                  5158050,  5566070,  5528000,  5461070,  5937410,  7415620,
                  5720310,  6387840,  6007800,  6331110,  4744170,  20912810,
                  14846450, 16990920, 13965600, 15075220, 13334530, 18500440,
                  5678850,  14351100, 9127040,  8413550,  5203400}}) {
          const size_t chunkSize = 1024 * 1024;
          TS_ASSERT_THROWS_NOTHING(
              Chunker chunker(workers, worker, bankSizes, chunkSize));
          Chunker chunker(workers, worker, bankSizes, chunkSize);
          TS_ASSERT_THROWS_NOTHING(chunker.makeLoadRanges());
        }
      }
    }
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

  void test_large_and_small_banks_with_many_ranks() {
    for (int workers = 1; workers < 100; ++workers) {
      const std::vector<size_t> sizes{1234, 5678, 17, 3, 555};
      const auto result = Chunker::makeBalancedPartitioning(workers, sizes);

      // Maximum work a single worker has to do
      size_t maxWork = 0;
      for (const auto &item : result) {
        size_t size = 0;
        for (const auto bank : item.second)
          size += sizes[bank];
        const size_t work = (size + item.first - 1) / item.first;
        maxWork = std::max(maxWork, work);
      }

      const size_t totalWork =
          std::accumulate(sizes.begin(), sizes.end(), static_cast<size_t>(0));
      const size_t wastedWork = maxWork * workers - totalWork;

      // Fuzzy test to ensure that imbalance is not too large. This are by no
      // means hard limits and may be subject to change. Current limit is: At
      // most 30% and 3 of the workers may be `wasted` (whichever is less).
      TS_ASSERT(static_cast<double>(wastedWork) /
                    static_cast<double>(totalWork) <
                std::min(0.3, 3.0 / workers));
    }
  }

  void test_several_small_banks() {
    const int workers = 2;
    for (size_t banks = 2; banks < 10; ++banks) {
      const std::vector<size_t> sizes(banks, 1);
      const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
      TS_ASSERT_EQUALS(result.size(), workers);
      TS_ASSERT_EQUALS(result[0].first, 1);
      TS_ASSERT_EQUALS(result[1].first, 1);
      TS_ASSERT_EQUALS(result[0].second.size(),
                       (banks + workers - 1) / workers);
      TS_ASSERT_EQUALS(result[1].second.size(), banks / workers);
    }
  }

  void test_makeBalancedPartitioning_zero_size_bank() {
    const size_t workers = 2;
    const std::vector<size_t> sizes{5, 0, 3};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    TS_ASSERT_EQUALS(result.size(), 1);
    TS_ASSERT_EQUALS(result[0].first, 2);
    TS_ASSERT_EQUALS(result[0].second, (std::vector<size_t>{0, 2, 1}));
  }

  void test_makeBalancedPartitioning_all_banks_empty() {
    const size_t workers = 2;
    const std::vector<size_t> sizes{0, 0, 0};
    const auto result = Chunker::makeBalancedPartitioning(workers, sizes);
    TS_ASSERT_EQUALS(result.size(), 1);
    TS_ASSERT_EQUALS(result[0].first, 2);
  }
};

#endif /* MANTID_PARALLEL_CHUNKERTEST_H_ */
