// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAlgorithmManager.h"
#include "IALFAlgorithmManagerSubscriber.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"

using namespace Mantid::API;

namespace {
auto constexpr CONVERT_UNITS_ALG_NAME = "ConvertUnits";
auto constexpr CREATE_WORKSPACE_ALG_NAME = "CreateWorkspace";
auto constexpr CROP_WORKSPACE_ALG_NAME = "CropWorkspace";
auto constexpr DIVIDE_ALG_NAME = "Divide";
auto constexpr FIT_ALG_NAME = "Fit";
auto constexpr LOAD_ALG_NAME = "Load";
auto constexpr NORMALISE_CURRENT_ALG_NAME = "NormaliseByCurrent";
auto constexpr REBIN_TO_WORKSPACE_ALG_NAME = "RebinToWorkspace";
auto constexpr REPLACE_SPECIAL_VALUES_ALG_NAME = "ReplaceSpecialValues";
auto constexpr REBUNCH_ALG_NAME = "Rebunch";
auto constexpr SCALE_X_ALG_NAME = "ScaleX";

enum class AlgorithmType {
  LOAD,
  NORMALISE,
  REBIN,
  DIVIDE,
  REPLACE_SPECIAL,
  CONVERT_UNITS,
  CREATE_WORKSPACE,
  SCALE_X,
  REBUNCH,
  CROP_WORKSPACE,
  FIT
};

AlgorithmType algorithmType(MantidQt::API::IConfiguredAlgorithm_sptr &configuredAlg) {
  auto const &name = configuredAlg->algorithm()->name();
  if (name == LOAD_ALG_NAME) {
    return AlgorithmType::LOAD;
  } else if (name == NORMALISE_CURRENT_ALG_NAME) {
    return AlgorithmType::NORMALISE;
  } else if (name == REBIN_TO_WORKSPACE_ALG_NAME) {
    return AlgorithmType::REBIN;
  } else if (name == DIVIDE_ALG_NAME) {
    return AlgorithmType::DIVIDE;
  } else if (name == REPLACE_SPECIAL_VALUES_ALG_NAME) {
    return AlgorithmType::REPLACE_SPECIAL;
  } else if (name == CONVERT_UNITS_ALG_NAME) {
    return AlgorithmType::CONVERT_UNITS;
  } else if (name == CREATE_WORKSPACE_ALG_NAME) {
    return AlgorithmType::CREATE_WORKSPACE;
  } else if (name == SCALE_X_ALG_NAME) {
    return AlgorithmType::SCALE_X;
  } else if (name == REBUNCH_ALG_NAME) {
    return AlgorithmType::REBUNCH;
  } else if (name == CROP_WORKSPACE_ALG_NAME) {
    return AlgorithmType::CROP_WORKSPACE;
  } else if (name == FIT_ALG_NAME) {
    return AlgorithmType::FIT;
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

} // namespace

namespace MantidQt::CustomInterfaces {

ALFAlgorithmManager::ALFAlgorithmManager(std::unique_ptr<API::IJobRunner> jobRunner)
    : m_jobRunner(std::move(jobRunner)), m_subscriber() {
  m_jobRunner->subscribe(this);
}

void ALFAlgorithmManager::subscribe(IALFAlgorithmManagerSubscriber *subscriber) { m_subscriber = subscriber; }

void ALFAlgorithmManager::load(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(LOAD_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::normaliseByCurrent(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(NORMALISE_CURRENT_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::rebinToWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(REBIN_TO_WORKSPACE_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::divide(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(DIVIDE_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::replaceSpecialValues(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(REPLACE_SPECIAL_VALUES_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::convertUnits(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(CONVERT_UNITS_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::createWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(CREATE_WORKSPACE_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::scaleX(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(SCALE_X_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::rebunch(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(REBUNCH_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::cropWorkspace(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  executeAlgorithm(createAlgorithm(CROP_WORKSPACE_ALG_NAME), std::move(properties));
}

void ALFAlgorithmManager::fit(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  auto fitAlgorithm = createAlgorithm(FIT_ALG_NAME);

  // A quirk of the Fit algorithm means we need to set the properties now.
  // The Fit algorithm is different to most other algorithms due to having additional
  // properties which only exist depending on the number of domains the provided function has
  Mantid::API::IFunction_sptr function = properties->getProperty("Function");
  Mantid::API::Workspace_sptr input = properties->getProperty("InputWorkspace");
  bool createOutput = properties->getProperty("CreateOutput");
  double startX = properties->getProperty("StartX");
  double endX = properties->getProperty("EndX");

  fitAlgorithm->setProperty("Function", function);
  fitAlgorithm->setProperty("InputWorkspace", input);
  fitAlgorithm->setProperty("CreateOutput", createOutput);
  fitAlgorithm->setProperty("StartX", startX);
  fitAlgorithm->setProperty("EndX", endX);

  executeAlgorithm(fitAlgorithm, std::make_unique<AlgorithmRuntimeProps>());
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
  case AlgorithmType::DIVIDE:
    notifyDivideComplete(algorithm->algorithm());
    return;
  case AlgorithmType::REPLACE_SPECIAL:
    notifyReplaceSpecialValuesComplete(algorithm->algorithm());
    return;
  case AlgorithmType::CONVERT_UNITS:
    notifyConvertUnitsComplete(algorithm->algorithm());
    return;
  case AlgorithmType::CREATE_WORKSPACE:
    notifyCreateWorkspaceComplete(algorithm->algorithm());
    return;
  case AlgorithmType::SCALE_X:
    notifyScaleXComplete(algorithm->algorithm());
    return;
  case AlgorithmType::REBUNCH:
    notifyRebunchComplete(algorithm->algorithm());
    return;
  case AlgorithmType::CROP_WORKSPACE:
    notifyCropWorkspaceComplete(algorithm->algorithm());
    return;
  case AlgorithmType::FIT:
    notifyFitComplete(algorithm->algorithm());
    return;
  }
}

void ALFAlgorithmManager::executeAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                                           std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties) {
  API::IConfiguredAlgorithm_sptr configuredAlg =
      std::make_shared<API::ConfiguredAlgorithm>(std::move(algorithm), std::move(properties));
  m_jobRunner->executeAlgorithm(std::move(configuredAlg));
}

void ALFAlgorithmManager::notifyAlgorithmError(API::IConfiguredAlgorithm_sptr &algorithm, std::string const &message) {
  (void)algorithm;
  m_subscriber->notifyAlgorithmError(message);
}

void ALFAlgorithmManager::notifyLoadComplete(IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  Workspace_sptr workspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyLoadComplete(std::dynamic_pointer_cast<MatrixWorkspace>(workspace));
}

void ALFAlgorithmManager::notifyNormaliseComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyNormaliseByCurrentComplete(outputWorkspace);
}

void ALFAlgorithmManager::notifyRebinToWorkspaceComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyRebinToWorkspaceComplete(outputWorkspace);
}

void ALFAlgorithmManager::notifyDivideComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyDivideComplete(outputWorkspace);
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

void ALFAlgorithmManager::notifyCreateWorkspaceComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyCreateWorkspaceComplete(outputWorkspace);
}

void ALFAlgorithmManager::notifyScaleXComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyScaleXComplete(outputWorkspace);
}

void ALFAlgorithmManager::notifyRebunchComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyRebunchComplete(outputWorkspace);
}

void ALFAlgorithmManager::notifyCropWorkspaceComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  m_subscriber->notifyCropWorkspaceComplete(outputWorkspace);
}

void ALFAlgorithmManager::notifyFitComplete(Mantid::API::IAlgorithm_sptr const &algorithm) {
  // Explicitly provide return type. Return type must be the same as the input property type to allow type casting
  MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
  IFunction_sptr function = algorithm->getProperty("Function");
  std::string fitStatus = algorithm->getPropertyValue("OutputStatus");

  m_subscriber->notifyFitComplete(std::move(outputWorkspace), std::move(function), std::move(fitStatus));
}

} // namespace MantidQt::CustomInterfaces
