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
    func.setParameter("Dtt1", 1.0);
    double dtt1 = func.getParameter("Dtt1");
    TS_ASSERT_EQUALS(dtt1, 1.0);

    func.setParameter("Dtt2", 2.0);
    double dtt2 = func.getParameter("Dtt2");
    TS_ASSERT_EQUALS(dtt2, 2.0);

    TS_ASSERT_THROWS_ANYTHING(func.setParameter("Fake", 0.0));

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions: data is from Fullprof's sample: arg_si
    */
  void test_calculatePeakPositions()
  {
    NeutronBk2BkExpConvPVoigt func;

    func.setParameter("Dtt1", 7476.910);
    func.setParameter("Dtt2", -1.540);
    func.setParameter("Zero", -9.227);
    func.setParameter("LatticeConstant", 5.431363); // Silicon

    // (1,1,1)
    func.setMillerIndex(1, 1, 1);
    func.calculateParameters(false);
    double tofh1 = func.centre();
    TS_ASSERT_DELTA(tofh1, 23421.7207, 0.01);

    // (2, 2, 0)
    func.setMillerIndex(2, 2, 0);
    func.calculateParameters(false);
    double tofh2 = func.centre();
    TS_ASSERT_DELTA(tofh2, 14342.8350, 0.01);

    // (3,1,1)
    func.setMillerIndex(3, 1, 1);
    func.calculateParameters(false);
    double tofh3 = func.centre();
    TS_ASSERT_DELTA(tofh3, 12230.9648, 0.01);

    // (2, 2, 2)
    func.setMillerIndex(2, 2, 2);
    func.calculateParameters(false);
    double tofh4 = func.centre();
    TS_ASSERT_DELTA(tofh4, 11710.0332, 0.01);

  }
  /* From arg_si.prf
  11710.0332	0	(  2  2  2)	  0  1
  12230.9648	0	(  3  1  1)	  0  1
  14342.8350	0	(  2  2  0)	  0  1
  23421.7207    0	(  1  1  1)	  0  1
    */

  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions: data is from Fullprof's sample: arg_si
    */
  void test_calculatePeakShape()
  {
    NeutronBk2BkExpConvPVoigt func;

    func.setParameter("Dtt1", 7476.910);
    func.setParameter("Dtt2", -1.540);
    func.setParameter("Zero", -9.227);
    func.setParameter("LatticeConstant", 5.431363); // Silicon

    func.setParameter("Alph0", 0.000000);
    func.setParameter("Alph1", 0.597100);
    func.setParameter("Beta0", 0.042210);
    func.setParameter("Beta1", 0.009460);
    func.setParameter("Sig0", 3.032);
    func.setParameter("Sig1", 33.027);
    func.setParameter("Sig2", 0.000);
    func.setParameter("Gam0", 0.000);
    func.setParameter("Gam0", 2.604);
    func.setParameter("Gam0", 0.000);

    func.setMillerIndex(1, 1, 0);
    func.calculateParameters(false);
    double tofh1 = func.centre();
    TS_ASSERT_DELTA(tofh1, 0.0, 0.01);

  }

};


#endif /* MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_ */
