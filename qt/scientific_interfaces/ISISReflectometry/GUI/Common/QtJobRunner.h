// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IJobRunner.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include <QWidget>
#include <deque>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class JobRunnerSubscriber;

class MANTIDQT_ISISREFLECTOMETRY_DLL QtJobRunner : public QWidget, public IJobRunner {
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
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
