#ifndef MANTID_MPI_REQUESTTEST_H_
#define MANTID_MPI_REQUESTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMPI/Request.h"

using Mantid::MPI::Request;

class RequestTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RequestTest *createSuite() { return new RequestTest(); }
  static void destroySuite(RequestTest *suite) { delete suite; }

  void test_default() {
    TS_ASSERT_THROWS_NOTHING(Request());
    Request req;
    TS_ASSERT_THROWS_NOTHING(req.wait());
  }
};

#endif /* MANTID_MPI_REQUESTTEST_H_ */
