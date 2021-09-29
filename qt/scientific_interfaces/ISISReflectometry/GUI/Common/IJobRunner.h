// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"

#include <deque>
#include <memory>

namespace MantidQt::API {
class IConfiguredAlgorithm;
using IConfiguredAlgorithm_sptr = std::shared_ptr<IConfiguredAlgorithm>;
} // namespace MantidQt::API

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** @class JobRunnerSubscriber
 *
 * JobRunnerSubscriber is an interface to a class that subscribes to notifications from an IJobRunner
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL JobRunnerSubscriber {
public:
  virtual ~JobRunnerSubscriber() = default;

  virtual void notifyBatchComplete(bool error) = 0;
  virtual void notifyBatchCancelled() = 0;
  virtual void notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr &algorithm) = 0;
  virtual void notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) = 0;
  virtual void notifyAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) = 0;
};

/** @class IJobRunner
 *
 * IJobRunner is an interface to a class that provides functionality to run a batch algorithm queue
 */
class IJobRunner {
public:
  virtual ~IJobRunner() = default;
  virtual void subscribe(JobRunnerSubscriber *notifyee) = 0;
  virtual void clearAlgorithmQueue() = 0;
  virtual void setAlgorithmQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithms) = 0;
  virtual void executeAlgorithmQueue() = 0;
  virtual void cancelAlgorithmQueue() = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry