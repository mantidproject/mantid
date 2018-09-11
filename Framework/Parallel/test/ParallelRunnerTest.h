#ifndef MANTID_PARALLEL_PARALLELRUNNERTEST_H_
#define MANTID_PARALLEL_PARALLELRUNNERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/ParallelRunner.h"

#include <algorithm>
#include <mutex>
#include <vector>

using namespace Mantid::Parallel;
using ParallelTestHelpers::ParallelRunner;

namespace {
void get_sizes(const Communicator &comm, std::mutex &mutex,
               std::vector<int> &sizes) {
  std::lock_guard<std::mutex> lock(mutex);
  sizes.push_back(comm.size());
}

void get_ranks(const Communicator &comm, std::mutex &mutex,
               std::set<int> &ranks) {
  std::lock_guard<std::mutex> lock(mutex);
  ranks.insert(comm.rank());
}
} // namespace

class ParallelRunnerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ParallelRunnerTest *createSuite() { return new ParallelRunnerTest(); }
  static void destroySuite(ParallelRunnerTest *suite) { delete suite; }

  void test_size() {
    std::mutex mutex;
    ParallelRunner parallel;
    TS_ASSERT(parallel.size() > 1);
    std::vector<int> sizes;
    parallel.run(get_sizes, std::ref(mutex), std::ref(sizes));
    // Currently ParallelRunner also runs the callable with a single rank.
    TS_ASSERT_EQUALS(std::count(sizes.begin(), sizes.end(), 1), 1);
    TS_ASSERT_EQUALS(std::count(sizes.begin(), sizes.end(), parallel.size()),
                     parallel.size());
  }

  void test_rank() {
    std::mutex mutex;
    std::set<int> ranks;
    ParallelRunner parallel;
    parallel.run(get_ranks, std::ref(mutex), std::ref(ranks));
    int size{1};
#ifdef MPI_EXPERIMENTAL
    boost::mpi::communicator world;
    size = world.size();
#endif
    if (size == 1) {
      for (int rank = 0; rank < parallel.size(); ++rank)
        TS_ASSERT_EQUALS(ranks.count(rank), 1);
    } else {
#ifdef MPI_EXPERIMENTAL
      TS_ASSERT_EQUALS(ranks.count(world.rank()), 1);
#endif
    }
  }
};

#endif /* MANTID_PARALLEL_PARALLELRUNNERTEST_H_ */
