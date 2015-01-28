#ifndef MANTID_MDALGORITHMS_LOADHFIRPDDATATEST_H_
#define MANTID_MDALGORITHMS_LOADHFIRPDDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/LoadHFIRPDData.h"

using Mantid::MDAlgorithms::LoadHFIRPDData;
using namespace Mantid::API;

class LoadHFIRPDDataTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadHFIRPDDataTest *createSuite() { return new LoadHFIRPDDataTest(); }
  static void destroySuite( LoadHFIRPDDataTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_MDALGORITHMS_LOADHFIRPDDATATEST_H_ */