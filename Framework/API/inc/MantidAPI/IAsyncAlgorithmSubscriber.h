// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithm_fwd.h"

#include <string>

namespace Mantid::API {

class MANTID_API_DLL IAsyncAlgorithmSubscriber {

public:
  virtual ~IAsyncAlgorithmSubscriber() = default;

  virtual void notifyAlgorithmProgress(double const progress, std::string const &message) {
    (void)progress;
    (void)message;
  };

  virtual void notifyAlgorithmError(IAlgorithm const *alg, std::string const &message) = 0;
  virtual void notifyAlgorithmFinished(IAlgorithm const *alg) = 0;
};

} // namespace Mantid::API
