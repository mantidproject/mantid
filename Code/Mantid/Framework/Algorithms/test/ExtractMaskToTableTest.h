#ifndef MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_
#define MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ExtractMaskToTable.h"

using Mantid::Algorithms::ExtractMaskToTable;

class ExtractMaskToTableTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractMaskToTableTest *createSuite() { return new ExtractMaskToTableTest(); }
  static void destroySuite( ExtractMaskToTableTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_ALGORITHMS_EXTRACTMASKTOTABLETEST_H_ */