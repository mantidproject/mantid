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
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/IJobRunner.h"
#include "Reduction/Item.h"
#include "Reduction/RowExceptions.h"

#include <memory>

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Preview Job Manager");

constexpr auto PREPROCESS_ALG_NAME = "ReflectometryISISPreprocess";
constexpr auto SUM_BANKS_ALG_NAME = "ReflectometryISISSumBanks";
constexpr auto REDUCTION_ALG_NAME = "ReflectometryISISLoadAndProcess";

enum class AlgorithmType { PREPROCESS, SUM_BANKS, REDUCTION };

AlgorithmType algorithmType(MantidQt::API::IConfiguredAlgorithm_sptr &configuredAlg) {
  auto const &name = configuredAlg->algorithm()->name();
  if (name == PREPROCESS_ALG_NAME) {
    return AlgorithmType::PREPROCESS;
  } else if (name == SUM_BANKS_ALG_NAME) {
    return AlgorithmType::SUM_BANKS;
  } else if (name == REDUCTION_ALG_NAME) {
    return AlgorithmType::REDUCTION;
  } else {
    throw std::logic_error(std::string("Preview tab error: callback from invalid algorithm ") + name);
  }
}

const std::string LOADING_ERROR_PREFIX("Error loading workspace: ");
const std::string SUMMING_ERROR_PREFIX("Error summing banks: ");
const std::string REDUCTION_ERROR_PREFIX("Error performing reduction: ");
const std::string MULTIPLE_ROWS_ERROR_MSG("Matched multiple lookup rows in the Experiment Settings tab");
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using MantidQt::API::IConfiguredAlgorithm_sptr;

PreviewJobManager::PreviewJobManager(std::unique_ptr<API::IJobRunner> jobRunner,
                                     std::unique_ptr<IReflAlgorithmFactory> algFactory)
    : m_jobRunner(std::move(jobRunner)), m_algFactory(std::move(algFactory)) {
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
    notifyAlgorithmError(algorithm, ex.what());
    return;
  }

  switch (algorithmType(algorithm)) {
  case AlgorithmType::PREPROCESS:
    m_notifyee->notifyLoadWorkspaceCompleted();
    break;
  case AlgorithmType::SUM_BANKS:
    m_notifyee->notifySumBanksCompleted();
    break;
  case AlgorithmType::REDUCTION:
    m_notifyee->notifyReductionCompleted();
    break;
  };
}

void PreviewJobManager::notifyAlgorithmError(API::IConfiguredAlgorithm_sptr &algorithm, std::string const &message) {
  auto jobAlgorithm = std::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto item = jobAlgorithm->item();
  if (!item || !item->isPreview())
    return;

  // TODO It would probably be good to report these as popups instead of in the log. We can do this by
  // injecting IReflMessageHandler as other tabs do. This is not urgent for the initial implementation though.
  switch (algorithmType(algorithm)) {
  case AlgorithmType::PREPROCESS:
    g_log.error(LOADING_ERROR_PREFIX + message);
    m_notifyee->notifyLoadWorkspaceAlgorithmError();
    break;
  case AlgorithmType::SUM_BANKS:
    g_log.error(SUMMING_ERROR_PREFIX + message);
    m_notifyee->notifySumBanksAlgorithmError();
    break;
  case AlgorithmType::REDUCTION:
    g_log.error(REDUCTION_ERROR_PREFIX + message);
    m_notifyee->notifyReductionAlgorithmError();
    break;
  };
}

void PreviewJobManager::startPreprocessing(PreviewRow &row) {
  executeAlg(m_algFactory->makePreprocessingAlgorithm(row));
}

void PreviewJobManager::startSumBanks(PreviewRow &row) {
  try {
    executeAlg(m_algFactory->makeSumBanksAlgorithm(row));
  } catch (MultipleRowsFoundException const &) {
    g_log.error(SUMMING_ERROR_PREFIX + MULTIPLE_ROWS_ERROR_MSG);
    m_notifyee->notifySumBanksAlgorithmError();
    return;
  }
}

void PreviewJobManager::startReduction(PreviewRow &row) {
  try {
    executeAlg(m_algFactory->makeReductionAlgorithm(row));
  } catch (MultipleRowsFoundException const &) {
    g_log.error(REDUCTION_ERROR_PREFIX + MULTIPLE_ROWS_ERROR_MSG);
    m_notifyee->notifyReductionAlgorithmError();
    return;
  }
}

void PreviewJobManager::executeAlg(IConfiguredAlgorithm_sptr alg) {
  m_jobRunner->clearAlgorithmQueue();
  m_jobRunner->setAlgorithmQueue(std::deque<IConfiguredAlgorithm_sptr>{std::move(alg)});
  m_jobRunner->executeAlgorithmQueue();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
