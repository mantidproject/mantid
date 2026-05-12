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
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide Ikeda-Carpenter-pseudo-Voigt peak shape function interface to
IPeakFunction.

@author Anders Markvardsen, ISIS, RAL
@date 3/11/2009
*/
class MANTID_CURVEFITTING_DLL IkedaCarpenterPV : virtual public API::IPeakFunction, virtual public API::IFunctionMW {
public:
  /// overwrite IPeakFunction base class methods
  double centre() const override;
  double height() const override;
  double fwhm() const override;
  void setCentre(const double c) override;
  void setHeight(const double h) override;
  void setFwhm(const double w) override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "IkedaCarpenterPV"; }
  const std::string category() const override { return "Peak"; }

  /// Returns the integral intensity of the peak
  double intensity() const override;

  void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                          double endX) override;

protected:
  void functionLocal(double *out, const double *xValues, const size_t nData) const override;
  void functionDerivLocal(API::Jacobian *out, const double *xValues, const size_t nData) override;
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// container for storing wavelength values for each data point
  mutable std::vector<double> m_waveLength;

  /// calculate the const function
  void constFunction(double *out, const double *xValues, const int &nData) const;

  /// method for updating m_waveLength
  void calWavelengthAtEachDataPoint(const double *xValues, const size_t &nData) const;

  /// convert voigt params to pseudo voigt params
  void convertVoigtToPseudo(const double &voigtSigmaSq, const double &voigtGamma, double &H, double &eta) const;

  /// constrain all parameters to be non-negative
  void lowerConstraint0(const std::string &paramName);

  /// Re-apply any fixed tags from the IDF as fix() on the function. The base class
  /// IFunction::setMatrixWorkspace applies these only for parameters that
  /// are not already marked explicitlySet, so any earlier setParameter()
  /// call (from the user, from cross-peak inheritance, etc.) suppresses the
  /// re-application. Inherited or user-supplied values are preserved; only
  /// the fixed tag is re-applied so the moderator constants stay locked
  /// across successive setMatrixWorkspace invocations instead of being left
  /// active and drifting during the next fit.
  void refixIDFFixedParameters(const std::shared_ptr<const API::MatrixWorkspace> &workspace, size_t wi);
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
