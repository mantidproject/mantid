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


  //----------------------------------------------------------------------------------------------
  /** Set and get parameter
    */
  void test_accessParameter()
  {
    NeutronBk2BkExpConvPVoigt func;
    func.setParameter("A", 1.0);
    double a = func.getParameter("A");
    TS_ASSERT_EQUALS(a, 1.0);

    // Blabla
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions
    */
  void test_calculatePeakPositions()
  {
    NeutronBk2BkExpConvPVoigt func;

    func.setParameter("Dtt1", 0.0);
    func.setParameter("Dtt2", 0.0);
    func.setParameter("Zero", 0.0);
    func.setParameter("LatticeConstant", 0.0);

    func.setMillerIndex(1, 1, 0);
    func.calculateParameters(false);
    double tofh1 = func.centre();
    TS_ASSERT_DELTA(tofh1, 0.0, 0.01);

  }


};


#endif /* MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_ */
