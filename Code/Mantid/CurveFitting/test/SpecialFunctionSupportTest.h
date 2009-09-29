#ifndef SPECIALFUNCTIONSUPPORTTEST_H_
#define SPECIALFUNCTIONSUPPORTTEST_H_

#include <cxxtest/TestSuite.h>
#include <complex>
#include "MantidCurveFitting/SpecialFunctionSupport.h"

#include "MantidCurveFitting/Lorentzian1D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting::SpecialFunctionSupport;
using std::complex;

class SpecialFunctionSupportTest : public CxxTest::TestSuite
{
public:

  // reproduce number in Abramowitz and Stegun
  void testExpoentialIntegral()
  {
    complex<double> z;
    z = exponentialIntegral(complex<double>(0.1,0));
    TS_ASSERT_DELTA( z.real(), 2.0146,0.001);
    TS_ASSERT_DELTA( z.imag(), 0.0,0.001);
    z = exponentialIntegral(complex<double>(0.5,0));
    TS_ASSERT_DELTA( z.real(), 0.9229,0.001);
    TS_ASSERT_DELTA( z.imag(), 0.0,0.001);
    z = exponentialIntegral(complex<double>(1.0,0));
    TS_ASSERT_DELTA( z.real(), 0.5963,0.001);
    TS_ASSERT_DELTA( z.imag(), 0.0,0.001);
    z = exponentialIntegral(complex<double>(5.0,0));
    TS_ASSERT_DELTA( z.real(), 0.1704,0.001);
    TS_ASSERT_DELTA( z.imag(), 0.0,0.001);
    z = exponentialIntegral(complex<double>(8.5,0));
    TS_ASSERT_DELTA( z.real(), 0.1063,0.001);
    TS_ASSERT_DELTA( z.imag(), 0.0,0.001);
    z = exponentialIntegral(complex<double>(12.0,0));
    TS_ASSERT_DELTA( z.real(), 0.0773,0.001);
    TS_ASSERT_DELTA( z.imag(), 0.0,0.001);

    z = exponentialIntegral(complex<double>(-0.1,0));
    TS_ASSERT_DELTA( z.real(), 1.4684,0.001);
    TS_ASSERT_DELTA( z.imag(), -2.8426,0.001);
    z = exponentialIntegral(complex<double>(-0.5,0));
    TS_ASSERT_DELTA( z.real(), -0.2755,0.001);
    TS_ASSERT_DELTA( z.imag(), -1.9055,0.001);
    z = exponentialIntegral(complex<double>(-5.0,0));
    TS_ASSERT_DELTA( z.real(), -0.2708,0.001);
    TS_ASSERT_DELTA( z.imag(), -0.0212,0.001);
    z = exponentialIntegral(complex<double>(-12.0,0));
    TS_ASSERT_DELTA( z.real(), -0.0919,0.001);
    TS_ASSERT_DELTA( z.imag(), -0.0000,0.001);

    z = exponentialIntegral(complex<double>(0.1,1.0));
    TS_ASSERT_DELTA( z.real(), 0.3743,0.001);
    TS_ASSERT_DELTA( z.imag(), -0.5820,0.001);
    z = exponentialIntegral(complex<double>(-0.1,1.0));
    TS_ASSERT_DELTA( z.real(), 0.3059,0.001);
    TS_ASSERT_DELTA( z.imag(), -0.6572,0.001);
    z = exponentialIntegral(complex<double>(-0.1,-10.0));
    TS_ASSERT_DELTA( z.real(), 0.0085,0.001);
    TS_ASSERT_DELTA( z.imag(), 0.0984,0.001);
    z = exponentialIntegral(complex<double>(-0.1,10.0));
    TS_ASSERT_DELTA( z.real(), 0.0085,0.001);
    TS_ASSERT_DELTA( z.imag(), -0.0984,0.001);
  }

};

#endif /*SPECIALFUNCTIONSUPPORTTEST_H_*/
