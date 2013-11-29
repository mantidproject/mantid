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
  /** Test initialize profile number 9 (NeutronBk2BkExpConvPVoigt)
    */
  void test_initProfNo9()
  {
    LeBailFunction function("ThermalNeutronBk2BkExpConvPVoigt");
    TS_ASSERT(function.isParameterValid());
  }

  //----------------------------------------------------------------------------------------------
  /** Test whether background functions are supported
   */
  void test_addBckgroundFunctions()
  {
    LeBailFunction lebailfunction("NeutronBk2BkExpConvPVoigt");

    // Add peak parameters
    map<string, double> parammap;

    parammap.insert(make_pair("Dtt1", 29671.7500));
    parammap.insert(make_pair("Dtt2", 0.0));
    parammap.insert(make_pair("Zero", 0.0));

    parammap.insert(make_pair("Alph0", 4.026));
    parammap.insert(make_pair("Alph1", 7.362));
    parammap.insert(make_pair("Beta0", 3.489));
    parammap.insert(make_pair("Beta1", 19.535));


    parammap.insert(make_pair("Sig2",  sqrt(11.380)));
    parammap.insert(make_pair("Sig1",  sqrt(9.901)));
    parammap.insert(make_pair("Sig0",  sqrt(17.370)));

    parammap.insert(make_pair("Gam0", 0.0));
    parammap.insert(make_pair("Gam1", 0.0));
    parammap.insert(make_pair("Gam2", 0.0));

    parammap.insert(make_pair("LatticeConstant", 4.156890));

    lebailfunction.setProfileParameterValues(parammap);

    // Add background functions
    std::vector<double> parvalues;
    std::vector<std::string> parnames;
    parnames.push_back("A0");  parvalues.push_back(1.0);
    parnames.push_back("A1");  parvalues.push_back(1.0);
    parnames.push_back("A2");  parvalues.push_back(1.0);
    parnames.push_back("A3");  parvalues.push_back(1.0);

    // Chebyshev
    TS_ASSERT_THROWS_NOTHING(
          lebailfunction.addBackgroundFunction("Chebyshev", 3, parnames, parvalues, 5000., 10000.));

    // FullprofPolynomial
    parnames.push_back("Bkpos"); parvalues.push_back(7000.);

    LeBailFunction lebailfunction2("NeutronBk2BkExpConvPVoigt");
    TS_ASSERT_THROWS_ANYTHING(
          lebailfunction2.addBackgroundFunction("FullprofPolynomial", 4, parnames, parvalues, -1., -1.));
    TS_ASSERT_THROWS_NOTHING(
          lebailfunction2.addBackgroundFunction("FullprofPolynomial", 6, parnames, parvalues, -1., -1.));

    return;
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
    MatrixWorkspace_sptr testws = createDataWorkspace(1);
    const vector<double> vecX = testws->readX(0);
    const vector<double> vecY = testws->readY(0);

    size_t nData = vecX.size();
    vector<double> out(nData);

    // Calculate peak intensities
    vector<double> summedpeaksvalue(vecY.size(), 0.);
    lebailfunction.calculatePeaksIntensities(vecX, vecY, summedpeaksvalue);

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
    lebailfunction.function(out, vecX, true, false);
    TS_ASSERT_THROWS_ANYTHING(lebailfunction.function(out, vecX, true, true));

    /*
    map<string, double> bkgdparmap;
    bkgdparmap.insert(make_pair("A0", 0.001));
    bkgdparmap.insert(make_pair("A1", 0.));
    */
    vector<string> vecbkgdparnames(2);
    vecbkgdparnames[0] = "A0";
    vecbkgdparnames[1] = "A1";
    vector<double> bkgdvec(2);
    bkgdvec[0] = 0.01;
    bkgdvec[1] = 0.;
    lebailfunction.addBackgroundFunction("Polynomial", 2, vecbkgdparnames, bkgdvec, vecX.front(), vecX.back());

    lebailfunction.function(out, vecX, true, true);

    double v1 = out[imax111];
    double v2 = out[imax110];
    TS_ASSERT_DELTA(v1, 1380.5173, 10.);
    TS_ASSERT_DELTA(v2, 667.17743, 5.);

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Test LeBailFunction on calculating overalapped peaks
   *  The test data are of reflection (932) and (852) @ TOF = 12721.91 and 12790.13
   */
  void test_CalculateHeightsOfOverlappedPeaks()
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
    int xp932[] = {9, 3, 2};
    std::vector<int> p932(xp932, xp932+sizeof(xp932)/sizeof(int));
    int xp852[] = {8, 5, 2};
    std::vector<int> p852(xp852, xp852+sizeof(xp852)/sizeof(int));
    std::vector<std::vector<int> > hkls;
    hkls.push_back(p932);
    hkls.push_back(p852);
    lebailfunction.addPeaks(hkls);

    // Prepare data
    MatrixWorkspace_sptr dataws = createDataWorkspace(2);
    const MantidVec& vecX = dataws->readX(0);
    const MantidVec& vecY = dataws->readY(0);
    vector<double> vecoutput(vecY.size(), 0.);

    // Calculate peaks' intensities
    lebailfunction.calculatePeaksIntensities(vecX, vecY, vecoutput);

    // Check
    size_t ipeak1 = 6;
    size_t ipeak2 = 12;
    TS_ASSERT_DELTA(vecoutput[ipeak1], vecY[ipeak1], 5.0);
    TS_ASSERT_DELTA(vecoutput[ipeak2], vecY[ipeak2], 10.0);

    return;
  }



  //----------------------------------------------------------------------------------------------
  /** Goal: Test function() of LeBailFunction of Fullprof No. 9 by plotting 2 adjacent peaks
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
  void test_calculateLeBailFunctionProf9()
  {
    LeBailFunction lebailfunction("NeutronBk2BkExpConvPVoigt");

    // Add peak parameters
    map<string, double> parammap;

    parammap.insert(make_pair("Dtt1", 16370.650));
    parammap.insert(make_pair("Dtt2", 0.10));
    parammap.insert(make_pair("Zero", 0.0));

    parammap.insert(make_pair("Alph0", 1.0));
    parammap.insert(make_pair("Alph1", 0.0));
    parammap.insert(make_pair("Beta0", 0.109036));
    parammap.insert(make_pair("Beta1", 0.009834));

    parammap.insert(make_pair("Sig2",  sqrt(91.127)));
    parammap.insert(make_pair("Sig1",  sqrt(1119.230)));
    parammap.insert(make_pair("Sig0",  sqrt(0.0)));

    parammap.insert(make_pair("Gam0", 0.0));
    parammap.insert(make_pair("Gam1", 7.688));
    parammap.insert(make_pair("Gam2", 0.0));

    parammap.insert(make_pair("LatticeConstant", 5.431363));

    lebailfunction.setProfileParameterValues(parammap);

    // Add peaks
    vector<vector<int> > vechkl;
    vector<int> p220(3, 0);
    p220[0] = 2; p220[1] = 2;
    vechkl.push_back(p220);

    lebailfunction.addPeaks(vechkl);

    return;

    TS_ASSERT(lebailfunction.isParameterValid());

    // Test parameters of each peak
    double tof_h_d1 = lebailfunction.getPeakParameter(p220, "TOF_h");
    TS_ASSERT_DELTA(tof_h_d1, 31436.5488, 0.1);

    // double alpha_d1 = lebailfunction.getPeakParameter(p220, "Alpha");
    // double beta_d1 = lebailfunction.getPeakParameter(p220, "Beta");
    // double sigma2_d1 = lebailfunction.getPeakParameter(p220, "Sigma2");
    // double gamma_d1 = lebailfunction.getPeakParameter(p220, "Gamma");
    // TS_ASSERT_DELTA(alpha_d1, 0.02977, 0.0001);
    // TS_ASSERT_DELTA(beta_d1, 0.01865, 0.0001);
    // TS_ASSERT_DELTA(sigma2_d1, 451.94833, 0.1);
    // TS_ASSERT_DELTA(gamma_d1, 0.0, 0.01);

    // Test calculating peak

    // Generate data and set up output
    vector<double> vecX,  vecY, vecE;
    generateVulcanPeak220(vecX, vecY, vecE);

    // Calculate peak intensities
    vector<double> summedpeaksvalue(vecY.size(), 0.);
    lebailfunction.calculatePeaksIntensities(vecX, vecY, summedpeaksvalue);

    return;
  }




  //----------------------------------------------------------------------------------------------
  /** Create a test data workspace
    */
  MatrixWorkspace_sptr createDataWorkspace(int option)
  {
    // Create vectors
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;

    switch (option)
    {
      case 1:
        cout << "Generating 2 separated peaks data; " << ".\n";
        generateData(vecX, vecY, vecE);

        break;

      case 2:
        cout << "Generating 2 overlapped peaks data; " << ".\n";
        generateTwinPeakData(vecX, vecY, vecE);

        break;

      default:
        throw runtime_error("Option is not supported.");
        break;
    }

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

  //----------------------------------------------------------------------------------------------
  /** Generate data (vectors) containg twin peak w/o background
   */
  void generateTwinPeakData(std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
  {
    // These data of reflection (932) and (852)
    vecX.push_back(12646.470);    vecY.push_back(  0.56916749     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12658.333);    vecY.push_back(  0.35570398     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12670.196);    vecY.push_back(  0.85166878     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12682.061);    vecY.push_back(   4.6110063     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12693.924);    vecY.push_back(   24.960907     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12705.787);    vecY.push_back(   135.08231     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12717.650);    vecY.push_back(   613.15887     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12729.514);    vecY.push_back(   587.66174     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12741.378);    vecY.push_back(   213.99724     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12753.241);    vecY.push_back(   85.320320     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12765.104);    vecY.push_back(   86.317253     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12776.968);    vecY.push_back(   334.30905     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12788.831);    vecY.push_back(   1171.0187     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12800.695);    vecY.push_back(   732.47943     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12812.559);    vecY.push_back(   258.37717     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12824.422);    vecY.push_back(   90.549515     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12836.285);    vecY.push_back(   31.733501     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12848.148);    vecY.push_back(   11.121155     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12860.013);    vecY.push_back(   3.9048645     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12871.876);    vecY.push_back(  4.15836312E-02 );  vecE.push_back(  1000.0000 );
    vecX.push_back(12883.739);    vecY.push_back(  0.22341134     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12895.603);    vecY.push_back(   1.2002950     );  vecE.push_back(  1000.0000 );
    vecX.push_back(12907.466);    vecY.push_back(   6.4486742     );  vecE.push_back(  1000.0000 );

    return;
  }

  void generateVulcanPeak220(std::vector<double>& vecx, std::vector<double>& vecy, std::vector<double>& vece)
  {
    vecx.push_back(31019.30000);        vecy.push_back(0.02624178);         vece.push_back(0.00092672);
    vecx.push_back(31050.40000);        vecy.push_back(0.02646138);         vece.push_back(0.00093232);
    vecx.push_back(31081.40000);        vecy.push_back(0.02809566);         vece.push_back(0.00096305);
    vecx.push_back(31112.50000);        vecy.push_back(0.02896440);         vece.push_back(0.00097980);
    vecx.push_back(31143.60000);        vecy.push_back(0.02861105);         vece.push_back(0.00097545);
    vecx.push_back(31174.80000);        vecy.push_back(0.03432836);         vece.push_back(0.00107344);
    vecx.push_back(31205.90000);        vecy.push_back(0.03941826);         vece.push_back(0.00115486);
    vecx.push_back(31237.10000);        vecy.push_back(0.05355697);         vece.push_back(0.00135755);
    vecx.push_back(31268.40000);        vecy.push_back(0.09889440);         vece.push_back(0.00188719);
    vecx.push_back(31299.60000);        vecy.push_back(0.20556772);         vece.push_back(0.00285447);
    vecx.push_back(31330.90000);        vecy.push_back(0.43901506);         vece.push_back(0.00456425);
    vecx.push_back(31362.30000);        vecy.push_back(0.81941730);         vece.push_back(0.00702201);
    vecx.push_back(31393.60000);        vecy.push_back(1.33883897);         vece.push_back(0.01019324);
    vecx.push_back(31425.00000);        vecy.push_back(1.74451085);         vece.push_back(0.01262540);
    vecx.push_back(31456.50000);        vecy.push_back(1.83429503);         vece.push_back(0.01317582);
    vecx.push_back(31487.90000);        vecy.push_back(1.53455479);         vece.push_back(0.01141480);
    vecx.push_back(31519.40000);        vecy.push_back(1.03117425);         vece.push_back(0.00839135);
    vecx.push_back(31550.90000);        vecy.push_back(0.52893114);         vece.push_back(0.00522327);
    vecx.push_back(31582.50000);        vecy.push_back(0.23198354);         vece.push_back(0.00311024);
    vecx.push_back(31614.10000);        vecy.push_back(0.10961397);         vece.push_back(0.00203244);
    vecx.push_back(31645.70000);        vecy.push_back(0.06396058);         vece.push_back(0.00152266);
    vecx.push_back(31677.30000);        vecy.push_back(0.04880334);         vece.push_back(0.00132322);
    vecx.push_back(31709.00000);        vecy.push_back(0.03836045);         vece.push_back(0.00116918);
    vecx.push_back(31740.70000);        vecy.push_back(0.03639256);         vece.push_back(0.00113951);
    vecx.push_back(31772.50000);        vecy.push_back(0.03248324);         vece.push_back(0.00107658);
    vecx.push_back(31804.20000);        vecy.push_back(0.03096179);         vece.push_back(0.00105191);

    for (size_t i = 0; i < vecy.size(); ++i)
      vecy[i] -= 0.02295189;

    return;
  }


};


#endif /* MANTID_CURVEFITTING_LEBAILFITTEST_H_ */
