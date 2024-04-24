// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/IndirectDataValidationHelper.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "QENSFitting/FitDataModel.h"
#include <typeinfo>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL ElwinModel {

public:
  ElwinModel();
  virtual ~ElwinModel() = default;
  virtual void setupLoadAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &filepath,
                                  std::string const &outputName);
  virtual std::string createGroupedWorkspaces(MatrixWorkspace_sptr workspace, FunctionModelSpectra const &spectra);
  virtual void setupGroupAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                   std::string const &inputWorkspacesString, std::string const &inputGroupWsName);
  virtual void setupElasticWindowMultiple(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                          std::string const &workspaceBaseName, std::string const &inputGroupWsName,
                                          std::string const &sampleEnvironmentLogName,
                                          std::string const &sampleEnvironmentLogValue);
  virtual void ungroupAlgorithm(std::string const &inputWorkspace) const;
  virtual void groupAlgorithm(std::string const &inputWorkspaces, std::string const &outputWorkspace) const;
  virtual void setIntegrationStart(double integrationStart);
  virtual void setIntegrationEnd(double integrationEnd);
  virtual void setBackgroundStart(double backgroundStart);
  virtual void setBackgroundEnd(double backgroundEnd);
  virtual void setBackgroundSubtraction(bool backgroundSubtraction);
  virtual void setNormalise(bool normalise);
  virtual void setOutputWorkspaceNames(std::string const &workspaceBaseName);
  virtual std::string getOutputWorkspaceNames() const;

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