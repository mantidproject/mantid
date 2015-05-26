#ifndef MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUMTEST_H_
#define MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertCWSDExpToMomentum.h"

using Mantid::MDAlgorithms::ConvertCWSDExpToMomentum;
using namespace Mantid::API;

class ConvertCWSDExpToMomentumTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertCWSDExpToMomentumTest *createSuite() { return new ConvertCWSDExpToMomentumTest(); }
  static void destroySuite( ConvertCWSDExpToMomentumTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUMTEST_H_ */