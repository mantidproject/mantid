// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"

#include <optional>

namespace Mantid::API {

class MANTID_API_DLL IAsyncAlgorithmSubscriber {

public:
  virtual ~IAsyncAlgorithmSubscriber() = default;

  virtual void notifyAlgorithmFinished(std::string const &algorithmName,
                                       std::optional<std::string> const &error = std::nullopt) = 0;
  virtual void notifyAlgorithmProgress(double const progress, std::string const &message) {
    (void)progress;
    (void)message;
  };
};

} // namespace Mantid::API
