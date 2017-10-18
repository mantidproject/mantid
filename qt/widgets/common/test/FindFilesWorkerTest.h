#ifndef MANTIDQT_API_FINDFILESWORKERTEST_H_
#define MANTIDQT_API_FINDFILESWORKERTEST_H_

#include "MantidQtWidgets/Common/FindFilesWorker.h"

#include <cxxtest/TestSuite.h>

class FindFilesWorkerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindFilesWorkerTest *createSuite() { return new FindFilesWorkerTest(); }
  static void destroySuite(FindFilesWorkerTest *suite) { delete suite; }

  void test_find_single_file() {}
};

#endif /* MANTIDQT_API_FINDFILESWORKERTEST */
