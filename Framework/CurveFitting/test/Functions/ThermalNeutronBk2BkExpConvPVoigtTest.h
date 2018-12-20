#ifndef MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_

#include <array>
#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/ThermalNeutronBk2BkExpConvPVoigt.h"

using namespace Mantid;
using namespace Kernel;
using namespace std;
using Mantid::CurveFitting::Functions::ThermalNeutronBk2BkExpConvPVoigt;

class ThermalNeutronBk2BkExpConvPVoigtTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThermalNeutronBk2BkExpConvPVoigtTest *createSuite() {
    return new ThermalNeutronBk2BkExpConvPVoigtTest();
  }
  static void destroySuite(ThermalNeutronBk2BkExpConvPVoigtTest *suite) {
    delete suite;
  }

  /** Test overriden set parameter value functions
   */
  void test_setParameter() {
    ThermalNeutronBk2BkExpConvPVoigt peak;
    peak.initialize();

    vector<string> paramnames = peak.getParameterNames();
    TS_ASSERT_EQUALS(paramnames[0].compare("Height"), 0);

    // Set parameter, get it and compare
    peak.setParameter(1, 123.4);
    double parvalue1 = peak.getParameter(1);
    TS_ASSERT_DELTA(123.4, parvalue1, 0.00001);
    cout << "Parameter 1: Set To 123.4.  Value got = " << parvalue1 << '\n';

    // Set parameter, get it and compare
    peak.setParameter("Dtt1", 123456.78);
    double parvalue2 = peak.getParameter("Dtt1");
    TS_ASSERT_DELTA(123456.78, parvalue2, 0.00001);

    TS_ASSERT_DELTA(123.4, parvalue1, 0.00001);
    cout << "Parameter Dtt1: Set To 123456.78.  Value got = " << parvalue2
         << '\n';
  }

  /** Test on calcualte peak parameters
   */
  void test_CalculatePeakParameters() {
    // 0. Mock data
    auto vecX = generateData();

    // 1. Create peak
    ThermalNeutronBk2BkExpConvPVoigt peak;
    peak.initialize();

    peak.setMillerIndex(1, 1, 1);

    // 2. Set up parameters
    peak.setParameter("Dtt1", 29671.7500);
    peak.setParameter("Dtt2", 0.0);
    peak.setParameter("Dtt1t", 29671.750);
    peak.setParameter("Dtt2t", 0.30);

    peak.setParameter("Zero", 0.0);
    peak.setParameter("Zerot", 33.70);

    peak.setParameter("Alph0", 4.026);
    peak.setParameter("Alph1", 7.362);
    peak.setParameter("Beta0", 3.489);
    peak.setParameter("Beta1", 19.535);

    peak.setParameter("Alph0t", 60.683);
    peak.setParameter("Alph1t", 39.730);
    peak.setParameter("Beta0t", 96.864);
    peak.setParameter("Beta1t", 96.864);

    peak.setParameter("Sig2", sqrt(11.380));
    peak.setParameter("Sig1", sqrt(9.901));
    peak.setParameter("Sig0", sqrt(17.370));

    peak.setParameter("Width", 1.0055);
    peak.setParameter("Tcross", 0.4700);

    peak.setParameter("Gam0", 0.0);
    peak.setParameter("Gam1", 0.0);
    peak.setParameter("Gam2", 0.0);

    peak.setParameter("LatticeConstant", 4.156890);

    // double d1 = 2.399981; // 1 1 1
    double h1 = 1370.0 / 0.008;
    peak.setParameter("Height", h1);

    // 3. Parameter check
    double tof_h = peak.centre();
    double fwhm = peak.fwhm();
    TS_ASSERT_DELTA(tof_h, 71229.45, 0.1);
    TS_ASSERT_DELTA(fwhm, 50.0613, 0.0001);

    cout << "TOF_h = " << tof_h << ", FWHM = " << fwhm << '\n';

    // 4. Calculate
    size_t nData = vecX.size();
    double *xvalues = new double[nData];
    double *out = new double[nData];
    for (size_t i = 0; i < nData; ++i) {
      xvalues[i] = vecX[i];
      out[i] = 0.0;
    }
    peak.function1D(out, xvalues, nData);

    // 4. Compare calculated data
    double y25 = 1360.27;
    TS_ASSERT_DELTA(out[25], y25, 0.01);

    delete[] xvalues;
    delete[] out;

    return;
  }

  /** Test behavior of E1()
   */
  void test_E1() {
    // 0. Mock data
    auto vecX = generateData();

    // 1. Create peak
    ThermalNeutronBk2BkExpConvPVoigt peak;
    peak.initialize();

    peak.setMillerIndex(1, 1, 1);

    // 2. Set up parameters
    peak.setParameter("Dtt1", 29671.7500);
    peak.setParameter("Dtt2", 0.0);
    peak.setParameter("Dtt1t", 29671.750);
    peak.setParameter("Dtt2t", 0.30);

    peak.setParameter("Zero", 0.0);
    peak.setParameter("Zerot", 33.70);

    peak.setParameter("Alph0", 4.026);
    peak.setParameter("Alph1", 7.362);
    peak.setParameter("Beta0", 3.489);
    peak.setParameter("Beta1", 19.535);

    peak.setParameter("Alph0t", 60.683);
    peak.setParameter("Alph1t", 39.730);
    peak.setParameter("Beta0t", 96.864);
    peak.setParameter("Beta1t", 96.864);

    peak.setParameter("Sig2", sqrt(11.380));
    peak.setParameter("Sig1", sqrt(9.901));
    peak.setParameter("Sig0", sqrt(17.370));

    peak.setParameter("Width", 1.0055);
    peak.setParameter("Tcross", 0.4700);

    peak.setParameter("Gam0", 10.0);
    peak.setParameter("Gam1", 0.0);
    peak.setParameter("Gam2", 0.0);

    peak.setParameter("LatticeConstant", 4.156890);

    // double d1 = 2.399981; // 1 1 1
    double h1 = 1370.0 / 0.008;
    peak.setParameter("Height", h1);

    // 3. Parameter check
    double tof_h = peak.centre();
    double fwhm = peak.fwhm();
    TS_ASSERT_DELTA(tof_h, 71229.45, 0.1);
    TS_ASSERT_DELTA(fwhm, 55.0613, 0.5);

    // 3. Calculate
    size_t nData = vecX.size();
    double *xvalues = new double[nData];
    double *out = new double[nData];
    for (size_t i = 0; i < nData; ++i) {
      xvalues[i] = vecX[i];
      out[i] = 0.0;
    }
    peak.function1D(out, xvalues, nData);

    // 4. Compare calculated data
    double y25 = 1421.27;
    TS_ASSERT_DELTA(out[25], y25, 1.0);

    delete[] xvalues;
    delete[] out;

    return;
  }

  /** Test on calcualte peak parameters including Gamma (i.e., E1())
   * Parameter and data is from PG3_11485, Bank 1, (200) @ TOF = 46963
   */
  void test_CalculatePeakParameters2() {
    // 1. Mock data
    std::vector<double> vecX;
    std::vector<double> modelY;
    generateData2(vecX, modelY);

    // 3. Create peak
    ThermalNeutronBk2BkExpConvPVoigt peak;
    peak.initialize();

    peak.setMillerIndex(2, 0, 0);

    // 3. Set up parameters
    peak.setParameter("Dtt1", 22584.51172);
    peak.setParameter("Dtt2", 0.0);
    peak.setParameter("Dtt1t", 22604.85156);
    peak.setParameter("Dtt2t", 0.30);

    peak.setParameter("Zero", 0.0);
    peak.setParameter("Zerot", 11.31754);

    peak.setParameter("Alph0", 1.881868);
    peak.setParameter("Alph1", 0.000);
    peak.setParameter("Beta0", 6.251096);
    peak.setParameter("Beta1", 0.000);

    peak.setParameter("Alph0t", 64.410156);
    peak.setParameter("Alph1t", 0.000);
    peak.setParameter("Beta0t", 85.918922);
    peak.setParameter("Beta1t", 0.000);

    peak.setParameter("Sig2", sqrt(279.996));
    peak.setParameter("Sig1", sqrt(10.0));
    peak.setParameter("Sig0", 0.0);

    peak.setParameter("Width", 1.0521);
    peak.setParameter("Tcross", 0.3560);

    peak.setParameter("Gam0", 0.0);
    peak.setParameter("Gam1", 5.744);
    // peak.setParameter("Gam1", 0.0);
    peak.setParameter("Gam2", 0.0);

    peak.setParameter("LatticeConstant", 4.156890);

    double bkgd = 0.1;
    double h1 = (3.666 - 0.1) / 0.005;
    // h1 = 1.0;
    peak.setParameter("Height", h1);

    // 3. Parameter check
    peak.calculateParameters(true);
    /*
    double d_h = peak.getPeakParameter("d_h");
    double alpha = peak.getPeakParameter("Alpha");
    double beta = peak.getPeakParameter("Beta");
    double sigma2 = peak.getPeakParameter("Sigma2");
    double gamma = peak.getPeakParameter("Gamma");
    double eta = peak.getPeakParameter("Eta");
    double tof_h = peak.centre();
    double fwhm = peak.fwhm();
    cout << "Peak (200) Parameters: \n";
    cout << "Centre (TOF) = " << tof_h << ", Centre (d) = " << d_h << "\n";
    cout << "Alpha  = " << alpha << ", Beta  = " << beta << "\n";
    cout << "Sigma2 = " << sigma2 << ", Gamma = " << gamma << "\n";
    cout << "Peak height = " << h1 << ", FWHM = " << fwhm << ", Eta = " << eta
    << '\n';
    */

    // 4. Calculate
    size_t nData = vecX.size();
    vector<double> out(nData, 0.0);
    peak.function(out, vecX);

    /*
    std::stringstream outstring;
    for (size_t id = 0; id < nData; ++id)
    {
      outstring << vecX[id] << "\t\t" << out[id] << '\n';
    }
    std::ofstream ofile;
    ofile.open("peaks_200.dat");
    ofile << outstring.str();
    ofile.close();
    */

    // 4. Compare calculated data
    TS_ASSERT_DELTA(out[0] + bkgd, modelY[0], 0.1);
    TS_ASSERT_DELTA(out[25] + bkgd, modelY[25], 0.1);
    TS_ASSERT_DELTA(out[45] + bkgd, modelY[45], 0.2);
    TS_ASSERT_DELTA(out[65] + bkgd, modelY[65], 0.1);

    return;
  }

  /** Generate a set of powder diffraction data with 2 peaks (110 and 111)
   */
  std::array<double, 127> generateData() {
    std::array<double, 127> vecX = {
        {70931.750000, 70943.609000, 70955.477000, 70967.336000, 70979.203000,
         70991.063000, 71002.930000, 71014.789000, 71026.656000, 71038.516000,
         71050.383000, 71062.242000, 71074.109000, 71085.969000, 71097.836000,
         71109.695000, 71121.563000, 71133.430000, 71145.289000, 71157.156000,
         71169.016000, 71180.883000, 71192.742000, 71204.609000, 71216.469000,
         71228.336000, 71240.195000, 71252.063000, 71263.922000, 71275.789000,
         71287.648000, 71299.516000, 71311.375000, 71323.242000, 71335.102000,
         71346.969000, 71358.836000, 71370.695000, 71382.563000, 71394.422000,
         71406.289000, 71418.148000, 71430.016000, 71441.875000, 71453.742000,
         71465.602000, 71477.469000, 71489.328000, 71501.195000, 71513.055000,
         71524.922000, 71536.781000, 71548.648000, 71560.508000, 71572.375000,
         71584.242000, 71596.102000, 71607.969000, 71619.828000, 86911.852000,
         86923.719000, 86935.578000, 86947.445000, 86959.305000, 86971.172000,
         86983.039000, 86994.898000, 87006.766000, 87018.625000, 87030.492000,
         87042.352000, 87054.219000, 87066.078000, 87077.945000, 87089.805000,
         87101.672000, 87113.531000, 87125.398000, 87137.258000, 87149.125000,
         87160.984000, 87172.852000, 87184.711000, 87196.578000, 87208.445000,
         87220.305000, 87232.172000, 87244.031000, 87255.898000, 87267.758000,
         87279.625000, 87291.484000, 87303.352000, 87315.211000, 87327.078000,
         87338.938000, 87350.805000, 87362.664000, 87374.531000, 87386.391000,
         87398.258000, 87410.117000, 87421.984000, 87433.844000, 87445.711000,
         87457.578000, 87469.438000, 87481.305000, 87493.164000, 87505.031000,
         87516.891000, 87528.758000, 87540.617000, 87552.484000, 87564.344000,
         87576.211000, 87588.070000, 87599.938000, 87611.797000, 87623.664000,
         87635.523000, 87647.391000, 87659.250000, 87671.117000, 87682.984000,
         87694.844000, 87706.711000}};

    return vecX;
  }

  /** Generate data from PG3_11485 Jason refined .prf file
   * @param vecX:   x-values
   * @param dataY:  experimental y-values
   * @param modelY: calculated y-values
   */
  void generateData2(std::vector<double> &vecX, std::vector<double> &modelY) {
    vecX.clear();
    modelY.clear();

    vecX = {
        46129.156200, 46147.609400, 46166.066400, 46184.535200, 46203.007800,
        46221.488300, 46239.976600, 46258.472700, 46276.976600, 46295.488300,
        46314.003900, 46332.531200, 46351.062500, 46369.605500, 46388.152300,
        46406.707000, 46425.269500, 46443.839800, 46462.418000, 46481.003900,
        46499.593800, 46518.195300, 46536.800800, 46555.418000, 46574.039100,
        46592.668000, 46611.304700, 46629.949200, 46648.601600, 46667.261700,
        46685.929700, 46704.601600, 46723.285200, 46741.972700, 46760.671900,
        46779.375000, 46798.085900, 46816.804700, 46835.531200, 46854.265600,
        46873.007800, 46891.757800, 46910.515600, 46929.277300, 46948.050800,
        46966.828100, 46985.617200, 47004.410200, 47023.210900, 47042.023400,
        47060.839800, 47079.664100, 47098.496100, 47117.335900, 47136.179700,
        47155.035200, 47173.898400, 47192.765600, 47211.644500, 47230.527300,
        47249.421900, 47268.320300, 47287.226600, 47306.144500, 47325.066400,
        47343.996100, 47362.933600, 47381.878900, 47400.832000, 47419.793000,
        47438.757800, 47457.734400, 47476.718800, 47495.707000, 47514.707000,
        47533.714800, 47552.726600, 47571.746100, 47590.777300, 47609.812500,
        47628.855500, 47647.906200, 47666.968800, 47686.035200, 47705.109400,
        47724.191400, 47743.281200, 47762.378900, 47781.484400, 47800.593800,
        47819.714800, 47838.843800, 47857.980500, 47877.121100, 47896.273400,
        47915.429700, 47934.597700, 47953.769500, 47972.953100, 47992.140600};
    modelY = {
        0.118500, 0.118500, 0.118600, 0.118600, 0.118700, 0.118800, 0.118900,
        0.119000, 0.119100, 0.119300, 0.119400, 0.119600, 0.119800, 0.120100,
        0.120300, 0.120600, 0.121000, 0.121400, 0.121800, 0.122300, 0.122900,
        0.123600, 0.124400, 0.125300, 0.126500, 0.128000, 0.130100, 0.133000,
        0.137400, 0.144300, 0.155500, 0.173800, 0.203800, 0.252000, 0.327800,
        0.442700, 0.609800, 0.841400, 1.145800, 1.522400, 1.958300, 2.425400,
        2.881400, 3.275300, 3.556300, 3.684700, 3.643000, 3.439600, 3.106400,
        2.689400, 2.238900, 1.798900, 1.402000, 1.066700, 0.799000, 0.595100,
        0.446100, 0.340700, 0.268100, 0.219100, 0.186300, 0.164500, 0.150000,
        0.140200, 0.133500, 0.128700, 0.125400, 0.122800, 0.120900, 0.119400,
        0.118100, 0.117100, 0.116200, 0.115400, 0.114700, 0.114100, 0.113500,
        0.112900, 0.112400, 0.112000, 0.111500, 0.111100, 0.110700, 0.110400,
        0.110000, 0.109700, 0.109300, 0.109000, 0.108700, 0.108400, 0.108100,
        0.107900, 0.107600, 0.107300, 0.107100, 0.106800, 0.106600, 0.106300,
        0.106100, 0.105800};
  }
};

#endif /* MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_ */
