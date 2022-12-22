#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include <valarray>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

class MANTID_CURVEFITTING_DLL MeierV2 : public API::ParamFunction, public API::IFunction1D {
public:
  std::string name() const override { return "MeierV2"; }
  const std::string category() const override { return "Muon\\MuonSpecific"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void init() override;

private:
  void calculateAlphaArrays(std::valarray<double> &sinSQalpha, std::valarray<double> &cosSQalpha,
                            std::valarray<double> &sinSQ2alpha, std::valarray<double> &cosSQ2alpha,
                            std::valarray<double> &lamm, std::valarray<double> &lamp, const double &J,
                            const double &J2) const;

  void calculatePx(std::valarray<double> &Px, const std::valarray<double> &xValues,
                   const std::valarray<double> &sinSQalpha, const std::valarray<double> &cosSQalpha,
                   const std::valarray<double> &lamm, const std::valarray<double> &lamp, const double &J,
                   const double &J2) const;

  void calculatePz(std::valarray<double> &Pz, const std::valarray<double> &xValues,
                   const std::valarray<double> &sinSQ2alpha, const std::valarray<double> &cosSQ2alpha,
                   const std::valarray<double> &lamm, const std::valarray<double> &lamp, const double &J,
                   const double &J2) const;
};
} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
