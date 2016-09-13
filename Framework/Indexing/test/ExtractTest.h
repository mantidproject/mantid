#ifndef MANTID_INDEXING_EXTRACTTEST_H_
#define MANTID_INDEXING_EXTRACTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Extract.h"

using Mantid::Indexing::Extract;

class ExtractTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractTest *createSuite() { return new ExtractTest(); }
  static void destroySuite( ExtractTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_INDEXING_EXTRACTTEST_H_ */