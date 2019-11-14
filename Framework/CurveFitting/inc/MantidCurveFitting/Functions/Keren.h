// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_KEREN_H_
#define MANTID_CURVEFITTING_KEREN_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** Keren : Keren fitting function for muon scientists

  Generalization of the Abragam relaxation function to a longitudinal field
  See Phys Rev B, 50, 10039 (1994)
*/
class MANTID_CURVEFITTING_DLL Keren : public API::ParamFunction,
                                      public API::IFunction1D {

public:
  /// Name of function
  std::string name() const override { return "Keren"; }
  /// Category for function
  const std::string category() const override { return "MuonSpecific"; }
  /// Set active parameter
  void setActiveParameter(size_t i, double value) override;
  /// Get active parameter
  double activeParameter(size_t i) const override;

protected:
  /// Evaluate the function at the given values
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  /// Initialize parameters
  void init() override;
  /// Time-dependent muon polarization
  double polarization(const double delta, const double larmor,
                      const double fluct, const double time) const;
  /// Relaxation form
  double relaxation(const double delta, const double larmor, const double fluct,
                    const double time) const;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_KEREN_H_ */