#ifndef MANTID_API_MultiPeriodGroupAlgorithmTEST_H_
#define MANTID_API_MultiPeriodGroupAlgorithmTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MultiPeriodGroupAlgorithm.h"

using Mantid::API::MultiPeriodGroupAlgorithm;

class MultiPeriodGroupAlgorithmTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiPeriodGroupAlgorithmTest *createSuite() { return new MultiPeriodGroupAlgorithmTest(); }
  static void destroySuite( MultiPeriodGroupAlgorithmTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_API_MultiPeriodGroupAlgorithmTEST_H_ */