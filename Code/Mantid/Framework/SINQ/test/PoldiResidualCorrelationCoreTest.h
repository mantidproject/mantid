#ifndef MANTID_SINQ_POLDIRESIDUALCORRELATIONCORETEST_H_
#define MANTID_SINQ_POLDIRESIDUALCORRELATIONCORETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/PoldiResidualCorrelationCore.h"

using Mantid::Poldi::PoldiResidualCorrelationCore;
using namespace Mantid::API;

class PoldiResidualCorrelationCoreTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiResidualCorrelationCoreTest *createSuite() { return new PoldiResidualCorrelationCoreTest(); }
  static void destroySuite( PoldiResidualCorrelationCoreTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_SINQ_POLDIRESIDUALCORRELATIONCORETEST_H_ */
