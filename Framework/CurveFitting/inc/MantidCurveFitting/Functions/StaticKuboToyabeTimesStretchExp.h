// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXP_H_
#define MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXP_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
StaticKuboToyabeTimesStretchExp fitting function

Represents multiplication of two other fitting functions: StaticKuboToyabe and
Stretched Exponential Decay.

@author Lamar Moore
@date 13/11/2015
*/
class DLLExport StaticKuboToyabeTimesStretchExp : public API::ParamFunction,
                                                  public API::IFunction1D {
public:
  std::string name() const override {
    return "StaticKuboToyabeTimesStretchExp";
  }

  const std::string category() const override { return "Muon\\MuonGeneric"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXP_H_ */