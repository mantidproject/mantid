#ifndef MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_
#define MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/NeutronBk2BkExpConvPVoigt.h"

using namespace std;
using Mantid::CurveFitting::Functions::NeutronBk2BkExpConvPVoigt;

class NeutronBk2BkExpConvPVoigtTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NeutronBk2BkExpConvPVoigtTest *createSuite() {
    return new NeutronBk2BkExpConvPVoigtTest();
  }
  static void destroySuite(NeutronBk2BkExpConvPVoigtTest *suite) {
    delete suite;
  }

  //----------------------------------------------------------------------------------------------
  /** Set and get parameter
   */
  void test_accessParameter() {
    NeutronBk2BkExpConvPVoigt func;
    func.initialize();

    func.setParameter("Dtt1", 1.0);
    double dtt1 = func.getParameter("Dtt1");
    TS_ASSERT_EQUALS(dtt1, 1.0);

    func.setParameter("Dtt2", 2.0);
    double dtt2 = func.getParameter("Dtt2");
    TS_ASSERT_EQUALS(dtt2, 2.0);

    TS_ASSERT_THROWS_ANYTHING(func.setParameter("Fake", 0.0));
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions: data is from Fullprof's sample: arg_si
   */
  void test_calculatePeakPositions() {
    // (1,1,1)
    NeutronBk2BkExpConvPVoigt func;
    func.initialize();

    func.setParameter("Dtt1", 7476.910);
    func.setParameter("Dtt2", -1.540);
    func.setParameter("Zero", -9.227);
    func.setParameter("LatticeConstant", 5.431363); // Silicon

    func.setMillerIndex(1, 1, 1);
    func.calculateParameters(false);
    double tofh1 = func.centre();
    TS_ASSERT_DELTA(tofh1, 23421.7207, 0.01);

    // (2, 2, 0)
    NeutronBk2BkExpConvPVoigt func220;
    func220.initialize();

    func220.setParameter("Dtt1", 7476.910);
    func220.setParameter("Dtt2", -1.540);
    func220.setParameter("Zero", -9.227);
    func220.setParameter("LatticeConstant", 5.431363); // Silicon
    func220.setMillerIndex(2, 2, 0);
    func220.calculateParameters(false);
    double tofh2 = func220.centre();
    TS_ASSERT_DELTA(tofh2, 14342.8350, 0.01);

    // (3,1,1)
    NeutronBk2BkExpConvPVoigt func311;
    func311.initialize();

    func311.setParameter("Dtt1", 7476.910);
    func311.setParameter("Dtt2", -1.540);
    func311.setParameter("Zero", -9.227);
    func311.setParameter("LatticeConstant", 5.431363); // Silicon

    func311.setMillerIndex(3, 1, 1);
    func311.calculateParameters(false);
    double tofh3 = func311.centre();
    TS_ASSERT_DELTA(tofh3, 12230.9648, 0.01);

    // (2, 2, 2)
    NeutronBk2BkExpConvPVoigt func222;
    func222.initialize();

    func222.setParameter("Dtt1", 7476.910);
    func222.setParameter("Dtt2", -1.540);
    func222.setParameter("Zero", -9.227);
    func222.setParameter("LatticeConstant", 5.431363); // Silicon
    func222.setMillerIndex(2, 2, 2);

    func222.calculateParameters(false);
    double tofh4 = func222.centre();
    TS_ASSERT_DELTA(tofh4, 11710.0332, 0.01);
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions: data is from Fullprof's sample: arg_si
   */
  void test_calculatePeakShape() {
    NeutronBk2BkExpConvPVoigt func;
    func.initialize();

    func.setParameter("Dtt1", 7476.910);
    func.setParameter("Dtt2", -1.540);
    func.setParameter("Zero", -9.227);
    func.setParameter("LatticeConstant", 5.431363); // Silicon

    func.setParameter("Alph0", 0.000000);
    func.setParameter("Alph1", 0.597100);
    func.setParameter("Beta0", 0.042210);
    func.setParameter("Beta1", 0.009460);
    func.setParameter("Sig0", sqrt(3.032));
    func.setParameter("Sig1", sqrt(33.027));
    func.setParameter("Sig2", 0.000);
    func.setParameter("Gam0", 0.000);
    func.setParameter("Gam1", 2.604);
    func.setParameter("Gam2", 0.000);

    func.setMillerIndex(1, 1, 1);
    func.calculateParameters(false);

    // Peak centre
    double tofh1 = func.centre();
    TS_ASSERT_DELTA(tofh1, 23421.7207, 0.01);

    // Peak shape
    func.setParameter("Height", (24061.1 - 114.9) / 0.0166701); // TOF=23425.

    double fwhm = func.fwhm();
    TS_ASSERT_DELTA(fwhm, 47.049, 0.001);
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions: data is from Fullprof's sample: arg_si
   */
  void calculateVulcanPeakPositions() {
    // TODO - This will be left to the ticket for VULCAN
    // (2, 2, 0)
    NeutronBk2BkExpConvPVoigt func;
    func.initialize();

    func.setParameter("Dtt1", 16370.650);
    func.setParameter("Dtt2", 0.100);
    func.setParameter("Zero", 0.000);
    func.setParameter("LatticeConstant", 5.431363); // Silicon

    func.setMillerIndex(3, 3, 1);
    func.calculateParameters(false);
    double dh1 = func.getPeakParameter("d_h");
    double tofh1 = func.centre();

    std::stringstream str;
    str << "Peak [111]: d_h = " << dh1 << ", TOF_h = " << tofh1 << ".\n";
    TS_TRACE(str.str());

    // TODO - Find out Vulcan's 220 peak centre's range.
    // TS_ASSERT_DELTA(tofh1, 23421.7207, 0.01);

    /*
    // (2, 2, 0)
    NeutronBk2BkExpConvPVoigt func220;
    func220.initialize();

    func220.setParameter("Dtt1", 16370.650);
    func220.setParameter("Dtt2", -1.540);
    func220.setParameter("Zero", -9.227);
    func220.setParameter("LatticeConstant", 5.431363); // Silicon
    func220.setMillerIndex(2, 2, 0);
    func220.calculateParameters(false);
    double tofh2 = func220.centre();
    TS_ASSERT_DELTA(tofh2, 14342.8350, 0.01);

    // (3,1,1)
    NeutronBk2BkExpConvPVoigt func311;
    func311.initialize();

    func311.setParameter("Dtt1", 7476.910);
    func311.setParameter("Dtt2", -1.540);
    func311.setParameter("Zero", -9.227);
    func311.setParameter("LatticeConstant", 5.431363); // Silicon

    func311.setMillerIndex(3, 1, 1);
    func311.calculateParameters(false);
    double tofh3 = func311.centre();
    TS_ASSERT_DELTA(tofh3, 12230.9648, 0.01);

    // (2, 2, 2)
    NeutronBk2BkExpConvPVoigt func222;
    func222.initialize();

    func222.setParameter("Dtt1", 7476.910);
    func222.setParameter("Dtt2", -1.540);
    func222.setParameter("Zero", -9.227);
    func222.setParameter("LatticeConstant", 5.431363); // Silicon
    func222.setMillerIndex(2, 2, 2);

    func222.calculateParameters(false);
    double tofh4 = func222.centre();
    TS_ASSERT_DELTA(tofh4, 11710.0332, 0.01);
    */
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions: data is from Fullprof's sample: arg_si
      NOTE: this test is currently disabled and needs fixing.
    */
  void XXXtest_calculateVulcanProfile() {
    NeutronBk2BkExpConvPVoigt func;
    func.initialize();

    func.setParameter("Dtt1", 16370.650);
    func.setParameter("Dtt2", 0.100);
    func.setParameter("Zero", 0.000);
    func.setParameter("LatticeConstant", 5.431363); // Silicon

    func.setParameter("Alph0", 1.000000);
    func.setParameter("Alph1", 0.000000);
    func.setParameter("Beta0", 0.109036);
    func.setParameter("Beta1", 0.009834);
    func.setParameter("Sig0", sqrt(0.000));
    func.setParameter("Sig1", sqrt(1119.230));
    func.setParameter("Sig2", sqrt(91.127));
    func.setParameter("Gam0", 0.000);
    func.setParameter("Gam1", 2.604);
    func.setParameter("Gam2", 0.000);

    // Peak 220
    func.setMillerIndex(2, 2, 0);
    func.calculateParameters(false);

    // Peak centre
    double tofh1 = func.centre();
    TS_ASSERT_DELTA(tofh1, 23421.7207, 0.01);

    // Peak shape
    func.setParameter("Height", 1.0);

    double fwhm = func.fwhm();
    TS_ASSERT_DELTA(fwhm, 47.049, 0.001);

    std::stringstream str;
    str << "Peak 220: TOF_h = " << tofh1 << ", FWHM = " << fwhm << ".\n";
    TS_TRACE(str.str());

    vector<double> vecX;
    double tof = tofh1 - 10 * fwhm;
    while (tof < tofh1 + 10 * fwhm) {
      vecX.push_back(tof);
      tof += fwhm * 0.1;
    }
    vector<double> vecY(vecX.size(), 0.0);
    func.function(vecY, vecX);
    for (size_t i = 0; i < vecX.size(); ++i) {
      std::stringstream str;
      str << vecX[i] << "\t\t" << vecY[i] << "\n";
      TS_TRACE(str.str());
    }
  }
};

#endif /* MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_ */
