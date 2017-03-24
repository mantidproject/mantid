#ifndef MANTID_MPI_STATUSTEST_H_
#define MANTID_MPI_STATUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMPI/Status.h"

using Mantid::MPI::Status;

class StatusTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StatusTest *createSuite() { return new StatusTest(); }
  static void destroySuite(StatusTest *suite) { delete suite; }

  void test_constructor() {
#ifdef MPI_EXPERIMENTAL
    Status status(boost::mpi::status{});
    TS_ASSERT_EQUALS(status.source(), 0);
    TS_ASSERT_EQUALS(status.tag(), 0);
    TS_ASSERT_EQUALS(status.error(), 0);
#else
    Status status(1, 2, 3);
    TS_ASSERT_EQUALS(status.source(), 1);
    TS_ASSERT_EQUALS(status.tag(), 2);
    TS_ASSERT_EQUALS(status.error(), 3);
#endif
  }
};

#endif /* MANTID_MPI_STATUSTEST_H_ */
