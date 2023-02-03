// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IJobRunner.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/JobRunners/BatchAlgorithmRunner.h"
#include <QWidget>
#include <deque>
#include <vector>

namespace MantidQt::API {
class JobRunnerSubscriber;

class EXPORT_OPT_MANTIDQT_COMMON QtJobRunner : public QWidget, public IJobRunner {
  Q_OBJECT
public:
  QtJobRunner();
  void subscribe(JobRunnerSubscriber *notifyee) override;
  void clearAlgorithmQueue() override;
  void setAlgorithmQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithms) override;
  void executeAlgorithmQueue() override;
  void cancelAlgorithmQueue() override;

private slots:
  void onBatchComplete(bool error);
  void onBatchCancelled();
  void onAlgorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr algorithm);
  void onAlgorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr algorithm);
  void onAlgorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm, const std::string &errorMessage);

private:
  MantidQt::API::BatchAlgorithmRunner m_batchAlgoRunner;
  JobRunnerSubscriber *m_notifyee;
  void connectBatchAlgoRunnerSlots();
};
} // namespace MantidQt::API
