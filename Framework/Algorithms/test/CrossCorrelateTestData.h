#pragma once

#include <cxxtest/TestSuite.h>
#include <initializer_list>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAlgorithms/ConvertToHistogram.h"
#include "MantidCurveFitting/Functions/Bk2BkExpConvPV.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::CurveFitting::Functions::Gaussian;

enum class PeakShapeEnum : int { B2BEXP, GAUSSIAN };

const double DIFC = 1434.66;
const double DIFA = -1.88;
const double T0 = 2.25;

namespace { // anonymous
/*
Code to generate parameters for b2bexpconvpv:

  import numpy as np

  diam_d = {"111": 2.05995, "220": 1.26146, "311": 1.07577}

  alp = 0.791431E-01
  beta0 = 0.580874E-01
  beta1 = 0.947427E-01
  sig0 = 0.0E+00
  sig1 = 0.157741E+03
  sig2 = 0.402182E+02
  gamma1 = 0.302644E+01

  print("===============================================")
  print("Back-to-back shape parameters for diamond peaks")
  print("===============================================")
  for key, item in diam_d.items():
    A = alp / item
    B = beta0 + beta1 / item**4
    S = np.sqrt(sig0 + sig1 * item**2 + sig2 * item**4)
    Gamma = gamma1 * item

    print("\n--------------------")
    print("({0:3s})".format(key))
    print("--------------------")
    print("A = {0:<10.5F}".format(A))
    print("B = {0:<10.5F}".format(B))
    print("S = {0:<10.5F}".format(S))
    print("Gamma = {0:<10.5F}".format(Gamma))
    print("===============================================")

  Result:

    ===============================================
    Back-to-back shape parameters for diamond peaks
    ===============================================

    --------------------
    (111)
    --------------------
    A = 0.03842
    B = 0.06335
    S = 37.33017
    Gamma = 6.23432
    ===============================================

    --------------------
    (220)
    --------------------
    A = 0.06274
    B = 0.09550
    S = 18.78430
    Gamma = 3.81773
    ===============================================

    --------------------
    (311)
    --------------------
    A = 0.07357
    B = 0.12883
    S = 15.37579
    Gamma = 3.25575
    ===============================================
*/

// these are intentionally missing "Intensity" and "X0" (center)
const std::string B2BEXP_SHAPE_111 = "Alpha=0.03842,Beta=0.06335,Sigma2=37.33017,Gamma=6.23432";
const std::string B2BEXP_SHAPE_220 = "Alpha=0.06274,Beta=0.09550,Sigma2=18.78430,Gamma=3.81773";
const std::string B2BEXP_SHAPE_311 = "Alpha=0.07357,Beta=0.12883,Sigma2=15.37579,Gamma=3.25575";

const double B2BEXP_POSITION_111 = 2.05995;
const double B2BEXP_POSITION_220 = 1.26146;
const double B2BEXP_POSITION_311 = 1.07577;

enum class PeakIndex : int { POS_111, POS_220, POS_311 };

std::shared_ptr<IFunction> createPeakFunction(const PeakShapeEnum shape, const PeakIndex peak_index,
                                              const double intensity, const double d) {
  std::stringstream peak;
  if (shape == PeakShapeEnum::B2BEXP) {
    peak << "name=Bk2BkExpConvPV,";
    if (peak_index == PeakIndex::POS_111) {
      peak << B2BEXP_SHAPE_111;
    } else if (peak_index == PeakIndex::POS_220) {
      peak << B2BEXP_SHAPE_220;
    } else if (peak_index == PeakIndex::POS_311) {
      peak << B2BEXP_SHAPE_311;
    } else {
      throw std::runtime_error("Developer forgot a peak index");
    }
    peak << ",Intensity=" << intensity << ",X0=" << d * DIFC + d * d * DIFA + T0;
  } else if (shape == PeakShapeEnum::GAUSSIAN) {
    // all Gaussians are hard coded to same arbitrary width
    peak << "name=Gaussian,Sigma=10,Height=" << intensity << ",PeakCentre=" << d * DIFC + d * d * DIFA + T0;
  }
  std::cout << peak.str() << std::endl;
  return FunctionFactory::Instance().createInitialized(peak.str());
}
} // anonymous namespace

class CrossCorrelateTestData : public CxxTest::TestSuite {
public:
  static CrossCorrelateTestData *createSuite() { return new CrossCorrelateTestData(); }
  static void destroySuite(CrossCorrelateTestData *suite) { delete suite; }

  static CompositeFunction_sptr createCompositeB2BExp(const PeakShapeEnum shape, const int spectrumIndex) {
    CompositeFunction_sptr function = std::make_shared<CompositeFunction>();

    // values for reference spectrum that may be changed for the others
    double shift_in_d{0.};
    double scale_in_d{1.};
    double horizontal_shift{0.};
    double height_111{100.};
    double height_220{200.};
    double height_311{300.};

    if (spectrumIndex == 0) { // expected value = 0.
      // reference spectrum
    } else if (spectrumIndex == 1) { // expected value = .1, or about 10 bins
      // additive shift in d-spacing from the reference
      // peak heights are unchanged
      shift_in_d = .1;
    } else if (spectrumIndex == 2) { // expected value = ???
                                     // multiplicative shift in d-spacing from the reference
                                     // peak heights are unchanged
      scale_in_d = 1.1;
    } else if (spectrumIndex == 3) { // expected value = 0.
      // shifted vertically by  a constant
      horizontal_shift = 40.;
    } else if (spectrumIndex == 4) { // expected value = 0.
      // scaled version of the reference - done via peak heights
      height_111 *= 2;
      height_220 *= 2;
      height_311 *= 2;
    } else {
      throw std::runtime_error("Logic for this spectrum index has not been written");
    }

    function->addFunction(
        createPeakFunction(shape, PeakIndex::POS_111, height_111, scale_in_d * B2BEXP_POSITION_111 + shift_in_d));
    function->addFunction(
        createPeakFunction(shape, PeakIndex::POS_220, height_220, scale_in_d * B2BEXP_POSITION_220 + shift_in_d));
    function->addFunction(
        createPeakFunction(shape, PeakIndex::POS_311, height_311, scale_in_d * B2BEXP_POSITION_311 + shift_in_d));
    if (horizontal_shift > 0.) {
      std::stringstream background;
      background << "name=FlatBackground,A0=" << horizontal_shift;
      function->addFunction(FunctionFactory::Instance().createInitialized(background.str()));
    }

    return function;
  }
  static std::vector<double> evaluateFunction(std::shared_ptr<IFunction> function, std::vector<double> &xValues) {
    std::vector<double> TOFvalues;
    std::transform(xValues.begin(), xValues.end(), std::back_inserter(TOFvalues),
                   [](double d) -> double { return d * DIFC + d * d * DIFA + T0; });

    FunctionDomain1DVector domain(TOFvalues);
    FunctionValues values(domain);
    function->function(domain, values);
    return values.toVector();
  }

  void testDataGenerator() {
    const auto result = createCompositeB2BExp(PeakShapeEnum::B2BEXP, 0);
    TS_ASSERT(result != nullptr);
  }
};
