// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"
#include "MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h"

namespace MantidQt::API {

AlgorithmRunner::AlgorithmRunner(std::unique_ptr<IJobRunner> jobRunner)
    : m_jobRunner(std::move(jobRunner)), m_subscriber(), m_lastAlgorithm() {
  m_jobRunner->subscribe(this);
}

void AlgorithmRunner::subscribe(IAlgorithmRunnerSubscriber *subscriber) { m_subscriber = subscriber; }

void AlgorithmRunner::execute(IConfiguredAlgorithm_sptr algorithm) {
  m_jobRunner->executeAlgorithm(std::move(algorithm));
}

void AlgorithmRunner::execute(std::deque<IConfiguredAlgorithm_sptr> algorithmQueue) {
  m_jobRunner->setAlgorithmQueue(std::move(algorithmQueue));
  m_jobRunner->executeAlgorithmQueue();
}

void AlgorithmRunner::notifyBatchComplete(bool error) { m_subscriber->notifyBatchComplete(m_lastAlgorithm, error); }

void AlgorithmRunner::notifyBatchCancelled() { m_subscriber->notifyBatchCancelled(); }

void AlgorithmRunner::notifyAlgorithmStarted(IConfiguredAlgorithm_sptr &algorithm) {
  m_subscriber->notifyAlgorithmStarted(algorithm);
}

void AlgorithmRunner::notifyAlgorithmComplete(IConfiguredAlgorithm_sptr &algorithm) {
  m_subscriber->notifyAlgorithmComplete(algorithm);
  m_lastAlgorithm = algorithm;
}

void AlgorithmRunner::notifyAlgorithmError(IConfiguredAlgorithm_sptr &algorithm, std::string const &message) {
  m_subscriber->notifyAlgorithmError(algorithm, message);
  m_lastAlgorithm = algorithm;
}

} // namespace MantidQt::API
