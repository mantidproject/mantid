#ifndef MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_
#define MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/NeutronBk2BkExpConvPVoigt.h"

using Mantid::CurveFitting::NeutronBk2BkExpConvPVoigt;

class NeutronBk2BkExpConvPVoigtTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NeutronBk2BkExpConvPVoigtTest *createSuite() { return new NeutronBk2BkExpConvPVoigtTest(); }
  static void destroySuite( NeutronBk2BkExpConvPVoigtTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_ */