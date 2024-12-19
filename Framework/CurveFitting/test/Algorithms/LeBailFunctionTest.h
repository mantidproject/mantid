// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/Algorithms/LeBailFunction.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/cow_ptr.h"

#include <fstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::API;

using namespace std;

class LeBailFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeBailFunctionTest *createSuite() { return new LeBailFunctionTest(); }
  static void destroySuite(LeBailFunctionTest *suite) { delete suite; }

  void test_init() {
    LeBailFunction function("ThermalNeutronBk2BkExpConvPVoigt");
    TS_ASSERT(function.isParameterValid());
  }

  //----------------------------------------------------------------------------------------------
  /** Test initialize profile number 9 (NeutronBk2BkExpConvPVoigt)
   */
  void test_initProfNo9() {
    LeBailFunction function("ThermalNeutronBk2BkExpConvPVoigt");
    TS_ASSERT(function.isParameterValid());
  }

  //----------------------------------------------------------------------------------------------
  /** Test whether background functions are supported
   */
  void test_addBckgroundFunctions() {
    LeBailFunction lebailfunction("NeutronBk2BkExpConvPVoigt");

    // Add peak parameters
    map<string, double> parammap{{"Dtt1", 29671.7500},
                                 {"Dtt2", 0.0},
                                 {"Zero", 0.0},

                                 {"Alph0", 4.026},
                                 {"Alph1", 7.362},
                                 {"Beta0", 3.489},
                                 {"Beta1", 19.535},

                                 {"Sig2", sqrt(11.380)},
                                 {"Sig1", sqrt(9.901)},
                                 {"Sig0", sqrt(17.370)},

                                 {"Gam0", 0.0},
                                 {"Gam1", 0.0},
                                 {"Gam2", 0.0},

                                 {"LatticeConstant", 4.156890}};

    lebailfunction.setProfileParameterValues(parammap);

    // Add background functions
    std::vector<double> parvalues{1.0, 1.0, 1.0, 1.0};
    std::vector<std::string> parnames{"A0", "A1", "A2", "A3"};

    // Chebyshev
    TS_ASSERT_THROWS_NOTHING(lebailfunction.addBackgroundFunction("Chebyshev", 3, parnames, parvalues, 5000., 10000.));

    // FullprofPolynomial
    parnames.emplace_back("Bkpos");
    parvalues.emplace_back(7000.);

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
   * (3) plot out the graph with decent heights for both peaks to compare with
   *the data qualitatively
   *
   * Source data:
   * ...../Tests/Peaks/Jason-Powgen/HR_10Hz/B_mods/pg10b1.irf, LB4917b1.hkl
   * ...../"/home/wzz/Mantid/mantid/Code/release/LB4917b1_unittest.dat"
   */
  void test_CalculateLeBailFunction() {
    LeBailFunction lebailfunction("ThermalNeutronBk2BkExpConvPVoigt");

    // Add peak parameters
    map<string, double> parammap

        {{"Dtt1", 29671.7500},
         {"Dtt2", 0.0},
         {"Dtt1t", 29671.750},
         {"Dtt2t", 0.30},

         {"Zero", 0.0},
         {"Zerot", 33.70},

         {"Alph0", 4.026},
         {"Alph1", 7.362},
         {"Beta0", 3.489},
         {"Beta1", 19.535},

         {"Alph0t", 60.683},
         {"Alph1t", 39.730},
         {"Beta0t", 96.864},
         {"Beta1t", 96.864},

         {"Sig2", sqrt(11.380)},
         {"Sig1", sqrt(9.901)},
         {"Sig0", sqrt(17.370)},

         {"Width", 1.0055},
         {"Tcross", 0.4700},

         {"Gam0", 0.0},
         {"Gam1", 0.0},
         {"Gam2", 0.0},

         {"LatticeConstant", 4.156890}};

    lebailfunction.setProfileParameterValues(parammap);

    // Add peaks
    vector<vector<int>> vechkl;
    vector<int> p111;
    p111.emplace_back(1);
    p111.emplace_back(1);
    p111.emplace_back(1);
    vechkl.emplace_back(p111);
    vector<int> p110;
    p110.emplace_back(1);
    p110.emplace_back(1);
    p110.emplace_back(0);
    vechkl.emplace_back(p110);
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
    lebailfunction.function(vecX, true, false);
    TS_ASSERT_THROWS_ANYTHING(lebailfunction.function(vecX, true, true));

    vector<string> vecbkgdparnames(2);
    vecbkgdparnames[0] = "A0";
    vecbkgdparnames[1] = "A1";
    vector<double> bkgdvec(2);
    bkgdvec[0] = 0.01;
    bkgdvec[1] = 0.;
    lebailfunction.addBackgroundFunction("Polynomial", 2, vecbkgdparnames, bkgdvec, vecX.front(), vecX.back());

    auto out = lebailfunction.function(vecX, true, true);

    double v1 = out[imax111];
    double v2 = out[imax110];
    TS_ASSERT_DELTA(v1, 1380.5173, 10.);
    TS_ASSERT_DELTA(v2, 667.17743, 5.);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test LeBailFunction on calculating overalapped peaks
   *  The test data are of reflection (932) and (852) @ TOF = 12721.91 and
   * 12790.13
   */
  void test_CalculateHeightsOfOverlappedPeaks() {
    LeBailFunction lebailfunction("ThermalNeutronBk2BkExpConvPVoigt");

    // Add peak parameters
    map<string, double> parammap

        {{"Dtt1", 29671.7500},
         {"Dtt2", 0.0},
         {"Dtt1t", 29671.750},
         {"Dtt2t", 0.30},

         {"Zero", 0.0},
         {"Zerot", 33.70},

         {"Alph0", 4.026},
         {"Alph1", 7.362},
         {"Beta0", 3.489},
         {"Beta1", 19.535},

         {"Alph0t", 60.683},
         {"Alph1t", 39.730},
         {"Beta0t", 96.864},
         {"Beta1t", 96.864},

         {"Sig2", sqrt(11.380)},
         {"Sig1", sqrt(9.901)},
         {"Sig0", sqrt(17.370)},

         {"Width", 1.0055},
         {"Tcross", 0.4700},

         {"Gam0", 0.0},
         {"Gam1", 0.0},
         {"Gam2", 0.0},

         {"LatticeConstant", 4.156890}};

    lebailfunction.setProfileParameterValues(parammap);

    // Add peaks
    int xp932[] = {9, 3, 2};
    std::vector<int> p932(xp932, xp932 + sizeof(xp932) / sizeof(int));
    int xp852[] = {8, 5, 2};
    std::vector<int> p852(xp852, xp852 + sizeof(xp852) / sizeof(int));
    std::vector<std::vector<int>> hkls;
    hkls.emplace_back(p932);
    hkls.emplace_back(p852);
    lebailfunction.addPeaks(hkls);

    // Prepare data
    MatrixWorkspace_sptr dataws = createDataWorkspace(2);
    const MantidVec &vecX = dataws->readX(0);
    const MantidVec &vecY = dataws->readY(0);
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
  /** Goal: Test function() of LeBailFunction of Fullprof No. 9 by plotting 2
   *adjacent peaks
   * Input
   * (1) Instrument geometry parameters Dtt1, Dtt1t, Zero, ... from .prf file;
   * (2) Base peak parameters Alph0, Alph1, ... from .prf file
   * (3) 2 d-space values from .hkl file
   * Validate
   * (1) alpha0, beta0, and etc. for both d_h
   * (2) Tof_h for both d_h
   * (3) plot out the graph with decent heights for both peaks to compare with
   *the data qualitatively
   *
   * Source data:
   * ...../Tests/Peaks/Jason-Powgen/HR_10Hz/B_mods/pg10b1.irf, LB4917b1.hkl
   * ...../"/home/wzz/Mantid/mantid/Code/release/LB4917b1_unittest.dat"
   */
  void test_calculateLeBailFunctionProf9() {
    LeBailFunction lebailfunction("NeutronBk2BkExpConvPVoigt");

    // Add peak parameters
    map<string, double> parammap

        {{"Dtt1", 16370.650},    {"Dtt2", 0.10},           {"Zero", 0.0},

         {"Alph0", 1.0},         {"Alph1", 0.0},           {"Beta0", 0.109036}, {"Beta1", 0.009834},

         {"Sig2", sqrt(91.127)}, {"Sig1", sqrt(1119.230)}, {"Sig0", sqrt(0.0)},

         {"Gam0", 0.0},          {"Gam1", 7.688},          {"Gam2", 0.0},       {"LatticeConstant", 5.431363}};

    lebailfunction.setProfileParameterValues(parammap);

    // Add peaks
    vector<vector<int>> vechkl;
    vector<int> p220(3, 0);
    p220[0] = 2;
    p220[1] = 2;
    vechkl.emplace_back(p220);

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
    vector<double> vecX, vecY, vecE;
    generateVulcanPeak220(vecX, vecY, vecE);

    // Calculate peak intensities
    vector<double> summedpeaksvalue(vecY.size(), 0.);
    lebailfunction.calculatePeaksIntensities(vecX, vecY, summedpeaksvalue);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a test data workspace
   */
  MatrixWorkspace_sptr createDataWorkspace(int option) {
    // Create vectors
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;

    switch (option) {
    case 1:
      cout << "Generating 2 separated peaks data; "
           << ".\n";
      generateData(vecX, vecY, vecE);

      break;

    case 2:
      cout << "Generating 2 overlapped peaks data; "
           << ".\n";
      generateTwinPeakData(vecX, vecY, vecE);

      break;

    default:
      throw runtime_error("Option is not supported.");
      break;
    }

    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, vecX.size(), vecY.size());

    for (size_t i = 0; i < vecX.size(); ++i)
      ws->dataX(0)[i] = vecX[i];
    for (size_t i = 0; i < vecY.size(); ++i) {
      ws->dataY(0)[i] = vecY[i];
      ws->dataE(0)[i] = vecE[i];
    }

    return ws;
  }

  void importDataFromColumnFile(const std::string &filename, std::vector<double> &vecX, std::vector<double> &vecY,
                                std::vector<double> &vecE) {
    std::ifstream ins;
    ins.open(filename.c_str());
    char line[256];
    // std::cout << "File " << filename << " isOpen = " << ins.is_open() <<
    // '\n';
    while (ins.getline(line, 256)) {
      if (line[0] != '#') {
        double x, y;
        std::stringstream ss;
        ss.str(line);
        ss >> x >> y;
        vecX.emplace_back(x);
        vecY.emplace_back(y);
        double e = 1.0;
        if (y > 1.0E-5)
          e = std::sqrt(y);
        vecE.emplace_back(e);
      }
    }
  }

  /** Generate a set of powder diffraction data with 2 peaks
   */
  void generateData(std::vector<double> &vecX, std::vector<double> &vecY, std::vector<double> &vecE) {

    vecX = std::vector<double>{
        70931.750000, 70943.609000, 70955.477000, 70967.336000, 70979.203000, 70991.063000, 71002.930000, 71014.789000,
        71026.656000, 71038.516000, 71050.383000, 71062.242000, 71074.109000, 71085.969000, 71097.836000, 71109.695000,
        71121.563000, 71133.430000, 71145.289000, 71157.156000, 71169.016000, 71180.883000, 71192.742000, 71204.609000,
        71216.469000, 71228.336000, 71240.195000, 71252.063000, 71263.922000, 71275.789000, 71287.648000, 71299.516000,
        71311.375000, 71323.242000, 71335.102000, 71346.969000, 71358.836000, 71370.695000, 71382.563000, 71394.422000,
        71406.289000, 71418.148000, 71430.016000, 71441.875000, 71453.742000, 71465.602000, 71477.469000, 71489.328000,
        71501.195000, 71513.055000, 71524.922000, 71536.781000, 71548.648000, 71560.508000, 71572.375000, 71584.242000,
        71596.102000, 71607.969000, 71619.828000, 86911.852000, 86923.719000, 86935.578000, 86947.445000, 86959.305000,
        86971.172000, 86983.039000, 86994.898000, 87006.766000, 87018.625000, 87030.492000, 87042.352000, 87054.219000,
        87066.078000, 87077.945000, 87089.805000, 87101.672000, 87113.531000, 87125.398000, 87137.258000, 87149.125000,
        87160.984000, 87172.852000, 87184.711000, 87196.578000, 87208.445000, 87220.305000, 87232.172000, 87244.031000,
        87255.898000, 87267.758000, 87279.625000, 87291.484000, 87303.352000, 87315.211000, 87327.078000, 87338.938000,
        87350.805000, 87362.664000, 87374.531000, 87386.391000, 87398.258000, 87410.117000, 87421.984000, 87433.844000,
        87445.711000, 87457.578000, 87469.438000, 87481.305000, 87493.164000, 87505.031000, 87516.891000, 87528.758000,
        87540.617000, 87552.484000, 87564.344000, 87576.211000, 87588.070000, 87599.938000, 87611.797000, 87623.664000,
        87635.523000, 87647.391000, 87659.250000, 87671.117000, 87682.984000, 87694.844000, 87706.711000};

    vecY = std::vector<double>{
        0.000000,    0.000000,    0.695623,    0.990163,    1.409745,    2.006657,   2.856977,   4.066674,
        5.789926,    8.241489,    11.733817,   16.702133,   23.779659,   33.848408,  48.191662,  68.596909,
        97.664757,   139.048890,  197.908080,  281.608030,  399.650210,  562.426700, 773.341920, 1015.281300,
        1238.361300, 1374.938000, 1380.517300, 1266.397800, 1086.214100, 894.758910, 723.461120, 581.045350,
        465.935880,  373.453830,  299.358000,  239.927200,  192.294970,  154.141530, 123.540130, 99.028404,
        79.368507,   63.620914,   50.990391,   40.873333,   32.758839,   26.259121,  21.045954,  16.870203,
        13.520998,   10.838282,   8.686581,    6.963067,    5.580704,    4.473431,   3.585330,   2.873542,
        2.303400,    1.846111,    0.000000,    0.286515,    0.391570,    0.535034,   0.731211,   0.999114,
        1.365452,    1.866113,    2.549823,    3.484748,    4.761496,    6.507361,   8.891540,   12.151738,
        16.603910,   22.691912,   31.005537,   42.372311,   57.886639,   79.062233,  107.820820, 146.586610,
        197.830060,  263.461850,  343.089660,  432.578460,  522.641240,  600.013730, 651.222600, 667.177430,
        646.900390,  597.388730,  530.125730,  456.838900,  386.052950,  322.584560, 267.962310, 222.048630,
        183.800430,  152.111010,  125.858200,  104.147070,  86.170067,   71.304932,  58.996807,  48.819309,
        40.392483,   33.420235,   27.654932,   22.881344,   18.934097,   15.665835,  12.963332,  10.725698,
        8.875416,    7.343407,    6.076601,    5.027703,    4.160378,    3.442244,   2.848425,   2.356751,
        1.950190,    1.613562,    1.335208,    1.104734,    0.914043,    0.756362,   0.000000};

    for (double y : vecY) {
      double e = 1.0;
      if (y > 1.0)
        e = sqrt(y);
      vecE.emplace_back(e);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate data (vectors) containg twin peak w/o background
   */
  void generateTwinPeakData(std::vector<double> &vecX, std::vector<double> &vecY, std::vector<double> &vecE) {
    // These data of reflection (932) and (852)
    vecX.emplace_back(12646.470);
    vecY.emplace_back(0.56916749);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12658.333);
    vecY.emplace_back(0.35570398);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12670.196);
    vecY.emplace_back(0.85166878);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12682.061);
    vecY.emplace_back(4.6110063);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12693.924);
    vecY.emplace_back(24.960907);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12705.787);
    vecY.emplace_back(135.08231);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12717.650);
    vecY.emplace_back(613.15887);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12729.514);
    vecY.emplace_back(587.66174);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12741.378);
    vecY.emplace_back(213.99724);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12753.241);
    vecY.emplace_back(85.320320);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12765.104);
    vecY.emplace_back(86.317253);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12776.968);
    vecY.emplace_back(334.30905);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12788.831);
    vecY.emplace_back(1171.0187);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12800.695);
    vecY.emplace_back(732.47943);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12812.559);
    vecY.emplace_back(258.37717);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12824.422);
    vecY.emplace_back(90.549515);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12836.285);
    vecY.emplace_back(31.733501);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12848.148);
    vecY.emplace_back(11.121155);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12860.013);
    vecY.emplace_back(3.9048645);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12871.876);
    vecY.emplace_back(4.15836312E-02);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12883.739);
    vecY.emplace_back(0.22341134);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12895.603);
    vecY.emplace_back(1.2002950);
    vecE.emplace_back(1000.0000);
    vecX.emplace_back(12907.466);
    vecY.emplace_back(6.4486742);
    vecE.emplace_back(1000.0000);

    return;
  }

  void generateVulcanPeak220(std::vector<double> &vecx, std::vector<double> &vecy, std::vector<double> &vece) {
    vecx.emplace_back(31019.30000);
    vecy.emplace_back(0.02624178);
    vece.emplace_back(0.00092672);
    vecx.emplace_back(31050.40000);
    vecy.emplace_back(0.02646138);
    vece.emplace_back(0.00093232);
    vecx.emplace_back(31081.40000);
    vecy.emplace_back(0.02809566);
    vece.emplace_back(0.00096305);
    vecx.emplace_back(31112.50000);
    vecy.emplace_back(0.02896440);
    vece.emplace_back(0.00097980);
    vecx.emplace_back(31143.60000);
    vecy.emplace_back(0.02861105);
    vece.emplace_back(0.00097545);
    vecx.emplace_back(31174.80000);
    vecy.emplace_back(0.03432836);
    vece.emplace_back(0.00107344);
    vecx.emplace_back(31205.90000);
    vecy.emplace_back(0.03941826);
    vece.emplace_back(0.00115486);
    vecx.emplace_back(31237.10000);
    vecy.emplace_back(0.05355697);
    vece.emplace_back(0.00135755);
    vecx.emplace_back(31268.40000);
    vecy.emplace_back(0.09889440);
    vece.emplace_back(0.00188719);
    vecx.emplace_back(31299.60000);
    vecy.emplace_back(0.20556772);
    vece.emplace_back(0.00285447);
    vecx.emplace_back(31330.90000);
    vecy.emplace_back(0.43901506);
    vece.emplace_back(0.00456425);
    vecx.emplace_back(31362.30000);
    vecy.emplace_back(0.81941730);
    vece.emplace_back(0.00702201);
    vecx.emplace_back(31393.60000);
    vecy.emplace_back(1.33883897);
    vece.emplace_back(0.01019324);
    vecx.emplace_back(31425.00000);
    vecy.emplace_back(1.74451085);
    vece.emplace_back(0.01262540);
    vecx.emplace_back(31456.50000);
    vecy.emplace_back(1.83429503);
    vece.emplace_back(0.01317582);
    vecx.emplace_back(31487.90000);
    vecy.emplace_back(1.53455479);
    vece.emplace_back(0.01141480);
    vecx.emplace_back(31519.40000);
    vecy.emplace_back(1.03117425);
    vece.emplace_back(0.00839135);
    vecx.emplace_back(31550.90000);
    vecy.emplace_back(0.52893114);
    vece.emplace_back(0.00522327);
    vecx.emplace_back(31582.50000);
    vecy.emplace_back(0.23198354);
    vece.emplace_back(0.00311024);
    vecx.emplace_back(31614.10000);
    vecy.emplace_back(0.10961397);
    vece.emplace_back(0.00203244);
    vecx.emplace_back(31645.70000);
    vecy.emplace_back(0.06396058);
    vece.emplace_back(0.00152266);
    vecx.emplace_back(31677.30000);
    vecy.emplace_back(0.04880334);
    vece.emplace_back(0.00132322);
    vecx.emplace_back(31709.00000);
    vecy.emplace_back(0.03836045);
    vece.emplace_back(0.00116918);
    vecx.emplace_back(31740.70000);
    vecy.emplace_back(0.03639256);
    vece.emplace_back(0.00113951);
    vecx.emplace_back(31772.50000);
    vecy.emplace_back(0.03248324);
    vece.emplace_back(0.00107658);
    vecx.emplace_back(31804.20000);
    vecy.emplace_back(0.03096179);
    vece.emplace_back(0.00105191);

    for (double &i : vecy)
      i -= 0.02295189;

    return;
  }
};
