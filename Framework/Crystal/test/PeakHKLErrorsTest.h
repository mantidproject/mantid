// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * PeakHKLErrorsTest.h
 *
 *  Created on: Jan 27, 2013
 *      Author: ruth
 */

#pragma once

#include "MantidAPI/IFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/Matrix.h"
#include <cxxtest/TestSuite.h>
using Mantid::DataObjects::PeaksWorkspace_sptr;
using namespace Mantid;
using namespace Crystal;
using Mantid::API::IFunction;

class Jacob : public API::Jacobian {
private:
  Kernel::Matrix<double> M;

public:
  Jacob(int nparams, int npoints) { M = Kernel::Matrix<double>(nparams, npoints); }

  ~Jacob() override = default;
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
    DataObjects::PeaksWorkspace_sptr peaks = std::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ows);
    // std::cout<<"Peaks number="<<peaks->getNumberPeaks()<<'\n';

    LoadIsawUB loadUB;
    loadUB.initialize();
    loadUB.setProperty("InputWorkspace", alg.getPropertyValue("OutputWorkspace"));
    loadUB.setProperty("Filename", "ls5637.mat");
    loadUB.execute();

    PeakHKLErrors peakErrs;
    peakErrs.setAttribute(std::string("PeakWorkspaceName"), IFunction::Attribute("abcd"));
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

    TS_ASSERT_DELTA(-0.0074152019155562243, out[0], 1e-10);
    TS_ASSERT_DELTA(-0.0096970128407134482, out[4], 1e-10);
    TS_ASSERT_DELTA(0.012029918447158394, out[8], 1e-10);
    TS_ASSERT_DELTA(-0.0069087373756087089, out[12], 1e-10);
    TS_ASSERT_DELTA(-0.0103672693484409, out[16], 1e-10);

    std::shared_ptr<Jacob> Jac(new Jacob((int)peakErrs.nParams(), (int)(3 * NPeaks)));
    peakErrs.functionDeriv1D(Jac.get(), xValues.data(), (size_t)(3 * NPeaks));

    TS_ASSERT_DELTA(Jac->get(1, 0), -1.3955661103280184, 1e-10);
    TS_ASSERT_DELTA(Jac->get(3, 1), 2.2407106656625229, 1e-10);
    TS_ASSERT_DELTA(Jac->get(10, 2), -6.8022177639755403, 1e-10);
    TS_ASSERT_DELTA(Jac->get(55, 3), 0.18820267687715025, 1e-10);
    TS_ASSERT_DELTA(Jac->get(85, 3), 0.12735233498609333, 1e-10);
    TS_ASSERT_DELTA(Jac->get(235, 4), -0.050041366550665503, 1e-10);
    TS_ASSERT_DELTA(Jac->get(110, 5), 0.067876666198063804, 1e-10);
    TS_ASSERT_DELTA(Jac->get(100, 0), -1.4781950411241356, 1e-10);
    TS_ASSERT_DELTA(Jac->get(200, 1), -8.2413846551922543, 1e-10);
    TS_ASSERT_DELTA(Jac->get(160, 2), -12.774495036711505, 1e-10);
    TS_ASSERT_DELTA(Jac->get(80, 4), -0.094348423161765685, 1e-10);
  }
};
