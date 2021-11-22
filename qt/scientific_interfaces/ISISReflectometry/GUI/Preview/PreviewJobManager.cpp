// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewJobManager.h"
#include "GUI/Batch/IBatchJobAlgorithm.h"
#include "GUI/Batch/IReflAlgorithmFactory.h"
#include "GUI/Batch/ReflAlgorithmFactory.h"
#include "GUI/Common/IJobRunner.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Item.h"

#include <memory>

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Preview Job Manager");

constexpr auto PREPROCESS_ALG_NAME = "ReflectometryISISPreprocess";
constexpr auto SUM_BANKS_ALG_NAME = "ReflectometryISISSumBanks";

enum class AlgorithmType { PREPROCESS, SUM_BANKS };

AlgorithmType algorithmType(MantidQt::API::IConfiguredAlgorithm_sptr &configuredAlg) {
  auto const &name = configuredAlg->algorithm()->name();
  if (name == PREPROCESS_ALG_NAME) {
    return AlgorithmType::PREPROCESS;
  } else if (name == SUM_BANKS_ALG_NAME) {
    return AlgorithmType::SUM_BANKS;
  } else {
    throw std::logic_error(std::string("Preview tab error: callback from invalid algorithm ") + name);
  }
}
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using MantidQt::API::IConfiguredAlgorithm_sptr;

PreviewJobManager::PreviewJobManager(IJobRunner *jobRunner, std::unique_ptr<IReflAlgorithmFactory> algFactory)
    : m_jobRunner(jobRunner), m_algFactory(std::move(algFactory)) {
  m_jobRunner->subscribe(this);
}

void PreviewJobManager::subscribe(JobManagerSubscriber *notifyee) { m_notifyee = notifyee; }

void PreviewJobManager::notifyBatchComplete(bool) {}

void PreviewJobManager::notifyBatchCancelled() {}

void PreviewJobManager::notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr &) {}

void PreviewJobManager::notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) {
  auto jobAlgorithm = std::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto item = jobAlgorithm->item();
  if (!item || !item->isPreview())
    return;

  try {
    jobAlgorithm->updateItem();
  } catch (std::runtime_error const &ex) {
    g_log.error(ex.what());
    return;
  }

  switch (algorithmType(algorithm)) {
  case AlgorithmType::PREPROCESS:
    m_notifyee->notifyLoadWorkspaceCompleted();
    break;
  case AlgorithmType::SUM_BANKS:
    m_notifyee->notifySumBanksCompleted();
    break;
  };
}

void PreviewJobManager::notifyAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) {
  auto jobAlgorithm = std::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto item = jobAlgorithm->item();
  if (!item || !item->isPreview())
    return;

  // TODO It would probably be good to report these as popups instead of in the log. We can do this by
  // injecting IReflMessageHandler as other tabs do. This is not urgent for the initial implementation though.
  switch (algorithmType(algorithm)) {
  case AlgorithmType::PREPROCESS:
    g_log.error(std::string("Error loading workspace: ") + message);
    break;
  case AlgorithmType::SUM_BANKS:
    g_log.error(std::string("Error summing banks: ") + message);
    break;
  };
}

void PreviewJobManager::startPreprocessing(PreviewRow &row) {
  executeAlg(m_algFactory->makePreprocessingAlgorithm(row));
}

void PreviewJobManager::startSumBanks(PreviewRow &row) { executeAlg(m_algFactory->makeSumBanksAlgorithm(row)); }

void PreviewJobManager::executeAlg(IConfiguredAlgorithm_sptr alg) {
  m_jobRunner->clearAlgorithmQueue();
  m_jobRunner->setAlgorithmQueue(std::deque<IConfiguredAlgorithm_sptr>{std::move(alg)});
  m_jobRunner->executeAlgorithmQueue();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
