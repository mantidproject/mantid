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
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL SymmetriseModel {

public:
  SymmetriseModel();
  ~SymmetriseModel() = default;
  void setupPreviewAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::vector<long> spectraRange);
  std::string setupSymmetriseAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner);
  void reflectNegativeToPositive();
  void setWorkspaceName(std::string const &workspaceName);
  void setEMin(double value);
  void setEMax(double value);
  void setIsPositiveReflect(bool value);
  bool getIsPositiveReflect() const;

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