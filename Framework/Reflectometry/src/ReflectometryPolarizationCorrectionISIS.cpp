// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ReflectometryPolarizationCorrectionISIS.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SpinStateValidator.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Mantid::Reflectometry {

using namespace API;
using namespace Kernel;

namespace {
Logger g_log("ReflectometryPolarizationCorrectionISIS");

namespace Prop {
static const std::string FLIPPERS{"Flippers"};
static const std::string POLARIZATION_ANALYSIS{"PolarizationAnalysis"};
} // namespace Prop

namespace ParameterFile {
static const std::string FREDRIKZE_INPUT_SPIN_STATE_ORDER_PA{"FredrikzeInputSpinStateOrderPA"};
static const std::string FREDRIKZE_INPUT_SPIN_STATE_ORDER_PNR{"FredrikzeInputSpinStateOrderPNR"};
static const std::string WILDES_FLIPPER_CONFIG{"WildesFlipperConfig"};
} // namespace ParameterFile

namespace CorrectionMethod {
static const std::string WILDES{"Wildes"};
static const std::string FREDRIKZE{"Fredrikze"};

static const std::vector<std::string> WILDES_AXES = {"P1", "P2", "F1", "F2"};
static const std::vector<std::string> FREDRIKZE_AXES = {"Pp", "Ap", "Rho", "Alpha"};

static const std::map<std::string, std::string> OPTION_NAME{{CorrectionMethod::WILDES, Prop::FLIPPERS},
                                                            {CorrectionMethod::FREDRIKZE, Prop::POLARIZATION_ANALYSIS}};

void validate(const std::string &method) {
  if (!CorrectionMethod::OPTION_NAME.count(method)) {
    throw std::invalid_argument("Unsupported polarization correction method: " + method);
  }
}
} // namespace CorrectionMethod

namespace CorrectionOption {
static const std::string PNR{"PNR"};
static const std::string PA{"PA"};
static const std::string DEFAULT_FLIPPERS_NO_ANALYSER{"0, 1"};
static const std::string DEFAULT_FLIPPERS_FULL{"00, 01, 10, 11"};
} // namespace CorrectionOption

enum class SpinStateFamily { Fredrikze, Wildes, Invalid };

bool isWildesInputSpinStateOrder(const std::string &spinStates) {
  static const SpinStateValidator validator(std::unordered_set<int>{2, 4}, true, "0", "1", true, "", false);
  return validator.isValid(spinStates).empty();
}

bool isFredrikzeInputSpinStateOrder(const std::string &spinStates) {
  static const SpinStateValidator validator(std::unordered_set<int>{2, 4}, true, "p", "a", true, "", false);
  return validator.isValid(spinStates).empty();
}

SpinStateFamily spinStateFamily(const std::string &spinStates) {
  if (isFredrikzeInputSpinStateOrder(spinStates)) {
    return SpinStateFamily::Fredrikze;
  }
  if (isWildesInputSpinStateOrder(spinStates)) {
    return SpinStateFamily::Wildes;
  }
  return SpinStateFamily::Invalid;
}
} // namespace

ReflectometryPolarizationCorrectionISIS::ReflectometryPolarizationCorrectionISIS(
    const API::WorkspaceGroup_sptr &inputWorkspaces, ChildAlgorithmFactory childAlgorithmFactory, IsDefault isDefault,
    StringPropertyGetter getPropertyValue, MatrixWorkspacePropertyGetter getMatrixWorkspaceProperty)
    : m_inputWorkspaces(inputWorkspaces), m_childAlgorithmFactory(std::move(childAlgorithmFactory)),
      m_isDefault(std::move(isDefault)), m_getPropertyValue(std::move(getPropertyValue)),
      m_getMatrixWorkspaceProperty(std::move(getMatrixWorkspaceProperty)) {}

