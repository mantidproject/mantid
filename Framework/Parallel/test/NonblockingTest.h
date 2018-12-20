#ifndef MANTID_PARALLEL_NONBLOCKINGTEST_H_
#define MANTID_PARALLEL_NONBLOCKINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/Communicator.h"
#include "MantidParallel/Nonblocking.h"
#include "MantidTestHelpers/ParallelRunner.h"

using namespace Mantid;
using namespace Parallel;

namespace {
void run_wait_all(const Communicator &comm) {
  int64_t data = 123456789 + comm.rank();
  int dest = (comm.rank() + 1) % comm.size();
  int src = (comm.rank() + comm.size() - 1) % comm.size();
  int tag1 = 123;
  int tag2 = 124;
  int64_t result;

  std::vector<Request> requests;
  requests.emplace_back(comm.irecv(src, tag1, result));
  requests.emplace_back(comm.irecv(src, tag2, result));
  comm.send(dest, tag1, data);
  comm.send(dest, tag2, data);
  TS_ASSERT_THROWS_NOTHING(wait_all(requests.begin(), requests.end()));
}
} // namespace

class NonblockingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NonblockingTest *createSuite() { return new NonblockingTest(); }
  static void destroySuite(NonblockingTest *suite) { delete suite; }

  void test_wait_all() { ParallelTestHelpers::runParallel(run_wait_all); }
};

#endif /* MANTID_PARALLEL_NONBLOCKINGTEST_H_ */
