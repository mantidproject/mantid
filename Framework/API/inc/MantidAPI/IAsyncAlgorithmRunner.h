// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithm_fwd.h"

namespace Mantid::API {

class IAsyncAlgorithmSubscriber;

class MANTID_API_DLL IAsyncAlgorithmRunner {

public:
  virtual ~IAsyncAlgorithmRunner() = default;

  virtual void subscribe(IAsyncAlgorithmSubscriber *subscriber) = 0;

  virtual void cancelRunningAlgorithm() = 0;

  virtual void startAlgorithm(IAlgorithm_sptr alg) = 0;
  virtual IAlgorithm_sptr getAlgorithm() const = 0;
};

} // namespace Mantid::API