std::string
ReflectometryPolarizationCorrectionISIS::findCorrectionMethod(const MatrixWorkspace_sptr &efficiencies) const {
  try {
    auto const &axis = dynamic_cast<TextAxis &>(*efficiencies->getAxis(1));
    for (size_t i = 0; i < axis.length(); ++i) {
      if (std::find(CorrectionMethod::WILDES_AXES.begin(), CorrectionMethod::WILDES_AXES.end(), axis.label(i)) !=
          CorrectionMethod::WILDES_AXES.end()) {
        return CorrectionMethod::WILDES;
      }
      if (std::find(CorrectionMethod::FREDRIKZE_AXES.begin(), CorrectionMethod::FREDRIKZE_AXES.end(), axis.label(i)) !=
          CorrectionMethod::FREDRIKZE_AXES.end()) {
        return CorrectionMethod::FREDRIKZE;
      }
    }
  } catch (std::bad_cast &) {
    throw std::runtime_error("Efficiencies workspace is not in a supported format");
  }

  throw std::runtime_error(
      "Axes labels for efficiencies workspace do not match any supported polarization correction method");
}

std::string ReflectometryPolarizationCorrectionISIS::findCorrectionOption(const std::string &correctionMethod) const {
  auto numWorkspacesInGrp = m_inputWorkspaces->size();
  if (numWorkspacesInGrp != 4 && numWorkspacesInGrp != 2) {
    throw std::runtime_error("Only input workspace groups with two or four periods are supported");
  }

  if (correctionMethod == CorrectionMethod::WILDES) {
    auto const &correctionOption = std::dynamic_pointer_cast<MatrixWorkspace>(m_inputWorkspaces->getItem(0))
                                       ->getInstrument()
                                       ->getParameterAsString(ParameterFile::WILDES_FLIPPER_CONFIG);

    if (!correctionOption.empty()) {
      return correctionOption;
    }
  }

  auto const defaultOption = numWorkspacesInGrp == 2 ? CorrectionOption::DEFAULT_FLIPPERS_NO_ANALYSER
                                                     : CorrectionOption::DEFAULT_FLIPPERS_FULL;
  if (correctionMethod == CorrectionMethod::WILDES) {
    g_log.warning() << ParameterFile::WILDES_FLIPPER_CONFIG
                    << " is not specified in the instrument parameter file. Falling back to the default Wildes "
                       "flipper configuration "
                    << defaultOption << ".\n";
    return defaultOption;
  }

  if (numWorkspacesInGrp == 2) {
    return CorrectionOption::PNR;
  }
  return CorrectionOption::PA;
}

std::string
ReflectometryPolarizationCorrectionISIS::findFredrikzeInputSpinStateOrder(const std::string &correctionMethod,
                                                                          const std::string &correctionOption) const {
  if (correctionMethod != CorrectionMethod::FREDRIKZE) {
    return "";
  }

  auto const parameterName = correctionOption == CorrectionOption::PNR
                                 ? ParameterFile::FREDRIKZE_INPUT_SPIN_STATE_ORDER_PNR
                                 : ParameterFile::FREDRIKZE_INPUT_SPIN_STATE_ORDER_PA;
  auto const inputSpinStateOrder = std::dynamic_pointer_cast<MatrixWorkspace>(m_inputWorkspaces->getItem(0))
                                       ->getInstrument()
                                       ->getParameterAsString(parameterName);
  if (inputSpinStateOrder.empty()) {
    g_log.warning() << parameterName
                    << " is not specified in the instrument parameter file. Falling back to the default Fredrikze "
                       "input spin state order for "
                    << correctionOption << ".\n";
  }
  return inputSpinStateOrder;
}

std::string ReflectometryPolarizationCorrectionISIS::getCustomInputSpinStateOrder() const {
  auto const &spinStatesProp = m_getPropertyValue("PolarizationCorrectionInputSpinStateOrder");
  auto const &deprecatedSpinStatesProp = m_getPropertyValue("FredrikzePolarizationSpinStateOrder");
  if (!spinStatesProp.empty() && !deprecatedSpinStatesProp.empty() && spinStatesProp != deprecatedSpinStatesProp) {
    throw std::runtime_error("PolarizationCorrectionInputSpinStateOrder and FredrikzePolarizationSpinStateOrder cannot "
                             "both be set to different values.");
  }
  return !spinStatesProp.empty() ? spinStatesProp : deprecatedSpinStatesProp;
}

