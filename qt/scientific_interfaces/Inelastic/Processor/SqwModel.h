// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL ISqwModel {

public:
  virtual ~ISqwModel() = default;
  virtual API::IConfiguredAlgorithm_sptr setupRebinAlgorithm() const = 0;
  virtual API::IConfiguredAlgorithm_sptr setupSofQWAlgorithm() const = 0;
  virtual API::IConfiguredAlgorithm_sptr setupAddSampleLogAlgorithm() const = 0;
  virtual void setInputWorkspace(const std::string &workspace) = 0;
  virtual void setQMin(double qMin) = 0;
  virtual void setQWidth(double qWidth) = 0;
  virtual void setQMax(double qMax) = 0;
  virtual void setEMin(double eMin) = 0;
  virtual void setEWidth(double eWidth) = 0;
  virtual void setEMax(double eMax) = 0;
  virtual void setEFixed(const double eFixed) = 0;
  virtual void setRebinInEnergy(bool scale) = 0;
  virtual bool isRebinInEnergy() const = 0;
  virtual std::string getEFixedFromInstrument(std::string const &instrumentName, std::string analyser,
                                              std::string const &reflection) const = 0;
  virtual std::string getOutputWorkspace() const = 0;
  virtual MatrixWorkspace_sptr getRqwWorkspace() const = 0;
  virtual void validate(IUserInputValidator *validator, std::tuple<double, double> const qRange,
                        std::tuple<double, double> const eRange) const = 0;
  virtual MatrixWorkspace_sptr loadInstrumentWorkspace(const std::string &instrumentName, const std::string &analyser,
                                                       const std::string &reflection) const = 0;
};

class MANTIDQT_INELASTIC_DLL SqwModel final : public ISqwModel {

public:
  SqwModel();
  ~SqwModel() override = default;
  API::IConfiguredAlgorithm_sptr setupRebinAlgorithm() const override;
  API::IConfiguredAlgorithm_sptr setupSofQWAlgorithm() const override;
  API::IConfiguredAlgorithm_sptr setupAddSampleLogAlgorithm() const override;
  void setInputWorkspace(const std::string &workspace) override;
  void setQMin(double qMin) override;
  void setQWidth(double qWidth) override;
  void setQMax(double qMax) override;
  void setEMin(double eMin) override;
  void setEWidth(double eWidth) override;
  void setEMax(double eMax) override;
  void setEFixed(const double eFixed) override;
  void setRebinInEnergy(bool scale) override;
  bool isRebinInEnergy() const override;
  std::string getEFixedFromInstrument(std::string const &instrumentName, std::string analyser,
                                      std::string const &reflection) const override;
  std::string getOutputWorkspace() const override;
  MatrixWorkspace_sptr getRqwWorkspace() const override;
  void validate(IUserInputValidator *validator, std::tuple<double, double> const qRange,
                std::tuple<double, double> const eRange) const override;
  MatrixWorkspace_sptr loadInstrumentWorkspace(const std::string &instrumentName, const std::string &analyser,
                                               const std::string &reflection) const override;

private:
  std::string m_inputWorkspace;
  std::string m_baseName;
  double m_eFixed;
  double m_qLow;
  double m_qWidth;
  double m_qHigh;
  double m_eLow;
  double m_eWidth;
  double m_eHigh;
  bool m_rebinInEnergy;
};
} // namespace CustomInterfaces
} // namespace MantidQt