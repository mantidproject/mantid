// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "QtJobRunner.h"
#include "GUI/Common/IJobRunner.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

using namespace MantidQt::API;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

void QtJobRunner::subscribe(JobRunnerSubscriber *notifyee) { m_notifyees.push_back(notifyee); }

void QtJobRunner::clearAlgorithmQueue() { m_batchAlgoRunner.clearQueue(); }

void QtJobRunner::setAlgorithmQueue(std::deque<IConfiguredAlgorithm_sptr> algorithms) {
  m_batchAlgoRunner.setQueue(algorithms);
}

void QtJobRunner::executeAlgorithmQueue() { m_batchAlgoRunner.executeBatchAsync(); }

void QtJobRunner::cancelAlgorithmQueue() { m_batchAlgoRunner.cancelBatch(); }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
