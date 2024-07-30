// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include <MantidQtWidgets/Common/IConfiguredAlgorithm.h>
#include <typeinfo>

using namespace Mantid::API;
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IMomentsModel {

public:
  virtual ~IMomentsModel() = default;
  virtual API::IConfiguredAlgorithm_sptr setupMomentsAlgorithm() const = 0;
  virtual void setInputWorkspace(const std::string &workspace) = 0;
  virtual void setEMin(double eMin) = 0;
  virtual void setEMax(double eMax) = 0;
  virtual void setScale(bool scale) = 0;
  virtual void setScaleValue(double scaleValue) = 0;
  virtual std::string getOutputWorkspace() const = 0;
};

class MANTIDQT_INELASTIC_DLL MomentsModel final : public IMomentsModel {

public:
  MomentsModel();
  ~MomentsModel() override = default;
  API::IConfiguredAlgorithm_sptr setupMomentsAlgorithm() const override;
  void setInputWorkspace(const std::string &workspace) override;
  void setEMin(double eMin) override;
  void setEMax(double eMax) override;
  void setScale(bool scale) override;
  void setScaleValue(double scaleValue) override;
  std::string getOutputWorkspace() const override;

private:
  std::string m_inputWorkspace;
  std::string m_outputWorkspaceName;
  double m_eMin;
  double m_eMax;
  double m_scaleValue;
  bool m_scale;
};
} // namespace CustomInterfaces
} // namespace MantidQt
