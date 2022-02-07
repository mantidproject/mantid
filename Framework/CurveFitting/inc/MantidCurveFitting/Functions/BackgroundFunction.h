// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include <memory>

namespace mu {
class Parser;
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
A background function. Functions that are intended to be used as backgrounds
should inherit from this class to enable certain features. E.g. querying

@author Roman Tolchenov, Tessella plc
@date 26/04/2010
*/
class MANTID_CURVEFITTING_DLL BackgroundFunction : public API::IBackgroundFunction {
public:
  /// Returns the centre of the function, which may be something as simple as
  /// the centre of
  /// the fitting range in the case of a background function or peak shape
  /// function this
  /// return value reflects the centre of the peak
  double centre() const override { return 0.; }

  /// Returns the height of the function. For a background function this may
  /// return an average
  /// height of the background. For a peak function this return value is the
  /// height of the peak
  double height() const override { return 0.; }

  /// Sets the parameters such that centre == c
  void setCentre(const double c) override {
    (void)c; // Avoid compiler warning
  }

  /// Sets the parameters such that height == h
  void setHeight(const double h) override {
    (void)h; // Avoid compiler warning
  }

  void fit(const std::vector<double> &X, const std::vector<double> &Y) override;

  /// overwrite IFunction base class methods
  const std::string category() const override { return "Background"; }
};

using BackgroundFunction_sptr = std::shared_ptr<BackgroundFunction>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
