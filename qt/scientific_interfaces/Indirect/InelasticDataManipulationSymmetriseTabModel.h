// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectDataValidationHelper.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL InelasticDataManipulationSymmetriseTabModel {

public:
  InelasticDataManipulationSymmetriseTabModel();
  ~InelasticDataManipulationSymmetriseTabModel() = default;
  void setupPreviewAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::vector<long> spectraRange);
  std::string setupSymmetriseAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner);
  void setWorkspaceName(QString workspaceName);
  void setEMin(double value);
  void setEMax(double value);

private:
  std::string m_inputWorkspace;
  std::string m_outputWorkspace;
  double m_eMin;
  double m_eMax;
  std::vector<long> m_spectraRange;
};

} // namespace CustomInterfaces
} // namespace MantidQt