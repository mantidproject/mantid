#ifndef MANTID_CURVEFITTING_LEBAILFITTEST_H_
#define MANTID_CURVEFITTING_LEBAILFITTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "MantidCurveFitting/LeBailFit.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class LeBailFitTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeBailFitTest *createSuite() { return new LeBailFitTest(); }
  static void destroySuite( LeBailFitTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }

  /*
   * Goal: test function1D() of LeBailFit by plotting 2 adjacent peaks
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
   *
   */
  void test_Plot2Peaks()
  {
    LeBailFit fitalg;
    fitalg.initialize();

    // 1. Set up parameters
    fitalg.setParameter("Dtt1", 29671.7500);
    fitalg.setParameter("Dtt2", 0.0);
    fitalg.setParameter("Dtt1t", 29671.750);
    fitalg.setParameter("Dtt2t", 0.30);

    fitalg.setParameter("Zero", 0.0);
    fitalg.setParameter("Zerot", 33.70);

    fitalg.setParameter("Alph0", 4.026);
    fitalg.setParameter("Alph1", 7.362);
    fitalg.setParameter("Beta0", 3.489);
    fitalg.setParameter("Beta1", 19.535);

    fitalg.setParameter("Alph0t", 60.683);
    fitalg.setParameter("Alph1t", 39.730);
    fitalg.setParameter("Beta0t", 96.864);
    fitalg.setParameter("Beta1t", 96.864);

    fitalg.setParameter("Sig2",  11.380);
    fitalg.setParameter("Sig1",   9.901);
    fitalg.setParameter("Sig0",  17.370);

    fitalg.setParameter("Width", 1.0055);
    fitalg.setParameter("Tcross", 0.4700);

    fitalg.setParameter("Gam0", 0.0);
    fitalg.setParameter("Gam1", 0.0);
    fitalg.setParameter("Gam2", 0.0);

    double d1 = 2.399981; // 1 1 1
    double h1 = 1370.0/0.008;
    double d2 = 2.939365; // 1 1 0
    double h2 = 660.0/0.0064;
    fitalg.setPeak(d1, h1);
    fitalg.setPeak(d2, h2);

    // 2. Calculate
    std::vector<double> vecX;
    std::vector<double> vecY;
    std::vector<double> vecE;
    std::string filename("/home/wzz/Mantid/mantid/Code/release/LB4917b1_unittest.dat");
    importDataFromColumnFile(filename, vecX, vecY,  vecE);

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

    delete xvalues;
    delete out;

    return;
  }

  void importDataFromColumnFile(std::string filename, std::vector<double>& vecX, std::vector<double>& vecY, std::vector<double>& vecE)
  {
    std::ifstream ins;
    ins.open(filename.c_str());
    char line[256];
    std::cout << "File " << filename << " isOpen = " << ins.is_open() << std::endl;
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

};


#endif /* MANTID_CURVEFITTING_LEBAILFITTEST_H_ */
