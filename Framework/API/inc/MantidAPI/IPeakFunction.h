// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionWithLocation.h"

namespace Mantid {
namespace API {
/** An interface to a peak function, which extend the interface of
    IFunctionWithLocation by adding methods to set and get peak width.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009
*/
class MANTID_API_DLL IPeakFunction : public IFunctionWithLocation {
public:
  /// Constructor
  IPeakFunction();

  void function(const FunctionDomain &domain, FunctionValues &values) const override;

  /// Returns the peak FWHM
  virtual double fwhm() const = 0;

  /// Sets the parameters such that FWHM = w
  virtual void setFwhm(const double w) = 0;

  /// Returns the integral intensity of the peak
  virtual double intensity() const;

  /// Sets the integral intensity of the peak
  virtual void setIntensity(const double newIntensity);

  /// General implementation of the method for all peaks.
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  /// General implementation of the method for all peaks.
  void functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) override;

  /// Get the interval on which the peak has all its values above a certain
  /// level
  virtual std::pair<double, double> getDomainInterval(double level = DEFAULT_SEARCH_LEVEL) const;

  /// Function evaluation method to be implemented in the inherited classes
  virtual void functionLocal(double *out, const double *xValues, const size_t nData) const = 0;
  /// Derivative evaluation method. Default is to calculate numerically
  virtual void functionDerivLocal(Jacobian *jacobian, const double *xValues, const size_t nData);

  /// Get name of parameter that is associated to centre.
  std::string getCentreParameterName() const;

  /// Get the name of the parameter that is changed when the
  /// fwhm is changed. By default this returns an empty string (unless
  /// overridden in the individual functions) as some functions change two
  /// params when the fwhm is set and others don't have a width (delta
  /// func).This is intended for the BackToBackExponential-based peaks where
  /// the width parameter (S) can be set in the intsurment parameter file and
  /// this needs to be checked when a peak is added.
  virtual std::string getWidthParameterName() const { return ""; }

  /// Fix a parameter or set up a tie such that value returned
  /// by intensity() is constant during fitting.
  /// @param isDefault :: If true fix intensity by default:
  ///    don't show it in ties
  virtual void fixIntensity(bool isDefault = false) {
    UNUSED_ARG(isDefault);
    throw std::runtime_error("Generic intensity fixing isn't implemented for this function.");
  }

  /// Free the intensity parameter.
  virtual void unfixIntensity() {
    throw std::runtime_error("Generic intensity fixing isn't implemented for this function.");
  }

private:
  /// Set new peak radius
  void setPeakRadius(int r) const;
  /// Defines the area around the centre where the peak values are to be
  /// calculated (in FWHM).
  mutable int m_peakRadius;
  /// The default level for searching a domain interval (getDomainInterval())
  static constexpr double DEFAULT_SEARCH_LEVEL = 1e-5;
};

using IPeakFunction_sptr = std::shared_ptr<IPeakFunction>;
using IPeakFunction_const_sptr = std::shared_ptr<const IPeakFunction>;

} // namespace API
} // namespace Mantid
