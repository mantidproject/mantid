// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

#include <string>

namespace MantidQt::API {

class EXPORT_OPT_MANTIDQT_COMMON IAlgorithmRunnerSubscriber {

public:
  virtual ~IAlgorithmRunnerSubscriber() = default;

  virtual void notifyBatchComplete(IConfiguredAlgorithm_sptr &lastAlgorithm, bool error) {
    (void)lastAlgorithm;
    (void)error;
  };
  virtual void notifyBatchCancelled() {};
  virtual void notifyAlgorithmStarted(IConfiguredAlgorithm_sptr &algorithm) { (void)algorithm; };
  virtual void notifyAlgorithmComplete(IConfiguredAlgorithm_sptr &algorithm) { (void)algorithm; };
  virtual void notifyAlgorithmError(IConfiguredAlgorithm_sptr &algorithm, std::string const &message) {
    (void)algorithm;
    (void)message;
  };
};

} // namespace MantidQt::API
