// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
  This implements a resolution function for fitting a single mass in a compton
  scattering spectrum. The
  function has two domains defined by the VoigtEnergyCutOff attribute:

    - < VoigtEnergyCutOff: Voigt approximation to spectrum is used
    - >= VoigtEnergyCutOff: Gaussian approximation to spectrum is used.

  Both are normalized by the peak area.
*/
class MANTID_CURVEFITTING_DLL ComptonPeakProfile : public API::ParamFunction, public API::IFunction1D {
public:
  /// Default constructor required for factory
  ComptonPeakProfile();

private:
  std::string name() const override;

  /** @name Function evaluation */
  ///@{
  /// Calculate the function
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  /// Ensure the object is ready to be fitted
  void setUpForFit() override;
  /// Cache a copy of the workspace pointer and pull out the parameters
  void setWorkspace(std::shared_ptr<const API::Workspace> ws) override;
  ///@}

  void declareParameters() override;
  void declareAttributes() override;
  void setAttribute(const std::string &name, const Attribute &value) override;

  /// WorkspaceIndex attribute
  size_t m_wsIndex;
  /// Mass of peak
  double m_mass;
  /// Below this value a Voigt is used for profile approximation
  double m_voigtCutOff;

  /// Gaussian function for lower-energy peaks
  std::shared_ptr<API::IPeakFunction> m_gauss;
  /// Voigt function for higher-energy peaks
  std::shared_ptr<API::IPeakFunction> m_voigt;

  /// Final energy of analyser
  double m_efixed;
  /// Calculated value of lorentz width
  double m_hwhmLorentz;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
