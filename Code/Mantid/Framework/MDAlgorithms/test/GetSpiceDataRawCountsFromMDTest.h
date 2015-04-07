#ifndef MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMDTEST_H_
#define MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/GetSpiceDataRawCountsFromMD.h"

using Mantid::MDAlgorithms::GetSpiceDataRawCountsFromMD;
using namespace Mantid::API;

class GetSpiceDataRawCountsFromMDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetSpiceDataRawCountsFromMDTest *createSuite() { return new GetSpiceDataRawCountsFromMDTest(); }
  static void destroySuite( GetSpiceDataRawCountsFromMDTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_MDALGORITHMS_GETSPICEDATARAWCOUNTSFROMMDTEST_H_ */