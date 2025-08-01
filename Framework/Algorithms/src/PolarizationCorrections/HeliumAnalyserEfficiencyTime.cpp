// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiencyTime.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidTypes/Core/DateAndTime.h"
#include <MantidKernel/LambdaValidator.h>
#include <MantidKernel/UnitFactory.h>
#include <algorithm>
#include <vector>

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(HeliumAnalyserEfficiencyTime)

using namespace Kernel;
using namespace API;

namespace PropertyNames {
static const std::string INPUT_WORKSPACE{"InputWorkspace"};
static const std::string REFERENCE_WORKSPACE{"ReferenceWorkspace"};
static const std::string REFERENCE_TIMESTAMP{"ReferenceTimeStamp"};
static const std::string OUTPUT_WORKSPACE{"OutputWorkspace"};
static const std::string PXD{"PXD"};
static const std::string PXD_ERROR{"PXDError"};
static const std::string LIFETIME{"Lifetime"};
static const std::string LIFETIME_ERROR{"LifetimeError"};
static const std::string INITIAL_POL{"InitialPolarization"};
static const std::string INITIAL_POL_ERROR{"InitialPolarizationError"};
} // namespace PropertyNames

namespace {
auto constexpr LAMBDA_CONVERSION_FACTOR = 0.0733;

bool hasUnit(const std::string &unitToCompareWith, const MatrixWorkspace_sptr &ws) {
  if (ws->axes() == 0) {
    return false;
  }
  const auto unit = ws->getAxis(0)->unit();
  return (unit && unit->unitID() == unitToCompareWith);
}

bool hasTimeLogs(const MatrixWorkspace_sptr &ws) {
  const Run &run = ws->run();
  const bool hasStart = run.hasProperty("start_time") || run.hasProperty("run_start");
  const bool hasEnd = run.hasProperty("end_time") || run.hasProperty("run_end");
  return hasStart && hasEnd;
}

bool checkValidMatrixWorkspace(const Workspace_sptr &ws) {
  const auto &ws_input = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  return (ws_input && hasUnit("Wavelength", ws_input) && hasTimeLogs(ws_input));
}

std::string validateWorkspaceWithProperties(const Workspace_sptr &ws) {
  if (!ws) {
    return "Workspace has to be a valid workspace";
  }

  if (ws->isGroup()) {
    const auto &groupItems = std::dynamic_pointer_cast<WorkspaceGroup>(ws)->getAllItems();
    if (std::any_of(groupItems.cbegin(), groupItems.cend(),
                    [](const auto &childWs) { return !checkValidMatrixWorkspace(childWs); })) {
      return "Workspace must have time logs and Wavelength units";
    }
    return "";
  }

  if (!checkValidMatrixWorkspace(ws)) {
    return "Workspace must have time logs and Wavelength units";
  }
  return "";
}
} // namespace

void HeliumAnalyserEfficiencyTime::init() {
  auto wkpsValidator = std::make_shared<LambdaValidator<Workspace_sptr>>(validateWorkspaceWithProperties);
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input,
                                                                 wkpsValidator),
                  "Scattering Workspace from which to extract the experiment timestamp");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      PropertyNames::REFERENCE_WORKSPACE, "", Direction::Input, PropertyMode::Optional, wkpsValidator),
                  "Reference workspace for which to extract the reference timestamp and wavelength range");
  declareProperty(PropertyNames::REFERENCE_TIMESTAMP, "", std::make_shared<DateTimeValidator>(true),
                  "An ISO formatted date/time string specifying reference timestamp with respect to the scattering "
                  "workspace start time, e.g 2010-09-14T04:20:12",
                  Direction::Input);

  const auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::PXD, 12.0, mustBePositive, "Gas pressure in bar multiplied by cell length in metres");
  declareProperty(PropertyNames::PXD_ERROR, 0.0, mustBePositive, "Error in pxd");
  declareProperty(PropertyNames::INITIAL_POL, 0.9, mustBePositive, "Initial Polarization of He Gas in cell");
  declareProperty(PropertyNames::INITIAL_POL_ERROR, 0.0, mustBePositive, "Error in initial polarization");
  declareProperty(PropertyNames::LIFETIME, 45.0, mustBePositive,
                  "Lifetime of polarization decay of He gas in cell (in hours)");
  declareProperty(PropertyNames::LIFETIME_ERROR, 0.0, mustBePositive, "Error in lifetime (in hours)");
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
                  "Helium analyzer efficiency as a function of wavelength");
}

/**
 * Tests that the inputs are all valid
 * @return A map containing the incorrect workspace
 * properties and an error message
 */
std::map<std::string, std::string> HeliumAnalyserEfficiencyTime::validateInputs() {
  std::map<std::string, std::string> errorList;
  if (isDefault(PropertyNames::REFERENCE_WORKSPACE) && isDefault(PropertyNames::REFERENCE_TIMESTAMP)) {
    errorList[PropertyNames::REFERENCE_WORKSPACE] =
        "Both ReferenceWorkspace and ReferenceTimeStamp properties are empty, at least one of the two has to be "
        "supplied to execute the Algorithm";
  }

  return errorList;
}

void HeliumAnalyserEfficiencyTime::exec() {
  const auto outWs = calculateEfficiency();
  setProperty(PropertyNames::OUTPUT_WORKSPACE, outWs);
}

