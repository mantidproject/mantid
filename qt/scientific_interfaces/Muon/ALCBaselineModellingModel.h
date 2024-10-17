// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidKernel/System.h"

#include "IALCBaselineModellingModel.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {
/** ALCBaselineModellingModel : Concrete ALC Baseline Modelling step model
  implementation.
*/
class MANTIDQT_MUONINTERFACE_DLL ALCBaselineModellingModel : public IALCBaselineModellingModel {
public:
  Mantid::API::MatrixWorkspace_sptr data() const override;

  void fit(Mantid::API::IFunction_const_sptr function, const std::vector<Section> &sections) override;

  Mantid::API::IFunction_const_sptr fittedFunction() const override { return m_fittedFunction; }

  Mantid::API::MatrixWorkspace_sptr correctedData() const override;

  Mantid::API::MatrixWorkspace_sptr baselineData(Mantid::API::IFunction_const_sptr function,
                                                 const std::vector<double> &xValues) const override;

  Mantid::API::ITableWorkspace_sptr parameterTable() const { return m_parameterTable; }

  const std::vector<Section> &sections() const { return m_sections; }

  Mantid::API::MatrixWorkspace_sptr exportWorkspace() override;

  void setData(Mantid::API::MatrixWorkspace_sptr data) override;

  void setCorrectedData(Mantid::API::MatrixWorkspace_sptr data) override;

  Mantid::API::ITableWorkspace_sptr exportSections() override;

  Mantid::API::ITableWorkspace_sptr exportModel() override;

private:
  /// Data used for fitting
  Mantid::API::MatrixWorkspace_sptr m_data;

  /// Result function of the last fit
  Mantid::API::IFunction_const_sptr m_fittedFunction;

  /// Fit table containing parameters and errors
  Mantid::API::ITableWorkspace_sptr m_parameterTable;

  /// Sections used for the last fit
  std::vector<Section> m_sections;

  // Setters for convenience
  void setFittedFunction(Mantid::API::IFunction_const_sptr function);

  // Set errors in the ws after the fit
  void setErrorsAfterFit(const Mantid::API::MatrixWorkspace_sptr &data);

  /// Disables points which shouldn't be used for fitting
  static void disableUnwantedPoints(const Mantid::API::MatrixWorkspace_sptr &ws, const std::vector<Section> &sections);

  /// Enable previously disabled points
  static void enableDisabledPoints(const Mantid::API::MatrixWorkspace_sptr &destWs,
                                   const Mantid::API::MatrixWorkspace_const_sptr &sourceWs);
};

} // namespace CustomInterfaces
} // namespace MantidQt