void ReflectometryPolarizationCorrectionISIS::validateInputSpinStateOrderFamily(const std::string &correctionMethod,
                                                                                const std::string &spinStates) const {
  if (spinStates.empty()) {
    return;
  }

  auto const family = spinStateFamily(spinStates);
  if (correctionMethod == CorrectionMethod::WILDES && family == SpinStateFamily::Fredrikze) {
    throw std::runtime_error("The input spin state order \"" + spinStates +
                             "\" uses Fredrikze spin states, but a Wildes polarization correction has been "
                             "selected. Use a Wildes flipper configuration such as \"00,01,10,11\" or \"0,1\".");
  }
  if (correctionMethod == CorrectionMethod::FREDRIKZE && family == SpinStateFamily::Wildes) {
    throw std::runtime_error("The input spin state order \"" + spinStates +
                             "\" uses Wildes spin states, but a Fredrikze polarization correction has been "
                             "selected. Use Fredrikze spin states such as \"pa,ap,pp,aa\" or \"p,a\".");
  }
}

ReflectometryPolarizationCorrectionISIS::Correction ReflectometryPolarizationCorrectionISIS::getCorrection() const {
  Correction correction;
  auto const customInputOrder = getCustomInputSpinStateOrder();

  if (!m_isDefault("PolarizationEfficiencies")) {
    correction.efficiencies = m_getMatrixWorkspaceProperty("PolarizationEfficiencies");
    correction.method = findCorrectionMethod(correction.efficiencies);
    correction.option = findCorrectionOption(correction.method);
    correction.fredrikzeInputSpinStateOrder = findFredrikzeInputSpinStateOrder(correction.method, correction.option);
  } else {
    Workspace_sptr workspace = m_inputWorkspaces->getItem(0);

    auto effAlg = m_childAlgorithmFactory("ExtractPolarizationEfficiencies");
    effAlg->setProperty("InputWorkspace", workspace);
    effAlg->execute();
    correction.efficiencies = effAlg->getProperty("OutputWorkspace");
    correction.method = effAlg->getPropertyValue("CorrectionMethod");
    correction.option = effAlg->getPropertyValue("CorrectionOption");
    correction.fredrikzeInputSpinStateOrder = findFredrikzeInputSpinStateOrder(correction.method, correction.option);
  }

  if (!customInputOrder.empty()) {
    validateInputSpinStateOrderFamily(correction.method, customInputOrder);
    if (correction.method == CorrectionMethod::WILDES) {
      correction.option = customInputOrder;
    } else {
      correction.fredrikzeInputSpinStateOrder = customInputOrder;
    }
  }
  validateInputSpinStateOrderFamily(correction.method, correction.method == CorrectionMethod::WILDES
                                                           ? correction.option
                                                           : correction.fredrikzeInputSpinStateOrder);

  return correction;
}

API::WorkspaceGroup_sptr ReflectometryPolarizationCorrectionISIS::apply(const std::string &outputGroupName) const {
  auto correction = getCorrection();
  CorrectionMethod::validate(correction.method);

  auto polAlg = m_childAlgorithmFactory("PolarizationEfficiencyCor");
  polAlg->setAlwaysStoreInADS(true);
  polAlg->setRethrows(true);
  polAlg->setProperty("OutputWorkspace", outputGroupName);
  polAlg->setProperty("Efficiencies", correction.efficiencies);
  polAlg->setProperty("CorrectionMethod", correction.method);
  polAlg->setProperty("SpinStatesInFredrikze", correction.fredrikzeInputSpinStateOrder);
  polAlg->setProperty(CorrectionMethod::OPTION_NAME.at(correction.method), correction.option);
  polAlg->setProperty("AddSpinStateToLog", true);
  polAlg->setProperty("InputWorkspaceGroup", m_inputWorkspaces);
  polAlg->execute();
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputGroupName);
}

} // namespace Mantid::Reflectometry
