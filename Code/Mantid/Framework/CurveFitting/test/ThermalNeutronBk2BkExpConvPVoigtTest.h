#ifndef MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>
#include <cmath>

#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPVoigt.h"

using namespace Mantid;
using namespace Kernel;
using Mantid::CurveFitting::ThermalNeutronBk2BkExpConvPVoigt;

class ThermalNeutronBk2BkExpConvPVoigtTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThermalNeutronBk2BkExpConvPVoigtTest *createSuite() { return new ThermalNeutronBk2BkExpConvPVoigtTest(); }
  static void destroySuite( ThermalNeutronBk2BkExpConvPVoigtTest *suite ) { delete suite; }

  /** Test overriden set parameter value functions
    */
  void test_setParameter()
  {
    ThermalNeutronBk2BkExpConvPVoigt peak;
    peak.initialize();

    vector<string> paramnames = peak.getParameterNames();
    TS_ASSERT_EQUALS(paramnames[0].compare("Height"), 0);

    // Set parameter, get it and compare
    peak.setParameter(1, 123.4);
    double parvalue1 = peak.getParameter(1);
    TS_ASSERT_DELTA(123.4, parvalue1, 0.00001);
    cout << "Parameter 1: Set To 123.4.  Value got = " << parvalue1 << endl;

    // Set parameter, get it and compare
    peak.setParameter("Dtt1", 123456.78);
    double parvalue2 = peak.getParameter("Dtt1");
    TS_ASSERT_DELTA(123456.78, parvalue2, 0.00001);

    TS_ASSERT_DELTA(123.4, parvalue1, 0.00001);
    cout << "Parameter Dtt1: Set To 123456.78.  Value got = " << parvalue2 << endl;

  }

  /** Test on calcualte peak parameters
   */
  void test_CalculatePeakParameters()
  {
    // 0. Mock data
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;
    generateData(vecX, vecY, vecE);

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

    peak.setParameter("Sig2",  sqrt(11.380));
    peak.setParameter("Sig1",   sqrt(9.901));
    peak.setParameter("Sig0",  sqrt(17.370));

    peak.setParameter("Width", 1.0055);
    peak.setParameter("Tcross", 0.4700);

    peak.setParameter("Gam0", 0.0);
    peak.setParameter("Gam1", 0.0);
    peak.setParameter("Gam2", 0.0);

    peak.setParameter("LatticeConstant", 4.156890);

    // double d1 = 2.399981; // 1 1 1
    double h1 = 1370.0/0.008;
    peak.setParameter("Height", h1);

    // 3. Parameter check
    double tof_h = peak.centre();
    double fwhm = peak.fwhm();
    TS_ASSERT_DELTA(tof_h, 71229.45, 0.1);
    TS_ASSERT_DELTA(fwhm, 50.0613, 0.0001);

    cout << "TOF_h = " << tof_h << ", FWHM = " << fwhm << endl;

    // 4. Calculate
    size_t nData = vecX.size();
    double* xvalues = new double[nData];
    double* out = new double[nData];
    for (size_t i = 0; i < nData; ++i)
    {
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
  void test_E1()
  {
    // 0. Mock data
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;
    generateData(vecX, vecY, vecE);

    // 1. Create peak
    CurveFitting::ThermalNeutronBk2BkExpConvPVoigt peak;
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

    peak.setParameter("Sig2",  sqrt(11.380));
    peak.setParameter("Sig1",  sqrt(9.901));
    peak.setParameter("Sig0",  sqrt(17.370));

    peak.setParameter("Width", 1.0055);
    peak.setParameter("Tcross", 0.4700);

    peak.setParameter("Gam0", 10.0);
    peak.setParameter("Gam1", 0.0);
    peak.setParameter("Gam2", 0.0);

    peak.setParameter("LatticeConstant", 4.156890);

    // double d1 = 2.399981; // 1 1 1
    double h1 = 1370.0/0.008;
    peak.setParameter("Height", h1);

    // 3. Parameter check
    double tof_h = peak.centre();
    double fwhm = peak.fwhm();
    TS_ASSERT_DELTA(tof_h, 71229.45, 0.1);
    TS_ASSERT_DELTA(fwhm, 55.0613, 0.5);

    // 3. Calculate
    size_t nData = vecX.size();
    double* xvalues = new double[nData];
    double* out = new double[nData];
    for (size_t i = 0; i < nData; ++i)
    {
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
  void test_CalculatePeakParameters2()
  {
    // 1. Mock data
    std::vector<double> vecX;
    std::vector<double> dataY;
    std::vector<double> modelY;
    generateData2(vecX, dataY, modelY);

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
    peak.setParameter("Beta0t", 85.918922 );
    peak.setParameter("Beta1t", 0.000);

    peak.setParameter("Sig2",  sqrt(279.996));
    peak.setParameter("Sig1",  sqrt(10.0));
    peak.setParameter("Sig0",  0.0);

    peak.setParameter("Width", 1.0521);
    peak.setParameter("Tcross", 0.3560);

    peak.setParameter("Gam0", 0.0);
    peak.setParameter("Gam1", 5.744);
    // peak.setParameter("Gam1", 0.0);
    peak.setParameter("Gam2", 0.0);

    peak.setParameter("LatticeConstant", 4.156890);

    double bkgd = 0.1;
    double h1 = (3.666-0.1)/0.005;
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
    cout << "Peak height = " << h1 << ", FWHM = " << fwhm << ", Eta = " << eta << endl;
    */

    // 4. Calculate
    size_t nData = vecX.size();
    vector<double> out(nData, 0.0);
    peak.function(out, vecX);

    /*
    std::stringstream outstring;
    for (size_t id = 0; id < nData; ++id)
    {
      outstring << vecX[id] << "\t\t" << out[id] << std::endl;
    }
    std::ofstream ofile;
    ofile.open("peaks_200.dat");
    ofile << outstring.str();
    ofile.close();
    */

    // 4. Compare calculated data
    TS_ASSERT_DELTA(out[0]+bkgd, modelY[0], 0.1);
    TS_ASSERT_DELTA(out[25]+bkgd, modelY[25], 0.1);
    TS_ASSERT_DELTA(out[45]+bkgd, modelY[45], 0.2);
    TS_ASSERT_DELTA(out[65]+bkgd, modelY[65], 0.1);


    return;
  }


  /** Generate a set of powder diffraction data with 2 peaks (110 and 111)
   */
  void generateData(std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
  {
    vecX.push_back(70931.750);    vecY.push_back(    0.0000000    );
    vecX.push_back(70943.609);    vecY.push_back(    0.0000000    );
    vecX.push_back(70955.477);    vecY.push_back(   0.69562334    );
    vecX.push_back(70967.336);    vecY.push_back(   0.99016321    );
    vecX.push_back(70979.203);    vecY.push_back(    1.4097446    );
    vecX.push_back(70991.063);    vecY.push_back(    2.0066566    );
    vecX.push_back(71002.930);    vecY.push_back(    2.8569770    );
    vecX.push_back(71014.789);    vecY.push_back(    4.0666742    );
    vecX.push_back(71026.656);    vecY.push_back(    5.7899261    );
    vecX.push_back(71038.516);    vecY.push_back(    8.2414885    );
    vecX.push_back(71050.383);    vecY.push_back(    11.733817    );
    vecX.push_back(71062.242);    vecY.push_back(    16.702133    );
    vecX.push_back(71074.109);    vecY.push_back(    23.779659    );
    vecX.push_back(71085.969);    vecY.push_back(    33.848408    );
    vecX.push_back(71097.836);    vecY.push_back(    48.191662    );
    vecX.push_back(71109.695);    vecY.push_back(    68.596909    );
    vecX.push_back(71121.563);    vecY.push_back(    97.664757    );
    vecX.push_back(71133.430);    vecY.push_back(    139.04889    );
    vecX.push_back(71145.289);    vecY.push_back(    197.90808    );
    vecX.push_back(71157.156);    vecY.push_back(    281.60803    );
    vecX.push_back(71169.016);    vecY.push_back(    399.65021    );
    vecX.push_back(71180.883);    vecY.push_back(    562.42670    );
    vecX.push_back(71192.742);    vecY.push_back(    773.34192    );
    vecX.push_back(71204.609);    vecY.push_back(    1015.2813    );
    vecX.push_back(71216.469);    vecY.push_back(    1238.3613    );
    vecX.push_back(71228.336);    vecY.push_back(    1374.9380    );
    vecX.push_back(71240.195);    vecY.push_back(    1380.5173    );
    vecX.push_back(71252.063);    vecY.push_back(    1266.3978    );
    vecX.push_back(71263.922);    vecY.push_back(    1086.2141    );
    vecX.push_back(71275.789);    vecY.push_back(    894.75891    );
    vecX.push_back(71287.648);    vecY.push_back(    723.46112    );
    vecX.push_back(71299.516);    vecY.push_back(    581.04535    );
    vecX.push_back(71311.375);    vecY.push_back(    465.93588    );
    vecX.push_back(71323.242);    vecY.push_back(    373.45383    );
    vecX.push_back(71335.102);    vecY.push_back(    299.35800    );
    vecX.push_back(71346.969);    vecY.push_back(    239.92720    );
    vecX.push_back(71358.836);    vecY.push_back(    192.29497    );
    vecX.push_back(71370.695);    vecY.push_back(    154.14153    );
    vecX.push_back(71382.563);    vecY.push_back(    123.54013    );
    vecX.push_back(71394.422);    vecY.push_back(    99.028404    );
    vecX.push_back(71406.289);    vecY.push_back(    79.368507    );
    vecX.push_back(71418.148);    vecY.push_back(    63.620914    );
    vecX.push_back(71430.016);    vecY.push_back(    50.990391    );
    vecX.push_back(71441.875);    vecY.push_back(    40.873333    );
    vecX.push_back(71453.742);    vecY.push_back(    32.758839    );
    vecX.push_back(71465.602);    vecY.push_back(    26.259121    );
    vecX.push_back(71477.469);    vecY.push_back(    21.045954    );
    vecX.push_back(71489.328);    vecY.push_back(    16.870203    );
    vecX.push_back(71501.195);    vecY.push_back(    13.520998    );
    vecX.push_back(71513.055);    vecY.push_back(    10.838282    );
    vecX.push_back(71524.922);    vecY.push_back(    8.6865807    );
    vecX.push_back(71536.781);    vecY.push_back(    6.9630671    );
    vecX.push_back(71548.648);    vecY.push_back(    5.5807042    );
    vecX.push_back(71560.508);    vecY.push_back(    4.4734306    );
    vecX.push_back(71572.375);    vecY.push_back(    3.5853302    );
    vecX.push_back(71584.242);    vecY.push_back(    2.8735423    );
    vecX.push_back(71596.102);    vecY.push_back(    2.3033996    );
    vecX.push_back(71607.969);    vecY.push_back(    1.8461106    );
    vecX.push_back(71619.828);    vecY.push_back(    0.0000000    );
    vecX.push_back(86911.852);    vecY.push_back(   0.28651541    );
    vecX.push_back(86923.719);    vecY.push_back(   0.39156997    );
    vecX.push_back(86935.578);    vecY.push_back(   0.53503412    );
    vecX.push_back(86947.445);    vecY.push_back(   0.73121130    );
    vecX.push_back(86959.305);    vecY.push_back(   0.99911392    );
    vecX.push_back(86971.172);    vecY.push_back(    1.3654519    );
    vecX.push_back(86983.039);    vecY.push_back(    1.8661126    );
    vecX.push_back(86994.898);    vecY.push_back(    2.5498226    );
    vecX.push_back(87006.766);    vecY.push_back(    3.4847479    );
    vecX.push_back(87018.625);    vecY.push_back(    4.7614965    );
    vecX.push_back(87030.492);    vecY.push_back(    6.5073609    );
    vecX.push_back(87042.352);    vecY.push_back(    8.8915405    );
    vecX.push_back(87054.219);    vecY.push_back(    12.151738    );
    vecX.push_back(87066.078);    vecY.push_back(    16.603910    );
    vecX.push_back(87077.945);    vecY.push_back(    22.691912    );
    vecX.push_back(87089.805);    vecY.push_back(    31.005537    );
    vecX.push_back(87101.672);    vecY.push_back(    42.372311    );
    vecX.push_back(87113.531);    vecY.push_back(    57.886639    );
    vecX.push_back(87125.398);    vecY.push_back(    79.062233    );
    vecX.push_back(87137.258);    vecY.push_back(    107.82082    );
    vecX.push_back(87149.125);    vecY.push_back(    146.58661    );
    vecX.push_back(87160.984);    vecY.push_back(    197.83006    );
    vecX.push_back(87172.852);    vecY.push_back(    263.46185    );
    vecX.push_back(87184.711);    vecY.push_back(    343.08966    );
    vecX.push_back(87196.578);    vecY.push_back(    432.57846    );
    vecX.push_back(87208.445);    vecY.push_back(    522.64124    );
    vecX.push_back(87220.305);    vecY.push_back(    600.01373    );
    vecX.push_back(87232.172);    vecY.push_back(    651.22260    );
    vecX.push_back(87244.031);    vecY.push_back(    667.17743    );
    vecX.push_back(87255.898);    vecY.push_back(    646.90039    );
    vecX.push_back(87267.758);    vecY.push_back(    597.38873    );
    vecX.push_back(87279.625);    vecY.push_back(    530.12573    );
    vecX.push_back(87291.484);    vecY.push_back(    456.83890    );
    vecX.push_back(87303.352);    vecY.push_back(    386.05295    );
    vecX.push_back(87315.211);    vecY.push_back(    322.58456    );
    vecX.push_back(87327.078);    vecY.push_back(    267.96231    );
    vecX.push_back(87338.938);    vecY.push_back(    222.04863    );
    vecX.push_back(87350.805);    vecY.push_back(    183.80043    );
    vecX.push_back(87362.664);    vecY.push_back(    152.11101    );
    vecX.push_back(87374.531);    vecY.push_back(    125.85820    );
    vecX.push_back(87386.391);    vecY.push_back(    104.14707    );
    vecX.push_back(87398.258);    vecY.push_back(    86.170067    );
    vecX.push_back(87410.117);    vecY.push_back(    71.304932    );
    vecX.push_back(87421.984);    vecY.push_back(    58.996807    );
    vecX.push_back(87433.844);    vecY.push_back(    48.819309    );
    vecX.push_back(87445.711);    vecY.push_back(    40.392483    );
    vecX.push_back(87457.578);    vecY.push_back(    33.420235    );
    vecX.push_back(87469.438);    vecY.push_back(    27.654932    );
    vecX.push_back(87481.305);    vecY.push_back(    22.881344    );
    vecX.push_back(87493.164);    vecY.push_back(    18.934097    );
    vecX.push_back(87505.031);    vecY.push_back(    15.665835    );
    vecX.push_back(87516.891);    vecY.push_back(    12.963332    );
    vecX.push_back(87528.758);    vecY.push_back(    10.725698    );
    vecX.push_back(87540.617);    vecY.push_back(    8.8754158    );
    vecX.push_back(87552.484);    vecY.push_back(    7.3434072    );
    vecX.push_back(87564.344);    vecY.push_back(    6.0766010    );
    vecX.push_back(87576.211);    vecY.push_back(    5.0277033    );
    vecX.push_back(87588.070);    vecY.push_back(    4.1603775    );
    vecX.push_back(87599.938);    vecY.push_back(    3.4422443    );
    vecX.push_back(87611.797);    vecY.push_back(    2.8484249    );
    vecX.push_back(87623.664);    vecY.push_back(    2.3567512    );
    vecX.push_back(87635.523);    vecY.push_back(    1.9501896    );
    vecX.push_back(87647.391);    vecY.push_back(    1.6135623    );
    vecX.push_back(87659.250);    vecY.push_back(    1.3352078    );
    vecX.push_back(87671.117);    vecY.push_back(    1.1047342    );
    vecX.push_back(87682.984);    vecY.push_back(   0.91404319    );
    vecX.push_back(87694.844);    vecY.push_back(   0.75636220    );
    vecX.push_back(87706.711);    vecY.push_back(    0.0000000    );

    for (size_t i = 0; i < vecY.size(); ++i)
    {
      double e = 1.0;
      if (vecY[i] > 1.0)
        e = sqrt(vecY[i]);
      vecE.push_back(e);
    }

    return;
  }


  /** Generate data from PG3_11485 Jason refined .prf file
    * @param vecX:   x-values
    * @param dataY:  experimental y-values
    * @param modelY: calculated y-values
    */
  void generateData2(std::vector<double>& vecX, std::vector<double>& dataY, std::vector<double>& modelY)
  {
    vecX.clear();
    dataY.clear();
    modelY.clear();

    vecX.push_back(46129.1562);	  dataY.push_back(0.1069);	  modelY.push_back(0.1185);
    vecX.push_back(46147.6094);	  dataY.push_back(0.1143);	  modelY.push_back(0.1185);
    vecX.push_back(46166.0664);	  dataY.push_back(0.1562);	  modelY.push_back(0.1186);
    vecX.push_back(46184.5352);	  dataY.push_back(0.0627);	  modelY.push_back(0.1186);
    vecX.push_back(46203.0078);	  dataY.push_back(0.1230);	  modelY.push_back(0.1187);
    vecX.push_back(46221.4883);	  dataY.push_back(0.1411);	  modelY.push_back(0.1188);
    vecX.push_back(46239.9766);	  dataY.push_back(0.1920);	  modelY.push_back(0.1189);
    vecX.push_back(46258.4727);	  dataY.push_back(0.0745);	  modelY.push_back(0.1190);
    vecX.push_back(46276.9766);	  dataY.push_back(0.0897);	  modelY.push_back(0.1191);
    vecX.push_back(46295.4883);	  dataY.push_back(0.1671);	  modelY.push_back(0.1193);
    vecX.push_back(46314.0039);	  dataY.push_back(0.2592);	  modelY.push_back(0.1194);
    vecX.push_back(46332.5312);	  dataY.push_back(0.0952);	  modelY.push_back(0.1196);
    vecX.push_back(46351.0625);	  dataY.push_back(0.1850);	  modelY.push_back(0.1198);
    vecX.push_back(46369.6055);	  dataY.push_back(0.1046);	  modelY.push_back(0.1201);
    vecX.push_back(46388.1523);	  dataY.push_back(0.2446);	  modelY.push_back(0.1203);
    vecX.push_back(46406.7070);	  dataY.push_back(0.1852);	  modelY.push_back(0.1206);
    vecX.push_back(46425.2695);	  dataY.push_back(0.0756);	  modelY.push_back(0.1210);
    vecX.push_back(46443.8398);	  dataY.push_back(0.1530);	  modelY.push_back(0.1214);
    vecX.push_back(46462.4180);	  dataY.push_back(0.1813);	  modelY.push_back(0.1218);
    vecX.push_back(46481.0039);	  dataY.push_back(0.1589);	  modelY.push_back(0.1223);
    vecX.push_back(46499.5938);	  dataY.push_back(0.1438);	  modelY.push_back(0.1229);
    vecX.push_back(46518.1953);	  dataY.push_back(0.0546);	  modelY.push_back(0.1236);
    vecX.push_back(46536.8008);	  dataY.push_back(0.1724);	  modelY.push_back(0.1244);
    vecX.push_back(46555.4180);	  dataY.push_back(0.1375);	  modelY.push_back(0.1253);
    vecX.push_back(46574.0391);	  dataY.push_back(0.1136);	  modelY.push_back(0.1265);
    vecX.push_back(46592.6680);	  dataY.push_back(0.1106);	  modelY.push_back(0.1280);
    vecX.push_back(46611.3047);	  dataY.push_back(0.2025);	  modelY.push_back(0.1301);
    vecX.push_back(46629.9492);	  dataY.push_back(0.2148);	  modelY.push_back(0.1330);
    vecX.push_back(46648.6016);	  dataY.push_back(0.2909);	  modelY.push_back(0.1374);
    vecX.push_back(46667.2617);	  dataY.push_back(0.1954);	  modelY.push_back(0.1443);
    vecX.push_back(46685.9297);	  dataY.push_back(0.1355);	  modelY.push_back(0.1555);
    vecX.push_back(46704.6016);	  dataY.push_back(0.1439);	  modelY.push_back(0.1738);
    vecX.push_back(46723.2852);	  dataY.push_back(0.3487);	  modelY.push_back(0.2038);
    vecX.push_back(46741.9727);	  dataY.push_back(0.3768);	  modelY.push_back(0.2520);
    vecX.push_back(46760.6719);	  dataY.push_back(0.3047);	  modelY.push_back(0.3278);
    vecX.push_back(46779.3750);	  dataY.push_back(0.4374);	  modelY.push_back(0.4427);
    vecX.push_back(46798.0859);	  dataY.push_back(0.5702);	  modelY.push_back(0.6098);
    vecX.push_back(46816.8047);	  dataY.push_back(0.7676);	  modelY.push_back(0.8414);
    vecX.push_back(46835.5312);	  dataY.push_back(0.9643);	  modelY.push_back(1.1458);
    vecX.push_back(46854.2656);	  dataY.push_back(1.2149);	  modelY.push_back(1.5224);
    vecX.push_back(46873.0078);	  dataY.push_back(1.6902);	  modelY.push_back(1.9583);
    vecX.push_back(46891.7578);	  dataY.push_back(2.3170);	  modelY.push_back(2.4254);
    vecX.push_back(46910.5156);	  dataY.push_back(2.5934);	  modelY.push_back(2.8814);
    vecX.push_back(46929.2773);	  dataY.push_back(2.5473);	  modelY.push_back(3.2753);
    vecX.push_back(46948.0508);	  dataY.push_back(2.6097);	  modelY.push_back(3.5563);
    vecX.push_back(46966.8281);	  dataY.push_back(2.7768);	  modelY.push_back(3.6847);
    vecX.push_back(46985.6172);	  dataY.push_back(2.7972);	  modelY.push_back(3.6430);
    vecX.push_back(47004.4102);	  dataY.push_back(2.5713);	  modelY.push_back(3.4396);
    vecX.push_back(47023.2109);	  dataY.push_back(2.2840);	  modelY.push_back(3.1064);
    vecX.push_back(47042.0234);	  dataY.push_back(1.9929);	  modelY.push_back(2.6894);
    vecX.push_back(47060.8398);	  dataY.push_back(1.6574);	  modelY.push_back(2.2389);
    vecX.push_back(47079.6641);	  dataY.push_back(1.4395);	  modelY.push_back(1.7989);
    vecX.push_back(47098.4961);	  dataY.push_back(1.1935);	  modelY.push_back(1.4020);
    vecX.push_back(47117.3359);	  dataY.push_back(0.7205);	  modelY.push_back(1.0667);
    vecX.push_back(47136.1797);	  dataY.push_back(0.7175);	  modelY.push_back(0.7990);
    vecX.push_back(47155.0352);	  dataY.push_back(0.4870);	  modelY.push_back(0.5951);
    vecX.push_back(47173.8984);	  dataY.push_back(0.5124);	  modelY.push_back(0.4461);
    vecX.push_back(47192.7656);	  dataY.push_back(0.3997);	  modelY.push_back(0.3407);
    vecX.push_back(47211.6445);	  dataY.push_back(0.1794);	  modelY.push_back(0.2681);
    vecX.push_back(47230.5273);	  dataY.push_back(0.2254);	  modelY.push_back(0.2191);
    vecX.push_back(47249.4219);	  dataY.push_back(0.1645);	  modelY.push_back(0.1863);
    vecX.push_back(47268.3203);	  dataY.push_back(0.1823);	  modelY.push_back(0.1645);
    vecX.push_back(47287.2266);	  dataY.push_back(0.1327);	  modelY.push_back(0.1500);
    vecX.push_back(47306.1445);	  dataY.push_back(0.1759);	  modelY.push_back(0.1402);
    vecX.push_back(47325.0664);	  dataY.push_back(0.1218);	  modelY.push_back(0.1335);
    vecX.push_back(47343.9961);	  dataY.push_back(0.0547);	  modelY.push_back(0.1287);
    vecX.push_back(47362.9336);	  dataY.push_back(0.0376);	  modelY.push_back(0.1254);
    vecX.push_back(47381.8789);	  dataY.push_back(0.0775);	  modelY.push_back(0.1228);
    vecX.push_back(47400.8320);	  dataY.push_back(0.0823);	  modelY.push_back(0.1209);
    vecX.push_back(47419.7930);	  dataY.push_back(0.2909);	  modelY.push_back(0.1194);
    vecX.push_back(47438.7578);	  dataY.push_back(0.2262);	  modelY.push_back(0.1181);
    vecX.push_back(47457.7344);	  dataY.push_back(0.0936);	  modelY.push_back(0.1171);
    vecX.push_back(47476.7188);	  dataY.push_back(0.1618);	  modelY.push_back(0.1162);
    vecX.push_back(47495.7070);	  dataY.push_back(0.0723);	  modelY.push_back(0.1154);
    vecX.push_back(47514.7070);	  dataY.push_back(0.1148);	  modelY.push_back(0.1147);
    vecX.push_back(47533.7148);	  dataY.push_back(0.1758);	  modelY.push_back(0.1141);
    vecX.push_back(47552.7266);	  dataY.push_back(0.0785);	  modelY.push_back(0.1135);
    vecX.push_back(47571.7461);	  dataY.push_back(0.1451);	  modelY.push_back(0.1129);
    vecX.push_back(47590.7773);	  dataY.push_back(0.0517);	  modelY.push_back(0.1124);
    vecX.push_back(47609.8125);	  dataY.push_back(0.2045);	  modelY.push_back(0.1120);
    vecX.push_back(47628.8555);	  dataY.push_back(0.0000);	  modelY.push_back(0.1115);
    vecX.push_back(47647.9062);	  dataY.push_back(0.0473);	  modelY.push_back(0.1111);
    vecX.push_back(47666.9688);	  dataY.push_back(0.1876);	  modelY.push_back(0.1107);
    vecX.push_back(47686.0352);	  dataY.push_back(0.1830);	  modelY.push_back(0.1104);
    vecX.push_back(47705.1094);	  dataY.push_back(0.1113);	  modelY.push_back(0.1100);
    vecX.push_back(47724.1914);	  dataY.push_back(0.1162);	  modelY.push_back(0.1097);
    vecX.push_back(47743.2812);	  dataY.push_back(0.1065);	  modelY.push_back(0.1093);
    vecX.push_back(47762.3789);	  dataY.push_back(0.1699);	  modelY.push_back(0.1090);
    vecX.push_back(47781.4844);	  dataY.push_back(0.1461);	  modelY.push_back(0.1087);
    vecX.push_back(47800.5938);	  dataY.push_back(0.0922);	  modelY.push_back(0.1084);
    vecX.push_back(47819.7148);	  dataY.push_back(0.0729);	  modelY.push_back(0.1081);
    vecX.push_back(47838.8438);	  dataY.push_back(0.1270);	  modelY.push_back(0.1079);
    vecX.push_back(47857.9805);	  dataY.push_back(0.0582);	  modelY.push_back(0.1076);
    vecX.push_back(47877.1211);	  dataY.push_back(0.1710);	  modelY.push_back(0.1073);
    vecX.push_back(47896.2734);	  dataY.push_back(0.1609);	  modelY.push_back(0.1071);
    vecX.push_back(47915.4297);	  dataY.push_back(0.1067);	  modelY.push_back(0.1068);
    vecX.push_back(47934.5977);	  dataY.push_back(0.0627);	  modelY.push_back(0.1066);
    vecX.push_back(47953.7695);	  dataY.push_back(0.0678);	  modelY.push_back(0.1063);
    vecX.push_back(47972.9531);	  dataY.push_back(0.0723);	  modelY.push_back(0.1061);
    vecX.push_back(47992.1406);	  dataY.push_back(0.0769);	  modelY.push_back(0.1058);

  }

};

#endif /* MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_ */