MatrixWorkspace_sptr HeliumAnalyserEfficiencyTime::retrieveWorkspaceForWavelength() const {
  const Workspace_sptr inputWs = isDefault(PropertyNames::REFERENCE_WORKSPACE)
                                     ? getProperty(PropertyNames::INPUT_WORKSPACE)
                                     : getProperty(PropertyNames::REFERENCE_WORKSPACE);
  if (inputWs->isGroup()) {
    // We get first index of workspace (all members of the group should have the same wavelength range for polarized
    // run)
    const auto ws = std::dynamic_pointer_cast<WorkspaceGroup>(inputWs);
    return std::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(0));
  }
  return std::dynamic_pointer_cast<MatrixWorkspace>(inputWs);
}

MatrixWorkspace_sptr HeliumAnalyserEfficiencyTime::calculateEfficiency() {
  const auto [time, timeError] = getTimeDifference();
  const double mu = LAMBDA_CONVERSION_FACTOR * static_cast<double>(getProperty(PropertyNames::PXD));
  const double muError = LAMBDA_CONVERSION_FACTOR * static_cast<double>(getProperty(PropertyNames::PXD_ERROR));
  const double lifetime = getProperty(PropertyNames::LIFETIME);
  const double lifetimeError = getProperty(PropertyNames::LIFETIME_ERROR);
  const double polIni = getProperty(PropertyNames::INITIAL_POL);
  const double polIniError = getProperty(PropertyNames::INITIAL_POL_ERROR);

  const MatrixWorkspace_sptr inputWs = retrieveWorkspaceForWavelength();
  const auto &pointData = inputWs->histogram(0).points();
  const auto &lambdas = pointData.rawData();
  const auto &binBoundaries = inputWs->x(0);

  auto efficiency = std::vector<double>(lambdas.size());
  auto efficiencyErrors = std::vector<double>(lambdas.size());
  for (size_t index = 0; index < lambdas.size(); index++) {
    const auto lambdaError = binBoundaries[index + 1] - binBoundaries[index];
    // Efficiency
    const auto lambda = lambdas.at(index);
    const auto expTerm = std::exp(-time / lifetime);
    const auto factor = mu * lambda * polIni * expTerm;
    efficiency.at(index) = (1 + std::tanh(factor)) / 2;

    // Calculate the errors for the efficiency, covariance between variables assumed to be zero
    const auto commonTerm = 0.5 / std::pow(std::cosh(factor), 2);
    const double de_dmu = commonTerm * lambda * polIni * expTerm;
    const double de_dlambda = commonTerm * mu * polIni * expTerm;
    const double de_dpolIni = commonTerm * mu * lambda * expTerm;
    const double de_dt = -commonTerm * factor / lifetime;
    const double de_lifetime = commonTerm * factor * time / (lifetime * lifetime);
    efficiencyErrors.at(index) =
        std::sqrt(de_dmu * de_dmu * muError * muError + de_dlambda * de_dlambda * lambdaError * lambdaError +
                  de_dpolIni * de_dpolIni * polIniError * polIniError + de_dt * de_dt * timeError * timeError +
                  de_lifetime * de_lifetime * lifetimeError * lifetimeError);
  }

  const auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, efficiency.size() + 1, efficiency.size());
  ws->mutableX(0) = binBoundaries;
  ws->mutableY(0) = HistogramData::HistogramY(efficiency);
  ws->mutableE(0) = HistogramData::HistogramE(efficiencyErrors);
  ws->setDistribution(true);
  ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
  return ws;
}

std::pair<double, double> HeliumAnalyserEfficiencyTime::getTimeDifference() {
  //  The reference workspace takes precedence in case a timestamp is also provided.
  const auto timeDiff = createChildAlgorithm("TimeDifference");
  timeDiff->initialize();
  timeDiff->setProperty("InputWorkspaces", getPropertyValue("InputWorkspace"));

  size_t rowTime = 0;
  size_t colTime = 1;
  constexpr size_t colTimeError = 5;
  std::string refTimeStamp;
  if (!isDefault(PropertyNames::REFERENCE_WORKSPACE)) {
    timeDiff->setProperty("ReferenceWorkspace", getPropertyValue("ReferenceWorkspace"));
    rowTime = 1;
    colTime = 4;
  } else {
    refTimeStamp = getPropertyValue(PropertyNames::REFERENCE_TIMESTAMP);
  }

  timeDiff->execute();
  const ITableWorkspace_sptr table = timeDiff->getProperty("OutputWorkspace");
  double tHours;
  if (!refTimeStamp.empty()) {
    // Here we can only take the time stamp of the input workspace from the table
    const auto expTimeStamp = table->cell<std::string>(rowTime, colTime);
    const auto duration = Types::Core::DateAndTime(expTimeStamp) - Types::Core::DateAndTime(refTimeStamp);
    tHours = static_cast<double>(duration.total_seconds() / 3600);
  } else {
    tHours = static_cast<double>(table->cell<float>(rowTime, colTime));
  }
  const auto tHoursErr = static_cast<double>(table->cell<float>(rowTime, colTimeError));
  return std::make_pair(std::abs(tHours), tHoursErr);
}

} // namespace Mantid::Algorithms
