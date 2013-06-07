#ifndef MANTID_CURVEFITTING_LEBAILFITTEST_H_
#define MANTID_CURVEFITTING_LEBAILFITTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "MantidCurveFitting/LeBailFunction.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

using namespace std;

class LeBailFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeBailFunctionTest *createSuite() { return new LeBailFunctionTest(); }
  static void destroySuite( LeBailFunctionTest *suite ) { delete suite; }

  void test_init()
  {
    LeBailFunction function("ThermalNeutronBk2BkExpConvPVoigt");
    TS_ASSERT(function.isParameterValid());
  }

  //----------------------------------------------------------------------------------------------
  /** Goal: Test function() of LeBailFunction by plotting 2 adjacent peaks
   * Input
   * (1) Instrument geometry parameters Dtt1, Dtt1t, Zero, ... from .prf file;
   * (2) Base peak parameters Alph0, Alph1, ... from .prf file
   * (3) 2 d-space values from .hkl file
   * Validate
   * (1) alpha0, beta0, and etc. for both d_h
   * (2) Tof_h for both d_h
   * (3) plot out the graph with decent heights for both peaks to compare with the data qualitatively
   *
   * Source data:
   * ...../Tests/Peaks/Jason-Powgen/HR_10Hz/B_mods/pg10b1.irf, LB4917b1.hkl
   * ...../"/home/wzz/Mantid/mantid/Code/release/LB4917b1_unittest.dat"
   */
  void test_CalculateLeBailFunction()
  {
    LeBailFunction lebailfunction("ThermalNeutronBk2BkExpConvPVoigt");

    // Add peak parameters
    map<string, double> parammap;

    parammap.insert(make_pair("Dtt1", 29671.7500));
    parammap.insert(make_pair("Dtt2", 0.0));
    parammap.insert(make_pair("Dtt1t", 29671.750));
    parammap.insert(make_pair("Dtt2t", 0.30));

    parammap.insert(make_pair("Zero", 0.0));
    parammap.insert(make_pair("Zerot", 33.70));

    parammap.insert(make_pair("Alph0", 4.026));
    parammap.insert(make_pair("Alph1", 7.362));
    parammap.insert(make_pair("Beta0", 3.489));
    parammap.insert(make_pair("Beta1", 19.535));

    parammap.insert(make_pair("Alph0t", 60.683));
    parammap.insert(make_pair("Alph1t", 39.730));
    parammap.insert(make_pair("Beta0t", 96.864));
    parammap.insert(make_pair("Beta1t", 96.864));

    parammap.insert(make_pair("Sig2",  sqrt(11.380)));
    parammap.insert(make_pair("Sig1",  sqrt(9.901)));
    parammap.insert(make_pair("Sig0",  sqrt(17.370)));

    parammap.insert(make_pair("Width", 1.0055));
    parammap.insert(make_pair("Tcross", 0.4700));

    parammap.insert(make_pair("Gam0", 0.0));
    parammap.insert(make_pair("Gam1", 0.0));
    parammap.insert(make_pair("Gam2", 0.0));

    parammap.insert(make_pair("LatticeConstant", 4.156890));

    lebailfunction.setProfileParameterValues(parammap);

    // Add peaks
    vector<vector<int> > vechkl;
    vector<int> p111;
    p111.push_back(1); p111.push_back(1); p111.push_back(1);
    vechkl.push_back(p111);
    vector<int> p110;
    p110.push_back(1); p110.push_back(1); p110.push_back(0);
    vechkl.push_back(p110);
    lebailfunction.addPeaks(vechkl);

    TS_ASSERT(lebailfunction.isParameterValid());

    // Test parameters of each peak
    double tof_h_d1 = lebailfunction.getPeakParameter(p111, "TOF_h");
    double alpha_d1 = lebailfunction.getPeakParameter(p111, "Alpha");
    double beta_d1 = lebailfunction.getPeakParameter(p111, "Beta");
    double sigma2_d1 = lebailfunction.getPeakParameter(p111, "Sigma2");
    double gamma_d1 = lebailfunction.getPeakParameter(p111, "Gamma");
    TS_ASSERT_DELTA(tof_h_d1, 71229.45, 0.1);
    TS_ASSERT_DELTA(alpha_d1, 0.02977, 0.0001);
    TS_ASSERT_DELTA(beta_d1, 0.01865, 0.0001);
    TS_ASSERT_DELTA(sigma2_d1, 451.94833, 0.1);
    TS_ASSERT_DELTA(gamma_d1, 0.0, 0.01);

    double tof_h_d2 = lebailfunction.getPeakParameter(p110, "TOF_h");
    double alpha_d2 = lebailfunction.getPeakParameter(p110, "Alpha");
    double beta_d2 = lebailfunction.getPeakParameter(p110, "Beta");
    double sigma2_d2 = lebailfunction.getPeakParameter(p110, "Sigma2");
    double gamma_d2 = lebailfunction.getPeakParameter(p110, "Gamma");
    TS_ASSERT_DELTA(tof_h_d2, 87235.37, 0.1);
    TS_ASSERT_DELTA(alpha_d2, 0.02632, 0.0001);
    TS_ASSERT_DELTA(beta_d2, 0.01597, 0.0001);
    TS_ASSERT_DELTA(sigma2_d2, 952.39972, 0.1);
    TS_ASSERT_DELTA(gamma_d2, 0.0, 0.01);

    // Calculate peak
    MatrixWorkspace_sptr testws = createDataWorkspace();
    const vector<double> vecX = testws->readX(0);
    const vector<double> vecY = testws->readY(0);

    size_t nData = vecX.size();
    vector<double> out(nData);

    // Calculate peak intensities
    vector<double> summedpeaksvalue(vecY.size(), 0.);
    lebailfunction.calculatePeaksIntensities(vecX, vecY, true, summedpeaksvalue);

    // IPowderDiffPeakFunction_sptr peak111 = lebailfunction.getPeak(0);
    // IPowderDiffPeakFunction_sptr peak110 = lebailfunction.getPeak(1);
    double height111 = lebailfunction.getPeakParameter(p111, "Height");
    double height110 = lebailfunction.getPeakParameter(p110, "Height");
    size_t imax111, imax110;
    double max111 = lebailfunction.getPeakMaximumValue(p111, vecX, imax111);
    double max110 = lebailfunction.getPeakMaximumValue(p110, vecX, imax110);
    cout << "Peak(111): height = " << height111 << ", Max = " << max111 << " @ TOF = " << vecX[imax111] << ".\n";
    cout << "Peak(110): height = " << height110 << ", Max = " << max110 << " @ TOF = " << vecX[imax110] << ".\n";

    TS_ASSERT_DELTA(max111, 1380.5173, 10.);
    TS_ASSERT_DELTA(max110, 667.17743, 5.);
    TS_ASSERT_DELTA(vecX[imax111], 71240.195, 0.01);
    TS_ASSERT_DELTA(vecX[imax110], 87244.031, 0.01);
    cout << "Max value of peak 110 is at TOF = " << vecX[imax111] << " as the " << imax111 << "-th points.\n";

    // Calculate diffraction patters
    lebailfunction.function(out, vecX, false);
    TS_ASSERT_THROWS_ANYTHING(lebailfunction.function(out, vecX, true));

    /*
    map<string, double> bkgdparmap;
    bkgdparmap.insert(make_pair("A0", 0.001));
    bkgdparmap.insert(make_pair("A1", 0.));
    */
    vector<double> bkgdvec(2);
    bkgdvec[0] = 0.01;
    bkgdvec[1] = 0.;
    lebailfunction.addBackgroundFunction("Polynomial", bkgdvec);

    lebailfunction.function(out, vecX, true);

    double v1 = out[imax111];
    double v2 = out[imax110];
    TS_ASSERT_DELTA(v1, 1380.5173, 10.);
    TS_ASSERT_DELTA(v2, 667.17743, 5.);

 #if 0

    // TS_ASSERT_EQUALS(fitalg.asString(),
    // "name=LeBailFunction,Dtt1=1,Dtt2=1,Dtt1t=1,Dtt2t=1,Zero=0,Zerot=0,Width=1,Tcross=1,Alph0=1.6,Alph1=1.5,Beta0=1.6,Beta1=1.5,Alph0t=1.6,Alph1t=1.5,Beta0t=1.6,Beta1t=1.5,Sig0=1,Sig1=1,Sig2=1,Gam0=0,Gam1=0,Gam2=0");



    fitalg.setParameter("LatticeConstant", 4.156890);

    // double d1 = 2.399981; // 1 1 1
    double h1 = 1370.0/0.008;
    // double d2 = 2.939365; // 1 1 0
    double h2 = 660.0/0.0064;

    std::vector<int> p111;
    p111.push_back(1); p111.push_back(1); p111.push_back(1);
    std::vector<int> p110;
    p110.push_back(1); p110.push_back(1); p110.push_back(0);
    std::vector<std::vector<int> > peaks;
    peaks.push_back(p111); peaks.push_back(p110);

    std::vector<double> peakheights;
    peakheights.push_back(h1); peakheights.push_back(h2);

    fitalg.addPeaks(peaks, peakheights);

    // 2. Calculate
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;
    // std::string filename("/home/wzz/Mantid/mantid/Code/release/LB4917b1_unittest.dat");
    // importDataFromColumnFile(filename, vecX, vecY,  vecE);
    generateData(vecX, vecY, vecE);

    size_t nData = vecX.size();
    double* xvalues = new double[nData];
    double* out = new double[nData];
    for (size_t i = 0; i < nData; ++i)
    {
      xvalues[i] = vecX[i];
      out[i] = 0.0;
    }

    fitalg.calPeaks(out, xvalues, nData);


    std::stringstream outstring;
    for (size_t id = 0; id < nData; ++id)
    {
      outstring << xvalues[id] << "\t\t" << out[id] << std::endl;
    }
    std::ofstream ofile;
    ofile.open("peaks_gen.dat");
    ofile << outstring.str();
    ofile.close();


    // 3. Evaluate
    double tof_h_d1 = fitalg.getPeakParameter(0, "TOF_h");
    double alpha_d1 = fitalg.getPeakParameter(0, "Alpha");
    double beta_d1 = fitalg.getPeakParameter(0, "Beta");
    double sigma2_d1 = fitalg.getPeakParameter(0, "Sigma2");
    double gamma_d1 = fitalg.getPeakParameter(0, "Gamma");
    TS_ASSERT_DELTA(tof_h_d1, 71229.45, 0.1);
    TS_ASSERT_DELTA(alpha_d1, 0.02977, 0.0001);
    TS_ASSERT_DELTA(beta_d1, 0.01865, 0.0001);
    TS_ASSERT_DELTA(sigma2_d1, 451.94833, 0.1);
    TS_ASSERT_DELTA(gamma_d1, 0.0, 0.01);

    double tof_h_d2 = fitalg.getPeakParameter(1, "TOF_h");
    double alpha_d2 = fitalg.getPeakParameter(1, "Alpha");
    double beta_d2 = fitalg.getPeakParameter(1, "Beta");
    double sigma2_d2 = fitalg.getPeakParameter(1, "Sigma2");
    double gamma_d2 = fitalg.getPeakParameter(1, "Gamma");
    TS_ASSERT_DELTA(tof_h_d2, 87235.37, 0.1);
    TS_ASSERT_DELTA(alpha_d2, 0.02632, 0.0001);
    TS_ASSERT_DELTA(beta_d2, 0.01597, 0.0001);
    TS_ASSERT_DELTA(sigma2_d2, 952.39972, 0.1);
    TS_ASSERT_DELTA(gamma_d2, 0.0, 0.01);

    // 4. Calcualte data
    double y25 = 1360.27;
    double y59 = 0.285529;
    double y86 = 648.998;

    TS_ASSERT_DELTA(out[25], y25, 0.01);
    TS_ASSERT_DELTA(out[59], y59, 0.0001);
    TS_ASSERT_DELTA(out[86], y86, 0.001);

    delete[] xvalues;
    delete[] out;
#endif

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a test data workspace
    */
  MatrixWorkspace_sptr createDataWorkspace()
  {
    // Create vectors
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;
    generateData(vecX, vecY, vecE);

    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create(
          "Workspace2D", 1, vecX.size(), vecY.size());

    for (size_t i = 0; i < vecX.size(); ++i)
      ws->dataX(0)[i] = vecX[i];
    for (size_t i = 0; i < vecY.size(); ++i)
    {
      ws->dataY(0)[i] = vecY[i];
      ws->dataE(0)[i] = vecE[i];
    }

    return ws;
  }


  void importDataFromColumnFile(std::string filename, std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
  {
    std::ifstream ins;
    ins.open(filename.c_str());
    char line[256];
    // std::cout << "File " << filename << " isOpen = " << ins.is_open() << std::endl;
    while(ins.getline(line, 256))
    {
      if (line[0] != '#')
      {
        double x, y;
        std::stringstream ss;
        std::string dataline(line);
        ss.str(line);
        ss >> x >> y;
        vecX.push_back(x);
        vecY.push_back(y);
        double e = 1.0;
        if (y > 1.0E-5)
          e = std::sqrt(y);
        vecE.push_back(e);
      }
    }
  }

  /** Generate a set of powder diffraction data with 2 peaks
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


};


#endif /* MANTID_CURVEFITTING_LEBAILFITTEST_H_ */
