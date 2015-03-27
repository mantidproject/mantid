#ifndef MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTIONTEST_H_
#define MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/PoldiSpectrumPawleyFunction.h"

using Mantid::SINQ::PoldiSpectrumPawleyFunction;
using namespace Mantid::API;

class PoldiSpectrumPawleyFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiSpectrumPawleyFunctionTest *createSuite() { return new PoldiSpectrumPawleyFunctionTest(); }
  static void destroySuite( PoldiSpectrumPawleyFunctionTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_SINQ_POLDISPECTRUMPAWLEYFUNCTIONTEST_H_ */