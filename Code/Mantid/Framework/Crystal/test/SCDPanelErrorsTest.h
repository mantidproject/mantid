/*
 * SCDPanelErrorsTest.h
 *
 *  Created on: Feb 28, 2012
 *      Author: ruth
 */

#ifndef SCDCALIBRATEPANELTEST_H_
#define SCDCALIBRATEPANELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IPeak.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IFunction.h"

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
    Crystal::SCDPanelErrors calib(Peakws, ComponentName, 14.0, 19.3, 8.6, 90., 105., 90., .12);
    calib.setAttribute("NGroups", IFunction::Attribute(1));
    calib.setAttribute("RotateCenters",IFunction::Attribute(0));
    calib.setAttribute("SampleOffsets",IFunction::Attribute(1));
    std::vector<std::string> banks;
    banks.push_back(std::string("bank26"));

    DataObjects::Workspace2D_sptr ws = Crystal::SCDPanelErrors::calcWorkspace(Peakws, banks, .12);
    calib.setWorkspace(ws);

    const int N = (int) ws->dataX(0).size();

    std::vector<double> out(N);
    std::vector<double> xVals(N);

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


    calib.setParameter("f0_detWidthScale", 1.0);

    calib.setParameter("f0_detHeightScale", 1.0);
    //calib.setParameter("Xoffset",1.0);
    //calib.setParameter("Yrot",90);

    calib.function1D(out.data(), xVals.data(), (size_t) N);
    //std::cout<<out[0]<<","<<out[4]<<","<<out[8]<<","<<out[10]<<std::endl;
    double d = .0001;
    TS_ASSERT_DELTA(out[0], -0.0038239, d);

    TS_ASSERT_DELTA(out[4], 0.00759182, d);

    TS_ASSERT_DELTA(out[8], 0.026758, d);

    TS_ASSERT_DELTA(out[10], 0.00883232, d);

    //-------------------------Test the derivative --------------------------------
  int nn=3;//sample offsets used

  boost::shared_ptr<Jacob> Jac(new Jacob(10+nn, N));
    calib.functionDeriv1D(Jac.get(), xVals.data(), (size_t) N);
    calib.functionDeriv1D(Jac.get(), xVals.data(), (size_t) N);


    size_t nData = N;
    std::vector<double> out0(N);// = new double[ N ];
    std::vector<double> out1(N);// = new double[ N ];
    std::vector<double> compRes(N);

    size_t params[20] =
    { 12,12,11,11,10,10,0,1,1, 2, 2,4,4,5, 5, 6, 6,7, 7, 7 };

    size_t indx[20] =
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1, 2, 3, 4, 5 };


    int x = 0;
    size_t prevParam = 999;

    for (size_t i = 0; i < 20; i++)
    {
      size_t param = params[i];

      if (param !=prevParam)
      {

        double sav = calib.getParameter(param);
        calib.setParameter(param, sav + .005);

        calib.function1D(out0.data(), xVals.data(), nData);
        calib.setParameter(param, sav - .005);

        calib.function1D(out1.data(), xVals.data(), nData);
        calib.setParameter(param, sav);
        for (int j = 0; j < (int) nData; j++)
          compRes[j] = (out0[j] - out1[j]) / .01;
        prevParam = param;
      }
      size_t k = indx[x];
      x++;


    TS_ASSERT_DELTA(Jac->get(k, param), compRes[k], .02);

    }
  }
  ;

};

#endif /* SCDCALIBRATEPANELTEST_H_ */
