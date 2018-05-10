#ifndef MANTID_PARALLEL_COMMUNICATORTEST_H_
#define MANTID_PARALLEL_COMMUNICATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/Communicator.h"
#include "MantidTestHelpers/ParallelRunner.h"

using namespace Mantid::Parallel;
using namespace ParallelTestHelpers;

namespace {
void send_recv(const Communicator &comm) {
  if (comm.size() < 2)
    return;

  double data = 3.14;

  if (comm.rank() == 0)
    (comm.send(1, 123, data));
  if (comm.rank() == 1) {
    double result;
    TS_ASSERT_THROWS_NOTHING(comm.recv(0, 123, result));
    TS_ASSERT_EQUALS(result, data);
  }
}

void send_recv_status(const Communicator &comm) {
  if (comm.size() < 2)
    return;

  std::vector<double> data{1.1, 2.2};

  if (comm.rank() == 0)
    (comm.send(1, 123, data.data(), 2));
  (comm.send(1, 123, data.data(), 1));
  if (comm.rank() == 1) {
    std::vector<double> result1(2);
    const auto status1 = comm.recv(0, 123, result1.data(), 2);
    TS_ASSERT_EQUALS(*status1.count<double>(), 2);
    TS_ASSERT_EQUALS(result1, data);
    std::vector<double> result2(2);
    const auto status2 = comm.recv(0, 123, result2.data(), 2);
    TS_ASSERT_EQUALS(*status2.count<double>(), 1);
    TS_ASSERT_EQUALS(result2, (std::vector<double>{1.1, 0.0}));
  }
}

void isend_recv(const Communicator &comm) {
  int64_t data = 123456789 + comm.rank();
  int dest = (comm.rank() + 1) % comm.size();
  int src = (comm.rank() + comm.size() - 1) % comm.size();
  int tag = 123;
  int64_t result;
  int64_t expected = comm.rank() == 0 ? data + comm.size() - 1 : data - 1;

  auto request = comm.isend(dest, tag, data);
  TS_ASSERT_THROWS_NOTHING(comm.recv(src, tag, result));
  TS_ASSERT_EQUALS(result, expected);
  TS_ASSERT_THROWS_NOTHING(request.wait());
}

void send_irecv(const Communicator &comm) {
  int64_t data = 123456789 + comm.rank();
  int dest = (comm.rank() + 1) % comm.size();
  int src = (comm.rank() + comm.size() - 1) % comm.size();
  int tag = 123;
  int64_t result;
  int64_t expected = comm.rank() == 0 ? data + comm.size() - 1 : data - 1;

  auto request = comm.irecv(src, tag, result);
  TS_ASSERT_THROWS_NOTHING(comm.send(dest, tag, data));
  TS_ASSERT_THROWS_NOTHING(request.wait());
  TS_ASSERT_EQUALS(result, expected);
}

void isend_irecv(const Communicator &comm) {
  int64_t data = 123456789 + comm.rank();
  int dest = (comm.rank() + 1) % comm.size();
  int src = (comm.rank() + comm.size() - 1) % comm.size();
  int tag = 123;
  int64_t result;
  int64_t expected = comm.rank() == 0 ? data + comm.size() - 1 : data - 1;

  auto send_req = comm.irecv(src, tag, result);
  auto recv_req = comm.isend(dest, tag, data);
  TS_ASSERT_THROWS_NOTHING(send_req.wait());
  TS_ASSERT_THROWS_NOTHING(recv_req.wait());
  TS_ASSERT_EQUALS(result, expected);
}
} // namespace

class CommunicatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CommunicatorTest *createSuite() { return new CommunicatorTest(); }
  static void destroySuite(CommunicatorTest *suite) { delete suite; }

  void test_defaults() {
    Communicator comm;
#ifdef MPI_EXPERIMENTAL
    boost::mpi::communicator world;
    TS_ASSERT_EQUALS(comm.size(), world.size());
    TS_ASSERT_EQUALS(comm.rank(), world.rank());
#else
    TS_ASSERT_EQUALS(comm.size(), 1);
    TS_ASSERT_EQUALS(comm.rank(), 0);
#endif
  }

  void test_send_recv() { runParallel(send_recv); }

  void test_send_recv_status() { runParallel(send_recv_status); }

  void test_isend_recv() { runParallel(isend_recv); }

  void test_send_irecv() { runParallel(send_irecv); }

  void test_isend_irecv() { runParallel(isend_irecv); }
};

#endif /* MANTID_PARALLEL_COMMUNICATORTEST_H_ */
