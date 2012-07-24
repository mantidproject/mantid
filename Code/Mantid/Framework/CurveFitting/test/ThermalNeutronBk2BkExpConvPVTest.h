#ifndef MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>

#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPV.h"

using namespace Mantid;
using namespace Kernel;
using Mantid::CurveFitting::ThermalNeutronBk2BkExpConvPV;

class ThermalNeutronBk2BkExpConvPVTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ThermalNeutronBk2BkExpConvPVTest *createSuite() { return new ThermalNeutronBk2BkExpConvPVTest(); }
  static void destroySuite( ThermalNeutronBk2BkExpConvPVTest *suite ) { delete suite; }


  /*
   * Test on calcualte peak parameters
   */
  void test_CalculatePeakParameters()
  {
      // 0. Mock data
      std::vector<double> vecX;
      std::vector<double> vecY;
      std::vector<double> vecE;
      generateData(vecX, vecY, vecE);

      // 1. Create peak
      ThermalNeutronBk2BkExpConvPV peak;
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

      peak.setParameter("Sig2",  11.380);
      peak.setParameter("Sig1",   9.901);
      peak.setParameter("Sig0",  17.370);

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


      std::stringstream outstring;
      for (size_t id = 0; id < nData; ++id)
      {
          outstring << xvalues[id] << "\t\t" << out[id] << std::endl;
      }
      std::ofstream ofile;
      ofile.open("peaks_gen.dat");
      ofile << outstring.str();
      ofile.close();


      // 4. Compare calculated data
      double y25 = 1360.27;
      TS_ASSERT_DELTA(out[25], y25, 0.01);

      delete[] xvalues;
      delete[] out;

      return;
  }

  /*
   * Generate a set of powder diffraction data with 2 peaks (110 and 111)
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

  /*
   * Experiment data for HKL = (2, 1, 0)
   */
  void getMockData(std::vector<double>& xvalues, std::vector<double>& yvalues)
  {
    xvalues.clear();
    yvalues.clear();

    xvalues.push_back(54999.094);       yvalues.push_back(2.6283364);
    xvalues.push_back(55010.957);       yvalues.push_back(4.0346470);
    xvalues.push_back(55022.820);       yvalues.push_back(6.1934152);
    xvalues.push_back(55034.684);       yvalues.push_back(9.5072470);
    xvalues.push_back(55046.547);       yvalues.push_back(14.594171);
    xvalues.push_back(55058.410);       yvalues.push_back(22.402889);
    xvalues.push_back(55070.273);       yvalues.push_back(34.389721);
    xvalues.push_back(55082.137);       yvalues.push_back(52.790192);
    xvalues.push_back(55094.000);       yvalues.push_back(81.035973);
    xvalues.push_back(55105.863);       yvalues.push_back(124.39484);
    xvalues.push_back(55117.727);       yvalues.push_back(190.95044);
    xvalues.push_back(55129.590);       yvalues.push_back(293.01022);
    xvalues.push_back(55141.453);       yvalues.push_back(447.60229);
    xvalues.push_back(55153.320);       yvalues.push_back(664.84778);
    xvalues.push_back(55165.184);       yvalues.push_back(900.43817);
    xvalues.push_back(55177.047);       yvalues.push_back(1028.0037);
    xvalues.push_back(55188.910);       yvalues.push_back(965.38873);
    xvalues.push_back(55200.773);       yvalues.push_back(787.02441);
    xvalues.push_back(55212.637);       yvalues.push_back(603.50177);
    xvalues.push_back(55224.500);       yvalues.push_back(456.12289);
    xvalues.push_back(55236.363);       yvalues.push_back(344.13235);
    xvalues.push_back(55248.227);       yvalues.push_back(259.61121);
    xvalues.push_back(55260.090);       yvalues.push_back(195.84842);
    xvalues.push_back(55271.953);       yvalues.push_back(147.74631);
    xvalues.push_back(55283.816);       yvalues.push_back(111.45851);
    xvalues.push_back(55295.680);       yvalues.push_back(84.083313);
    xvalues.push_back(55307.543);       yvalues.push_back(63.431709);
    xvalues.push_back(55319.406);       yvalues.push_back(47.852318);
    xvalues.push_back(55331.270);       yvalues.push_back(36.099365);
    xvalues.push_back(55343.133);       yvalues.push_back(27.233042);
    xvalues.push_back(55354.996);       yvalues.push_back(20.544367);
    xvalues.push_back(55366.859);       yvalues.push_back(15.498488);
    xvalues.push_back(55378.727);       yvalues.push_back(11.690837);
    xvalues.push_back(55390.590);       yvalues.push_back(8.8194647);
    xvalues.push_back(55402.453);       yvalues.push_back(6.6533256);

    return;
  }

/*
 * Test simple function calculator: It is migrated from Bk2BkExpConvPV's unit test
 */
void Ntest_functionCalculator()
{
    // 1. Create peak
    Mantid::CurveFitting::ThermalNeutronBk2BkExpConvPV peak;

    peak.initialize();

    // 2. Set peak's parameters
    peak.setMillerIndex(2, 1, 0);

    peak.setParameter("Height", 1000.0);

    /*
    peak.tie("TOF_h", "55175.79");
    peak.tie("Alpha", "0.03613");
    peak.tie("Beta", "0.02376");
    peak.tie("Sigma2", "187.50514");
    peak.tie("Gamma", "0.0");
    */

    // 2. Set workspace
    std::vector<double> Xs;
    std::vector<double> Ys;
    getMockData(Xs, Ys);

    size_t ndata = Xs.size();

    double* out = new double[ndata];
    double* xvalues = new double[ndata];
    out = new double[ndata];
    xvalues = new double[ndata];

    peak.function1D(out, xvalues, ndata);

   return;
 }

};


#endif /* MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPVTEST_H_ */
