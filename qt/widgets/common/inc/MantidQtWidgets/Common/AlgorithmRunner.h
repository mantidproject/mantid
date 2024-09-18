// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"
#include "MantidQtWidgets/Common/IJobRunner.h"

#include <deque>
#include <memory>

namespace MantidQt::API {

class IAlgorithmRunnerSubscriber;

class EXPORT_OPT_MANTIDQT_COMMON IAlgorithmRunner {

public:
  virtual ~IAlgorithmRunner() = default;

  virtual void subscribe(IAlgorithmRunnerSubscriber *subscriber) = 0;

  virtual void execute(IConfiguredAlgorithm_sptr algorithm) = 0;
  virtual void execute(std::deque<IConfiguredAlgorithm_sptr> algorithmQueue) = 0;
};

class EXPORT_OPT_MANTIDQT_COMMON AlgorithmRunner final : public IAlgorithmRunner, public JobRunnerSubscriber {

public:
  AlgorithmRunner(std::unique_ptr<IJobRunner> jobRunner);

  void subscribe(IAlgorithmRunnerSubscriber *subscriber) override;

  void execute(IConfiguredAlgorithm_sptr algorithm) override;
  void execute(std::deque<IConfiguredAlgorithm_sptr> algorithmQueue) override;

  void notifyBatchComplete(bool error) override;
  void notifyBatchCancelled() override;
  void notifyAlgorithmStarted(IConfiguredAlgorithm_sptr &algorithm) override;
  void notifyAlgorithmComplete(IConfiguredAlgorithm_sptr &algorithm) override;
  void notifyAlgorithmError(IConfiguredAlgorithm_sptr &algorithm, std::string const &message) override;

private:
  std::unique_ptr<IJobRunner> m_jobRunner;
  IAlgorithmRunnerSubscriber *m_subscriber;

  IConfiguredAlgorithm_sptr m_lastAlgorithm;
};

} // namespace MantidQt::API
