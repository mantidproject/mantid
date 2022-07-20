// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IJobRunner.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include <deque>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class JobRunnerSubscriber;

class QtJobRunner : public IJobRunner {
public:
  void subscribe(JobRunnerSubscriber *notifyee) override;
  void clearAlgorithmQueue() override;
  void setAlgorithmQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithms) override;
  void executeAlgorithmQueue() override;
  void cancelAlgorithmQueue() override;

private:
  MantidQt::API::BatchAlgorithmRunner m_batchAlgoRunner;
  std::vector<JobRunnerSubscriber *> m_notifyees;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
