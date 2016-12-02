/*
 * PeakHKLErrorsTest.h
 *
 *  Created on: Jan 27, 2013
 *      Author: ruth
 */

#ifndef PANELHKLERRORSTEST_H_
#define PANELHKLERRORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidAPI/Jacobian.h"
#include "MantidKernel/Matrix.h"
using Mantid::DataObjects::PeaksWorkspace_sptr;
using namespace Mantid;
using namespace Crystal;
using Mantid::API::IFunction;

class Jacob : public API::Jacobian {
private:
  Kernel::Matrix<double> M;

public:
  Jacob(int nparams, int npoints) {
    M = Kernel::Matrix<double>(nparams, npoints);
  }

  ~Jacob() override {}
  void set(size_t iY, size_t iP, double value) override { M[iP][iY] = value; }

  double get(size_t iY, size_t iP) override { return M[iP][iY]; }

  void zero() override { M.zeroMatrix(); }
};

class PeakHKLErrorsTest : public CxxTest::TestSuite {

public:
  void test_data() {
    LoadIsawPeaks alg;
    alg.initialize();
    alg.setProperty("Filename", "TOPAZ_5637_8.peaks");
    alg.setProperty("OutputWorkspace", "abcd");
    alg.execute();
    alg.setProperty("OutputWorkspace", "abcd");
    API::Workspace_sptr ows = alg.getProperty("OutputWorkspace");
    DataObjects::PeaksWorkspace_sptr peaks =
        boost::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ows);
    // std::cout<<"Peaks number="<<peaks->getNumberPeaks()<<'\n';

    LoadIsawUB loadUB;
    loadUB.initialize();
    loadUB.setProperty("InputWorkspace",
                       alg.getPropertyValue("OutputWorkspace"));
    loadUB.setProperty("Filename", "ls5637.mat");
    loadUB.execute();

    PeakHKLErrors peakErrs;
    peakErrs.setAttribute(std::string("PeakWorkspaceName"),
                          IFunction::Attribute("abcd"));
    peakErrs.setAttribute("OptRuns", IFunction::Attribute("/5638/"));
    peakErrs.initialize();
    std::string OptRuns[] = {"5638"};
    double chis[] = {135.0};
    double phis[] = {-.02};
    double omegas[] = {60};
    for (int i = 0; i < 1; i++) {
      std::string name = "chi" + OptRuns[i];
      peakErrs.setParameter(name, chis[i]);
      name = "phi" + OptRuns[i];
      peakErrs.setParameter(name, phis[i]);
      name = "omega" + OptRuns[i];
      peakErrs.setParameter(name, omegas[i]);
    }

    peakErrs.setParameter("SampleXOffset", 0.0);
    peakErrs.setParameter("SampleYOffset", 0.0);
    peakErrs.setParameter("SampleZOffset", 0.0);

    const int NPeaks(peaks->getNumberPeaks());
    std::vector<double> out(3 * NPeaks);
    std::vector<double> out1(3 * NPeaks);
    std::vector<double> xValues(3 * NPeaks);
    for (int i = 0; i < NPeaks; i++) {
      xValues[3 * i] = i;
      xValues[3 * i + 1] = i;
      xValues[3 * i + 2] = i;
    }

    peakErrs.function1D(out.data(), xValues.data(), (size_t)(3 * NPeaks));

    //       std::cout<<out[0]<<","<<out[4]<<","<<out[8]<<","<<out[12]<<","<<out[16]<<",";
    // std::cout<<'\n';

    TS_ASSERT_DELTA(-0.0074152, out[0], .01);
    TS_ASSERT_DELTA(-0.00969701, out[4], .01);
    TS_ASSERT_DELTA(0.0120299, out[8], .01);
    TS_ASSERT_DELTA(-0.0060874, out[12], .01);
    TS_ASSERT_DELTA(-0.0103673, out[16], .01);

    boost::shared_ptr<Jacob> Jac(
        new Jacob((int)peakErrs.nParams(), (int)(3 * NPeaks)));
    peakErrs.functionDeriv1D(Jac.get(), xValues.data(), (size_t)(3 * NPeaks));

    TS_ASSERT_DELTA(Jac->get(1, 0), -1.39557, .4);
    TS_ASSERT_DELTA(Jac->get(3, 1), 2.24071, .4);
    TS_ASSERT_DELTA(Jac->get(10, 2), -6.80222, .4);
    TS_ASSERT_DELTA(Jac->get(55, 3), .188203, .1);
    TS_ASSERT_DELTA(Jac->get(85, 3), .127, .1);
    TS_ASSERT_DELTA(Jac->get(235, 4), -.05, .1);
    TS_ASSERT_DELTA(Jac->get(110, 5), .0678, .1);
    TS_ASSERT_DELTA(Jac->get(100, 0), -1.4782, .4);
    TS_ASSERT_DELTA(Jac->get(200, 1), -8.24138, .4);
    TS_ASSERT_DELTA(Jac->get(160, 2), -12.7745, .1);
    TS_ASSERT_DELTA(Jac->get(80, 4), -.0943, .1);
  }
};

#endif /* PANELHKLERRORSTEST_H_ */
