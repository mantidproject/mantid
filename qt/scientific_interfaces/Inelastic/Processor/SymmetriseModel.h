// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL ISymmetriseModel {

public:
  virtual ~ISymmetriseModel() = default;
  virtual API::IConfiguredAlgorithm_sptr setupPreviewAlgorithm(std::vector<int> const &spectraRange) = 0;
  virtual API::IConfiguredAlgorithm_sptr setupSymmetriseAlgorithm() = 0;
  virtual void reflectNegativeToPositive() = 0;
  virtual void setWorkspaceName(std::string const &workspaceName) = 0;
  virtual void setEMin(double value) = 0;
  virtual void setEMax(double value) = 0;
  virtual void setIsPositiveReflect(bool value) = 0;
  virtual bool getIsPositiveReflect() const = 0;
};

class MANTIDQT_INELASTIC_DLL SymmetriseModel final : public ISymmetriseModel {

public:
  SymmetriseModel();
  ~SymmetriseModel() override = default;
  API::IConfiguredAlgorithm_sptr setupPreviewAlgorithm(std::vector<int> const &spectraRange) override;
  API::IConfiguredAlgorithm_sptr setupSymmetriseAlgorithm() override;
  void reflectNegativeToPositive() override;
  void setWorkspaceName(std::string const &workspaceName) override;
  void setEMin(double value) override;
  void setEMax(double value) override;
  void setIsPositiveReflect(bool value) override;
  bool getIsPositiveReflect() const override;

private:
  std::string m_inputWorkspace;
  std::string m_reflectedInputWorkspace;
  std::string m_negativeOutputWorkspace;
  std::string m_positiveOutputWorkspace;
  double m_eMin;
  double m_eMax;
  bool m_isPositiveReflect;
  std::vector<long> m_spectraRange;
};

} // namespace CustomInterfaces
} // namespace MantidQt
