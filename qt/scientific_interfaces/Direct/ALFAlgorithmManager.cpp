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

auto constexpr CONVERT_UNITS_ALG_NAME = "ConvertUnits";
auto constexpr LOAD_ALG_NAME = "Load";
auto constexpr NORMALISE_CURRENT_ALG_NAME = "NormaliseByCurrent";
auto constexpr REBIN_TO_WORKSPACE_ALG_NAME = "RebinToWorkspace";
auto constexpr REPLACE_SPECIAL_VALUES_ALG_NAME = "ReplaceSpecialValues";

enum class AlgorithmType { LOAD, NORMALISE, REBIN, REPLACE_SPECIAL, CONVERT_UNITS };

bool isALFData(MatrixWorkspace_const_sptr const &workspace) { return workspace->getInstrument()->getName() == "ALF"; }

AlgorithmType algorithmType(MantidQt::API::IConfiguredAlgorithm_sptr &configuredAlg) {
  auto const &name = configuredAlg->algorithm()->name();
  if (name == LOAD_ALG_NAME) {
    return AlgorithmType::LOAD;
  } else if (name == NORMALISE_CURRENT_ALG_NAME) {
    return AlgorithmType::NORMALISE;
  } else if (name == REBIN_TO_WORKSPACE_ALG_NAME) {
    return AlgorithmType::REBIN;
  } else if (name == REPLACE_SPECIAL_VALUES_ALG_NAME) {
    return AlgorithmType::REPLACE_SPECIAL;
  } else if (name == CONVERT_UNITS_ALG_NAME) {
    return AlgorithmType::CONVERT_UNITS;
  } else {
    throw std::logic_error(std::string("ALFView error: callback from invalid algorithm ") + name);
  }
}

IAlgorithm_sptr createAlgorithm(std::string const &algorithmName) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create(algorithmName);
  alg->initialize();
  alg->setAlwaysStoreInADS(false);
  return alg;
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

void ALFAlgorithmManager::rebinToWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(REBIN_TO_WORKSPACE_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::replaceSpecialValues(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(REPLACE_SPECIAL_VALUES_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::convertUnits(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(CONVERT_UNITS_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr &algorithm) {
  switch (algorithmType(algorithm)) {
  case AlgorithmType::LOAD:
    notifyLoadComplete(algorithm->algorithm());
    return;
  case AlgorithmType::NORMALISE:
    notifyNormaliseComplete(algorithm->algorithm());
    return;
  case AlgorithmType::REBIN:
    notifyRebinToWorkspaceComplete(algorithm->algorithm());
    return;
  case AlgorithmType::REPLACE_SPECIAL:
    notifyReplaceSpecialValuesComplete(algorithm->algorithm());
    return;
  case AlgorithmType::CONVERT_UNITS:
    notifyConvertUnitsComplete(algorithm->algorithm());
    return;
  }
}

void ALFAlgorithmManager::notifyAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) {
  (void)algorithm;
  m_subscriber->notifyAlgorithmError(message);
}

void ALFAlgorithmManager::executeAlgorithm(Mantid::API::IAlgorithm_sptr const &algorithm,
                                           std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  auto configuredAlg = std::make_shared<API::ConfiguredAlgorithm>(std::move(algorithm), std::move(properties));
  m_jobRunner->executeAlgorithm(configuredAlg);
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

void ALFAlgorithmManager::notifyRebinToWorkspaceComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyRebinToWorkspaceComplete(outputWorkspace);
}

void ALFAlgorithmManager::notifyReplaceSpecialValuesComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyReplaceSpecialValuesComplete(outputWorkspace);
}

void ALFAlgorithmManager::notifyConvertUnitsComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyConvertUnitsComplete(outputWorkspace);
}

} // namespace MantidQt::CustomInterfaces