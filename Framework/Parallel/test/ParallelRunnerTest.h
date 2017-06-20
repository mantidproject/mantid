#ifndef MANTID_PARALLEL_PARALLELRUNNERTEST_H_
#define MANTID_PARALLEL_PARALLELRUNNERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/ParallelRunner.h"

#include <mutex>
#include <vector>

using namespace Mantid::Parallel;
using ParallelTestHelpers::ParallelRunner;

namespace {
void check_size(const Communicator &comm, const int expected) {
  TS_ASSERT_EQUALS(comm.size(), expected);
}

void get_ranks(const Communicator &comm, std::mutex &mutex,
               std::set<int> &ranks) {
  std::lock_guard<std::mutex> lock(mutex);
  ranks.insert(comm.rank());
}
}

class ParallelRunnerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ParallelRunnerTest *createSuite() { return new ParallelRunnerTest(); }
  static void destroySuite(ParallelRunnerTest *suite) { delete suite; }

  void test_size() {
    ParallelRunner parallel;
    TS_ASSERT(parallel.size() > 1);
    parallel.run(check_size, parallel.size());
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
