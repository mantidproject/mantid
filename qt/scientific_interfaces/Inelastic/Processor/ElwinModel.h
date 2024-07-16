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
  virtual void setupLoadAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &filepath,
                                  std::string const &outputName) = 0;
  virtual std::string createGroupedWorkspaces(MatrixWorkspace_sptr workspace, FunctionModelSpectra const &spectra) = 0;
  virtual void setupGroupAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                   std::string const &inputWorkspacesString, std::string const &inputGroupWsName) = 0;
  virtual void setupElasticWindowMultiple(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                          std::string const &workspaceBaseName, std::string const &inputGroupWsName,
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

class MANTIDQT_INELASTIC_DLL ElwinModel : public IElwinModel {

public:
  ElwinModel();
  ~ElwinModel() = default;
  void setupLoadAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &filepath,
                          std::string const &outputName) override;
  std::string createGroupedWorkspaces(MatrixWorkspace_sptr workspace, FunctionModelSpectra const &spectra) override;
  void setupGroupAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                           std::string const &inputWorkspacesString, std::string const &inputGroupWsName) override;
  void setupElasticWindowMultiple(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                  std::string const &workspaceBaseName, std::string const &inputGroupWsName,
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