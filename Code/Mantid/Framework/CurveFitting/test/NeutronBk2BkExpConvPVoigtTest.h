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
    func.initialize();

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
  void test_calculatePeakShape()
  {
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
    func.setParameter("Height", (24061.1-114.9)/0.0166701); // TOF=23425.

    double fwhm = func.fwhm();
    TS_ASSERT_DELTA(fwhm, 47.049, 0.001);

    // Calcualte peak profile
    vector<double> vec_tof;
    genPeak111TOF(vec_tof);
    vector<double> vecY(vec_tof.size(), 0.0);
    func.function(vecY, vec_tof);
    /*
    for (size_t i = 0; i < vec_tof.size(); ++i)
      cout << vec_tof[i] << "\t\t" << vecY[i] + 114.6 << "\n";
    */

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate TOF values for peak (111) from Fullprof's arg_si example
    */
  void genPeak111TOF(vector<double>& vec_x)
  {
    vec_x.clear();
    vec_x.push_back(23005.0000);
    vec_x.push_back(23010.0000);
    vec_x.push_back(23015.0000);
    vec_x.push_back(23020.0000);
    vec_x.push_back(23025.0000);
    vec_x.push_back(23030.0000);
    vec_x.push_back(23035.0000);
    vec_x.push_back(23040.0000);
    vec_x.push_back(23045.0000);
    vec_x.push_back(23050.0000);
    vec_x.push_back(23055.0000);
    vec_x.push_back(23060.0000);
    vec_x.push_back(23065.0000);
    vec_x.push_back(23070.0000);
    vec_x.push_back(23075.0000);
    vec_x.push_back(23080.0000);
    vec_x.push_back(23085.0000);
    vec_x.push_back(23090.0000);
    vec_x.push_back(23095.0000);
    vec_x.push_back(23100.0000);
    vec_x.push_back(23105.0000);
    vec_x.push_back(23110.0000);
    vec_x.push_back(23115.0000);
    vec_x.push_back(23120.0000);
    vec_x.push_back(23125.0000);
    vec_x.push_back(23130.0000);
    vec_x.push_back(23135.0000);
    vec_x.push_back(23140.0000);
    vec_x.push_back(23145.0000);
    vec_x.push_back(23150.0000);
    vec_x.push_back(23155.0000);
    vec_x.push_back(23160.0000);
    vec_x.push_back(23165.0000);
    vec_x.push_back(23170.0000);
    vec_x.push_back(23175.0000);
    vec_x.push_back(23180.0000);
    vec_x.push_back(23185.0000);
    vec_x.push_back(23190.0000);
    vec_x.push_back(23195.0000);
    vec_x.push_back(23200.0000);
    vec_x.push_back(23205.0000);
    vec_x.push_back(23210.0000);
    vec_x.push_back(23215.0000);
    vec_x.push_back(23220.0000);
    vec_x.push_back(23225.0000);
    vec_x.push_back(23230.0000);
    vec_x.push_back(23235.0000);
    vec_x.push_back(23240.0000);
    vec_x.push_back(23245.0000);
    vec_x.push_back(23250.0000);
    vec_x.push_back(23255.0000);
    vec_x.push_back(23260.0000);
    vec_x.push_back(23265.0000);
    vec_x.push_back(23270.0000);
    vec_x.push_back(23275.0000);
    vec_x.push_back(23280.0000);
    vec_x.push_back(23285.0000);
    vec_x.push_back(23290.0000);
    vec_x.push_back(23295.0000);
    vec_x.push_back(23300.0000);
    vec_x.push_back(23305.0000);
    vec_x.push_back(23310.0000);
    vec_x.push_back(23315.0000);
    vec_x.push_back(23320.0000);
    vec_x.push_back(23325.0000);
    vec_x.push_back(23330.0000);
    vec_x.push_back(23335.0000);
    vec_x.push_back(23340.0000);
    vec_x.push_back(23345.0000);
    vec_x.push_back(23350.0000);
    vec_x.push_back(23355.0000);
    vec_x.push_back(23360.0000);
    vec_x.push_back(23365.0000);
    vec_x.push_back(23370.0000);
    vec_x.push_back(23375.0000);
    vec_x.push_back(23380.0000);
    vec_x.push_back(23385.0000);
    vec_x.push_back(23390.0000);
    vec_x.push_back(23395.0000);
    vec_x.push_back(23400.0000);
    vec_x.push_back(23405.0000);
    vec_x.push_back(23410.0000);
    vec_x.push_back(23415.0000);
    vec_x.push_back(23420.0000);
    vec_x.push_back(23425.0000);
    vec_x.push_back(23430.0000);
    vec_x.push_back(23435.0000);
    vec_x.push_back(23440.0000);
    vec_x.push_back(23445.0000);
    vec_x.push_back(23450.0000);
    vec_x.push_back(23455.0000);
    vec_x.push_back(23460.0000);
    vec_x.push_back(23465.0000);
    vec_x.push_back(23470.0000);
    vec_x.push_back(23475.0000);
    vec_x.push_back(23480.0000);
    vec_x.push_back(23485.0000);
    vec_x.push_back(23490.0000);
    vec_x.push_back(23495.0000);
    vec_x.push_back(23500.0000);
    vec_x.push_back(23505.0000);
    vec_x.push_back(23510.0000);
    vec_x.push_back(23515.0000);
    vec_x.push_back(23520.0000);
    vec_x.push_back(23525.0000);
    vec_x.push_back(23530.0000);
    vec_x.push_back(23535.0000);
    vec_x.push_back(23540.0000);
    vec_x.push_back(23545.0000);
    vec_x.push_back(23550.0000);
    vec_x.push_back(23555.0000);
    vec_x.push_back(23560.0000);
    vec_x.push_back(23565.0000);
    vec_x.push_back(23570.0000);
    vec_x.push_back(23575.0000);
    vec_x.push_back(23580.0000);
    vec_x.push_back(23585.0000);
    vec_x.push_back(23590.0000);
    vec_x.push_back(23595.0000);
    vec_x.push_back(23600.0000);
    vec_x.push_back(23605.0000);
    vec_x.push_back(23610.0000);
    vec_x.push_back(23615.0000);
    vec_x.push_back(23620.0000);
    vec_x.push_back(23625.0000);
    vec_x.push_back(23630.0000);
    vec_x.push_back(23635.0000);
    vec_x.push_back(23640.0000);
    vec_x.push_back(23645.0000);
    vec_x.push_back(23650.0000);
    vec_x.push_back(23655.0000);
    vec_x.push_back(23660.0000);
    vec_x.push_back(23665.0000);
    vec_x.push_back(23670.0000);
    vec_x.push_back(23675.0000);
    vec_x.push_back(23680.0000);
    vec_x.push_back(23685.0000);
    vec_x.push_back(23690.0000);
    vec_x.push_back(23695.0000);
    vec_x.push_back(23700.0000);
  }


  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions: data is from Fullprof's sample: arg_si
    */
  void XcalculateVulcanPeakPositions()
  {
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

    cout << "Peak [111]: d_h = " << dh1 << ", TOF_h = " << tofh1 << ".\n";

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

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak positions: data is from Fullprof's sample: arg_si
    */
  void Xtest_calculateVulcanProfile()
  {
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

    cout << "Peak 220: TOF_h = " << tofh1 << ", FWHM = " << fwhm << ".\n";

    vector<double> vecX;
    double tof = tofh1 - 10*fwhm;
    while (tof < tofh1 + 10*fwhm)
    {
      vecX.push_back(tof);
      tof += fwhm*0.1;
    }
    vector<double> vecY(vecX.size(), 0.0);
    func.function(vecY, vecX);
    for (size_t i = 0; i < vecX.size(); ++i)
      cout << vecX[i] << "\t\t" << vecY[i] << "\n";

    return;
  }


};


#endif /* MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGTTEST_H_ */
