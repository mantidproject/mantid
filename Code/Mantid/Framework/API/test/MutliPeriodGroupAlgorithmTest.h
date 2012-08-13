#ifndef MANTID_API_MUTLIPERIODGROUPALGORITHMTEST_H_
#define MANTID_API_MUTLIPERIODGROUPALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MutliPeriodGroupAlgorithm.h"

using Mantid::API::MutliPeriodGroupAlgorithm;

class MutliPeriodGroupAlgorithmTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MutliPeriodGroupAlgorithmTest *createSuite() { return new MutliPeriodGroupAlgorithmTest(); }
  static void destroySuite( MutliPeriodGroupAlgorithmTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_API_MUTLIPERIODGROUPALGORITHMTEST_H_ */