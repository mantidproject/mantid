#ifndef MANTID_PARALLEL_COLLECTIVESTEST_H_
#define MANTID_PARALLEL_COLLECTIVESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/Collectives.h"
#include "MantidTestHelpers/ParallelRunner.h"

using namespace Mantid;
using namespace Parallel;

namespace {
void run_gather(const Communicator &comm) {
  int root = 2;
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
  int root = 2;
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
}

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
};

#endif /* MANTID_PARALLEL_COLLECTIVESTEST_H_ */
