// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAlgorithmManager.h"
#include "IALFAlgorithmManagerSubscriber.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"

using namespace Mantid::API;

namespace {
std::string const NOT_IN_ADS = "not_stored_in_ads";

IAlgorithm_sptr loadAlgorithm(std::string const &filename) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("Filename", filename);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  return alg;
}

IAlgorithm_sptr normaliseByCurrentAlgorithm(MatrixWorkspace_sptr const &inputWorkspace) {
  auto alg = AlgorithmManager::Instance().create("NormaliseByCurrent");
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  return alg;
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFAlgorithmManager::ALFAlgorithmManager(std::unique_ptr<API::IJobRunner> jobRunner)
    : m_currentTask(), m_jobRunner(std::move(jobRunner)), m_subscriber() {
  m_jobRunner->subscribe(this);
}

void ALFAlgorithmManager::subscribe(IALFAlgorithmManagerSubscriber *subscriber) { m_subscriber = subscriber; }

void ALFAlgorithmManager::load(ALFTask const &task, std::string const &filename) {
  m_currentTask = task;
  auto properties = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();

  auto llll = loadAlgorithm(filename);
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> alg = {
      std::make_shared<MantidQt::API::ConfiguredAlgorithm>(std::move(llll), std::move(properties))};

  m_jobRunner->setAlgorithmQueue(alg);
  m_jobRunner->executeAlgorithmQueue();
}

void ALFAlgorithmManager::normaliseByCurrent(ALFTask const &task, MatrixWorkspace_sptr const &workspace) {
  m_currentTask = task;
  auto properties = std::make_unique<MantidQt::API::AlgorithmRuntimeProps>();

  auto llll = normaliseByCurrentAlgorithm(workspace);
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> alg = {
      std::make_shared<MantidQt::API::ConfiguredAlgorithm>(std::move(llll), std::move(properties))};

  m_jobRunner->setAlgorithmQueue(alg);
  m_jobRunner->executeAlgorithmQueue();
}

void ALFAlgorithmManager::notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) {
  switch (m_currentTask) {
  case ALFTask::SAMPLE_LOAD:
    notifySampleLoadComplete(algorithm->algorithm());
    return;
  case ALFTask::SAMPLE_NORMALISE:
    notifySampleNormaliseComplete(algorithm->algorithm());
    return;
  case ALFTask::VANADIUM_LOAD:
    return;
  }
}

void ALFAlgorithmManager::notifySampleLoadComplete(IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  Workspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifySampleLoaded(std::dynamic_pointer_cast<MatrixWorkspace>(outputWorkspace));
}

void ALFAlgorithmManager::notifySampleNormaliseComplete(IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifySampleNormalised(outputWorkspace);
}

} // namespace MantidQt::CustomInterfaces