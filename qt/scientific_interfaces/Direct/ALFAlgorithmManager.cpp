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
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"

using namespace Mantid::API;

namespace {
auto constexpr NOT_IN_ADS = "not_stored_in_ads";

auto constexpr LOAD_ALG_NAME = "Load";
auto constexpr NORMALISE_CURRENT_ALG_NAME = "NormaliseByCurrent";

enum class AlgorithmType { LOAD, NORMALISE };

bool isALFData(MatrixWorkspace_const_sptr const &workspace) { return workspace->getInstrument()->getName() == "ALF"; }

AlgorithmType algorithmType(MantidQt::API::IConfiguredAlgorithm_sptr &configuredAlg) {
  auto const &name = configuredAlg->algorithm()->name();
  if (name == LOAD_ALG_NAME) {
    return AlgorithmType::LOAD;
  } else if (name == NORMALISE_CURRENT_ALG_NAME) {
    return AlgorithmType::NORMALISE;
  } else {
    throw std::logic_error(std::string("ALFView error: callback from invalid algorithm ") + name);
  }
}

IAlgorithm_sptr loadAlgorithm(std::string const &filename) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create(LOAD_ALG_NAME);
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("Filename", filename);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  return alg;
}

IAlgorithm_sptr normaliseByCurrentAlgorithm(MatrixWorkspace_sptr const &inputWorkspace) {
  auto alg = AlgorithmManager::Instance().create(NORMALISE_CURRENT_ALG_NAME);
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  alg->setProperty("InputWorkspace", inputWorkspace);
  alg->setProperty("OutputWorkspace", NOT_IN_ADS);
  return alg;
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFAlgorithmManager::ALFAlgorithmManager(std::unique_ptr<API::IJobRunner> jobRunner)
    : m_jobRunner(std::move(jobRunner)), m_subscriber() {
  m_jobRunner->subscribe(this);
}

void ALFAlgorithmManager::subscribe(IALFAlgorithmManagerSubscriber *subscriber) { m_subscriber = subscriber; }

void ALFAlgorithmManager::loadAndNormalise(std::string const &filename) {
  m_jobRunner->executeAlgorithm(loadAlgorithm(filename));
}

void ALFAlgorithmManager::notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) {
  switch (algorithmType(algorithm)) {
  case AlgorithmType::LOAD:
    notifyLoadComplete(algorithm->algorithm());
    return;
  case AlgorithmType::NORMALISE:
    notifyNormaliseComplete(algorithm->algorithm());
    return;
  }
}

void ALFAlgorithmManager::notifyAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) {
  (void)algorithm;
  m_subscriber->notifyAlgorithmError(message);
}

void ALFAlgorithmManager::notifyLoadComplete(IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  Workspace_sptr loadedWorkspace = algorithm->getProperty("OutputWorkspace");
  auto workspace = std::dynamic_pointer_cast<MatrixWorkspace>(loadedWorkspace);

  if (!isALFData(workspace)) {
    throw std::invalid_argument("The loaded data is not from the ALF instrument");
  }

  m_jobRunner->executeAlgorithm(normaliseByCurrentAlgorithm(workspace));
}

void ALFAlgorithmManager::notifyNormaliseComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyLoadAndNormaliseComplete(outputWorkspace);
}

} // namespace MantidQt::CustomInterfaces