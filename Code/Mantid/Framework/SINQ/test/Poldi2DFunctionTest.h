#ifndef MANTID_SINQ_POLDI2DFUNCTIONTEST_H_
#define MANTID_SINQ_POLDI2DFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"

using Mantid::SINQ::Poldi2DFunction;
using namespace Mantid::API;

class Poldi2DFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Poldi2DFunctionTest *createSuite() { return new Poldi2DFunctionTest(); }
  static void destroySuite( Poldi2DFunctionTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_SINQ_POLDI2DFUNCTIONTEST_H_ */