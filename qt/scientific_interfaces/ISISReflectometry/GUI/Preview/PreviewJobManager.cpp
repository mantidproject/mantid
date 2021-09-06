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

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using MantidQt::API::IConfiguredAlgorithm_sptr;

PreviewJobManager::PreviewJobManager(IJobRunner *jobRunner, std::unique_ptr<IReflAlgorithmFactory> algFactory)
    : m_jobRunner(jobRunner), m_algFactory(std::move(algFactory)) {
  m_jobRunner->subscribe(this);
}

void PreviewJobManager::subscribe(JobManagerSubscriber *notifyee) { m_notifyee = notifyee; }

void PreviewJobManager::notifyBatchComplete(bool) {}

void PreviewJobManager::notifyBatchCancelled() {}

void PreviewJobManager::notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr) {}

void PreviewJobManager::notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr algorithm) {
  auto jobAlgorithm = std::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto item = jobAlgorithm->item();
  if (!item || !item->isPreview())
    return;

  // TODO When the full implementation is added we will need to switch between different algorithm cases.
  // For now we just deal with loading.
  m_notifyee->notifyLoadWorkspaceCompleted();
}

void PreviewJobManager::notifyAlgorithmError(API::IConfiguredAlgorithm_sptr, std::string const &) {}

void PreviewJobManager::startPreprocessing(PreviewRow &row) {
  executeAlg(m_algFactory->makePreprocessingAlgorithm(row));
}

void PreviewJobManager::executeAlg(IConfiguredAlgorithm_sptr alg) {
  m_jobRunner->clearAlgorithmQueue();
  m_jobRunner->setAlgorithmQueue(std::deque<IConfiguredAlgorithm_sptr>{std::move(alg)});
  m_jobRunner->executeAlgorithmQueue();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry