#ifndef MANTID_PARALLEL_COLLECTIVESTEST_H_
#define MANTID_PARALLEL_COLLECTIVESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/Collectives.h"
#include "MantidTestHelpers/ParallelRunner.h"

using namespace Mantid;
using namespace Parallel;

namespace {
void run_gather(const Communicator &comm) {
  int root = std::min(comm.size() - 1, 2);
  int value = 123 * comm.rank();
  std::vector<int> result;
  TS_ASSERT_THROWS_NOTHING(Parallel::gather(comm, value, result, root));
  if (comm.rank() == root) {
    TS_ASSERT_EQUALS(result.size(), comm.size());
    for (int i = 0; i < comm.size(); ++i) {
      TS_ASSERT_EQUALS(result[i], 123 * i);
    }
  } else {
    TS_ASSERT(result.empty());
  }
}

void run_gather_short_version(const Communicator &comm) {
  int root = std::min(comm.size() - 1, 2);
  int value = 123 * comm.rank();
  if (comm.rank() == root) {
    std::vector<int> result;
    TS_ASSERT_THROWS_NOTHING(Parallel::gather(comm, value, result, root));
    TS_ASSERT_EQUALS(result.size(), comm.size());
    for (int i = 0; i < comm.size(); ++i) {
      TS_ASSERT_EQUALS(result[i], 123 * i);
    }
  } else {
    TS_ASSERT_THROWS_NOTHING(Parallel::gather(comm, value, root));
  }
}

void run_all_gather(const Communicator &comm) {
  int value = 123 * comm.rank();
  std::vector<int> result;
  TS_ASSERT_THROWS_NOTHING(Parallel::all_gather(comm, value, result));
  TS_ASSERT_EQUALS(result.size(), comm.size());
  for (int i = 0; i < comm.size(); ++i) {
    TS_ASSERT_EQUALS(result[i], 123 * i);
  }
}

void run_all_to_all(const Communicator &comm) {
  std::vector<int> data;
  for (int rank = 0; rank < comm.size(); ++rank)
    data.emplace_back(1000 * comm.rank() + rank);
  std::vector<int> result;
  TS_ASSERT_THROWS_NOTHING(Parallel::all_to_all(comm, data, result));
  TS_ASSERT_EQUALS(result.size(), comm.size());
  for (int i = 0; i < comm.size(); ++i) {
    TS_ASSERT_EQUALS(result[i], 1000 * i + comm.rank());
  }
}
} // namespace

class CollectivesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CollectivesTest *createSuite() { return new CollectivesTest(); }
  static void destroySuite(CollectivesTest *suite) { delete suite; }

  void test_gather() { ParallelTestHelpers::runParallel(run_gather); }

  void test_gather_short_version() {
    ParallelTestHelpers::runParallel(run_gather_short_version);
  }

  void test_all_gather() { ParallelTestHelpers::runParallel(run_all_gather); }

  void test_all_to_all() { ParallelTestHelpers::runParallel(run_all_to_all); }
};

#endif /* MANTID_PARALLEL_COLLECTIVESTEST_H_ */
