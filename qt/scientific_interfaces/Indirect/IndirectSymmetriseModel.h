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

class MANTIDQT_INDIRECT_DLL IndirectSymmetriseModel {

public:
  IndirectSymmetriseModel();
  ~IndirectSymmetriseModel() = default;
  void setupPreviewAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, QString workspaceName, double eMin,
                             double eMax, std::vector<long> spectraRange);
  std::string setupSymmetriseAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, QString workspaceName,
                                       double eMin, double eMax);
};
} // namespace CustomInterfaces
} // namespace MantidQt