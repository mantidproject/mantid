// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Batch/IReflAlgorithmFactory.h"
#include "GUI/Common/IJobManager.h"
#include "MantidQtWidgets/Common/IJobRunner.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IBatch;

class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewJobManager final : public IJobManager, public API::JobRunnerSubscriber {
public:
  PreviewJobManager(std::unique_ptr<API::IJobRunner> jobRunner, std::unique_ptr<IReflAlgorithmFactory> algFactory);

  // IJobManager overrides
  void subscribe(JobManagerSubscriber *notifyee) override;
  void startPreprocessing(PreviewRow &row) override;
  void startSumBanks(PreviewRow &row) override;
  void startReduction(PreviewRow &row) override;

  // JobRunnerSubscriber overrides
  void notifyBatchComplete(bool) override;
  void notifyBatchCancelled() override;
  void notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr &) override;
  void notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) override;
  void notifyAlgorithmError(API::IConfiguredAlgorithm_sptr &, std::string const &) override;

private:
  std::unique_ptr<API::IJobRunner> m_jobRunner;
  std::unique_ptr<IReflAlgorithmFactory> m_algFactory;
  JobManagerSubscriber *m_notifyee;

  void executeAlg(API::IConfiguredAlgorithm_sptr alg);
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry