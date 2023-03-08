// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectDataValidationHelper.h"
#include "IndirectFitDataModel.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include <typeinfo>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL InelasticDataManipulationElwinTabModel {

public:
  InelasticDataManipulationElwinTabModel();
  ~InelasticDataManipulationElwinTabModel() = default;
  void setupLoadAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &filepath,
                          std::string const &outputName);
  std::string createGroupedWorkspaces(MatrixWorkspace_sptr workspace, FunctionModelSpectra spectra);
  void setupGroupAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                           std::string const &inputWorkspacesString, std::string const &inputGroupWsName);
  void setupElasticWindowMultiple(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, QString workspaceBaseName,
                                  std::string const &inputGroupWsName, std::string const &sampleEnvironmentLogName,
                                  std::string const &sampleEnvironmentLogValue);
  void ungroupAlgorithm(std::string const &InputWorkspace);
  void setIntegrationStart(double integrationStart);
  void setIntegrationEnd(double integrationEnd);
  void setBackgroundStart(double backgroundStart);
  void setBackgroundEnd(double backgroundEnd);
  void setBackgroundSubtraction(bool backgroundSubtraction);
  void setNormalise(bool normalise);

private:
  double m_integrationStart;
  double m_integrationEnd;
  double m_backgroundStart;
  double m_backgroundEnd;
  bool m_backgroundSubtraction;
  bool m_normalise;
};
} // namespace CustomInterfaces
} // namespace MantidQt