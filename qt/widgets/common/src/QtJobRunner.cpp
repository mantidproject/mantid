// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/QtJobRunner.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"
#include "MantidQtWidgets/Common/IJobRunner.h"

using namespace MantidQt::API;

namespace MantidQt::API {

QtJobRunner::QtJobRunner(bool const stopOnFailure) : QObject(), m_batchAlgoRunner(this) {
  qRegisterMetaType<API::IConfiguredAlgorithm_sptr>("MantidQt::API::IConfiguredAlgorithm_sptr");
  m_batchAlgoRunner.stopOnFailure(stopOnFailure);
  connectBatchAlgoRunnerSlots();
}

void QtJobRunner::subscribe(JobRunnerSubscriber *notifyee) { m_notifyee = notifyee; }

void QtJobRunner::clearAlgorithmQueue() { m_batchAlgoRunner.clearQueue(); }

void QtJobRunner::setAlgorithmQueue(std::deque<IConfiguredAlgorithm_sptr> algorithms) {
  m_batchAlgoRunner.setQueue(std::move(algorithms));
}

void QtJobRunner::executeAlgorithmQueue() { m_batchAlgoRunner.executeBatchAsync(); }

void QtJobRunner::executeAlgorithm(IConfiguredAlgorithm_sptr algorithm) {
  m_batchAlgoRunner.executeAlgorithmAsync(std::move(algorithm));
}

void QtJobRunner::cancelAlgorithmQueue() { m_batchAlgoRunner.cancelBatch(); }

void QtJobRunner::connectBatchAlgoRunnerSlots() {
  connect(&m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(onBatchComplete(bool)));
  connect(&m_batchAlgoRunner, SIGNAL(batchCancelled()), this, SLOT(onBatchCancelled()));
  connect(&m_batchAlgoRunner, SIGNAL(algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr)), this,
          SLOT(onAlgorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr)));
  connect(&m_batchAlgoRunner, SIGNAL(algorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr)), this,
          SLOT(onAlgorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr)));
  connect(&m_batchAlgoRunner, SIGNAL(algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr, std::string)), this,
          SLOT(onAlgorithmError(MantidQt::API::IConfiguredAlgorithm_sptr, std::string)));
}

void QtJobRunner::onBatchComplete(bool error) { m_notifyee->notifyBatchComplete(error); }

void QtJobRunner::onBatchCancelled() { m_notifyee->notifyBatchCancelled(); }

void QtJobRunner::onAlgorithmStarted(API::IConfiguredAlgorithm_sptr algorithm) {
  m_notifyee->notifyAlgorithmStarted(algorithm);
}

void QtJobRunner::onAlgorithmComplete(API::IConfiguredAlgorithm_sptr algorithm) {
  m_notifyee->notifyAlgorithmComplete(algorithm);
}

void QtJobRunner::onAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm, const std::string &message) {
  m_notifyee->notifyAlgorithmError(algorithm, message);
}

} // namespace MantidQt::API
