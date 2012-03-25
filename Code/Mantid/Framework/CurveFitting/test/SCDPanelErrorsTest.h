/*
 * SCDPanelErrorsTest.h
 *
 *  Created on: Feb 28, 2012
 *      Author: ruth
 */

#ifndef SCDCALIBRATEPANELTEST_H_
#define SCDCALIBRATEPANELTEST_H_

#include <cxxtest/TestSuite.h>

//#include "MantidCurveFitting/SCDPanelErrors.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IPeak.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidCurveFitting/SCDPanelErrors.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/Jacobian.h"

using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace DataObjects;
using namespace Geometry;
using namespace CurveFitting;

class Jacob: public Jacobian
{
private:
  Matrix<double> M;

public:
  Jacob(int nparams, int npoints)
  {
    M = Matrix<double> (nparams, npoints);
  }

  virtual ~ Jacob()
  {

  }
  void set(size_t iY, size_t iP, double value)
  {
    M[iP][iY] = value;
  }

  double get(size_t iY, size_t iP)
  {
    return M[iP][iY];
  }

};

class SCDPanelErrorsTest: public CxxTest::TestSuite
{

public:

  void test_data()
  {
    FrameworkManager::Instance();
    boost::shared_ptr<Algorithm> alg = AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);

    alg->initialize();
    alg->setPropertyValue("Filename", "TOPAZ_3007.peaks");
    alg->setPropertyValue("OutputWorkspace", "TOPAZ_3007");

    alg->execute();

    PeaksWorkspace_sptr Peakws = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("TOPAZ_3007"));

    AnalysisDataService::Instance().remove("TOPAZ_3007");

    std::string ComponentName("bank26");
    CurveFitting::SCDPanelErrors calib(Peakws, ComponentName, 14.0, 19.3, 8.6, 90., 105., 90., .12);

    std::vector<std::string> banks;
    banks.push_back(std::string("bank26"));

    DataObjects::Workspace2D_sptr ws = SCDPanelErrors::calcWorkspace(Peakws, banks, .12);
    calib.setWorkspace(ws, false);

    const int N = (int) ws->dataX(0).size();

    double out[N];
    double xVals[N];

    MantidVec xdata = ws->dataX(0);

    for (size_t i = 0; i < xdata.size(); i++)
      xVals[i] = xdata[i];

    IPeak & peak0 = Peakws->getPeak(0);
    calib.setParameter("l0", peak0.getL1());

    Instrument_const_sptr instr = peak0.getInstrument();
    TS_ASSERT( instr);


    IComponent_const_sptr bank = instr->getComponentByName("bank26");
    TS_ASSERT( bank);


    boost::shared_ptr<const RectangularDetector> det = boost::dynamic_pointer_cast<
        const RectangularDetector>(bank);
    TS_ASSERT( det);


    calib.setParameter("detWidthScale", 1.0);

    calib.setParameter("detHeightScale", 1.0);
    //calib.setParameter("Xoffset",1.0);
    //calib.setParameter("Yrot",90);

    calib.functionMW(out, xVals, (size_t) N);

    double d = .00001;
    TS_ASSERT_DELTA(out[0], -0.00396681, d);

    TS_ASSERT_DELTA(out[4], 0.00734371, d);

    TS_ASSERT_DELTA(out[8], 0.0268435, d);

    TS_ASSERT_DELTA(out[10], 0.00858511, d);

    //-------------------------Test the derivative --------------------------------
    boost::shared_ptr<Jacob> Jac(new Jacob(10, N));
    calib.functionDerivMW(Jac.get(), xVals, (size_t) N);

    size_t nData = N;
    double * out0 = new double[nData];
    double * out1 = new double[nData];
    double compRes[N];

    int params[20] =
    { 0, 0,0,0,0, 1, 1,1,1,1, 2, 2, 2,2, 2,9, 9, 9, 9, 9 };

    int indx[20] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };


    int x = 0;
    int prevParam = -1;
    for (int i = 0; i < 20; i++)
    {
      int param = params[i];
      if (param !=prevParam)
      {
        double sav = calib.getParameter(param);
        calib.setParameter(param, sav + .005);

        calib.functionMW(out0, xVals, nData);
        calib.setParameter(param, sav - .005);

        calib.functionMW(out1, xVals, nData);
        calib.setParameter(param, sav);
        for (int j = 0; j < (int) nData; j++)
          compRes[j] = (out0[j] - out1[j]) / .01;
        prevParam = param;
      }
      int k = indx[x];
      x++;

      TS_ASSERT_DELTA(Jac->get(k, param), compRes[k], .002);

    }

    delete[] out0;
    delete[] out1;

  }
  ;

};

#endif /* SCDCALIBRATEPANELTEST_H_ */
