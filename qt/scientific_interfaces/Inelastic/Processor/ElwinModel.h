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
#include "MantidQtWidgets/Spectroscopy/DataModel.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include <typeinfo>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IElwinModel {
public:
  virtual ~IElwinModel() = default;

  virtual API::IConfiguredAlgorithm_sptr setupLoadAlgorithm(std::string const &filepath,
                                                            std::string const &outputName) const = 0;
  virtual std::string setupExtractSpectra(MatrixWorkspace_sptr workspace, FunctionModelSpectra const &spectra,
                                          std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> *algQueue) const = 0;
  virtual API::IConfiguredAlgorithm_sptr setupGroupAlgorithm(std::string const &inputWorkspacesString,
                                                             std::string const &inputGroupWsName) const = 0;
  virtual API::IConfiguredAlgorithm_sptr setupElasticWindowMultiple(std::string const &workspaceBaseName,
                                                                    std::string const &inputGroupWsName,
                                                                    std::string const &sampleEnvironmentLogName,
                                                                    std::string const &sampleEnvironmentLogValue) = 0;
  virtual void ungroupAlgorithm(std::string const &inputWorkspace) const = 0;
  virtual void groupAlgorithm(std::string const &inputWorkspaces, std::string const &outputWorkspace) const = 0;
  virtual void setIntegrationStart(double integrationStart) = 0;
  virtual void setIntegrationEnd(double integrationEnd) = 0;
  virtual void setBackgroundStart(double backgroundStart) = 0;
  virtual void setBackgroundEnd(double backgroundEnd) = 0;
  virtual void setBackgroundSubtraction(bool backgroundSubtraction) = 0;
  virtual void setNormalise(bool normalise) = 0;
  virtual void setOutputWorkspaceNames(std::string const &workspaceBaseName) = 0;
  virtual std::string getOutputWorkspaceNames() const = 0;
};

class MANTIDQT_INELASTIC_DLL ElwinModel final : public IElwinModel {

public:
  ElwinModel();
  ~ElwinModel() override = default;
  API::IConfiguredAlgorithm_sptr setupLoadAlgorithm(std::string const &filepath,
                                                    std::string const &outputName) const override;
  std::string setupExtractSpectra(MatrixWorkspace_sptr workspace, FunctionModelSpectra const &spectra,
                                  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> *algQueue) const override;
  API::IConfiguredAlgorithm_sptr setupGroupAlgorithm(std::string const &inputWorkspacesString,
                                                     std::string const &inputGroupWsName) const override;
  API::IConfiguredAlgorithm_sptr setupElasticWindowMultiple(std::string const &workspaceBaseName,
                                                            std::string const &inputGroupWsName,
                                                            std::string const &sampleEnvironmentLogName,
                                                            std::string const &sampleEnvironmentLogValue) override;
  void ungroupAlgorithm(std::string const &inputWorkspace) const override;
  void groupAlgorithm(std::string const &inputWorkspaces, std::string const &outputWorkspace) const override;
  void setIntegrationStart(double integrationStart) override;
  void setIntegrationEnd(double integrationEnd) override;
  void setBackgroundStart(double backgroundStart) override;
  void setBackgroundEnd(double backgroundEnd) override;
  void setBackgroundSubtraction(bool backgroundSubtraction) override;
  void setNormalise(bool normalise) override;
  void setOutputWorkspaceNames(std::string const &workspaceBaseName) override;
  std::string getOutputWorkspaceNames() const override;

private:
  double m_integrationStart;
  double m_integrationEnd;
  double m_backgroundStart;
  double m_backgroundEnd;
  bool m_backgroundSubtraction;
  bool m_normalise;
  std::unordered_map<std::string, std::string> m_outputWorkspaceNames;
};
} // namespace CustomInterfaces
} // namespace MantidQt
