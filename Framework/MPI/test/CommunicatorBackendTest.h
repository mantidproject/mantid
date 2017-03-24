#ifndef MANTID_MPI_COMMUNICATORBACKENDTEST_H_
#define MANTID_MPI_COMMUNICATORBACKENDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMPI/CommunicatorBackend.h"

using Mantid::MPI::CommunicatorBackend;

class CommunicatorBackendTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CommunicatorBackendTest *createSuite() {
    return new CommunicatorBackendTest();
  }
  static void destroySuite(CommunicatorBackendTest *suite) { delete suite; }

  void test_default_constructor() {
    CommunicatorBackend comm;
    TS_ASSERT_EQUALS(comm.size(), 1);
  }

  void test_size_constructor() {
    CommunicatorBackend comm{2};
    TS_ASSERT_EQUALS(comm.size(), 2);
  }
};

#endif /* MANTID_MPI_COMMUNICATORBACKENDTEST_H_ */
