// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/FitSpecularPeak.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DynamicPointerCastHelper.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
namespace Prop {
std::string const BACKGROUND_TYPE{"BackgroundType"};
std::string const END_INDEX{"EndWorkspaceIndex"};
std::string const INPUT_WS{"InputWorkspace"};
std::string const OUTPUT_FIT_WS{"OutputFitWorkspace"};
std::string const OUTPUT_PROFILE_WS{"OutputProfileWorkspace"};
std::string const OUTPUT_STATUS{"OutputStatus"};
std::string const PEAK_CENTRE{"PeakCentre"};
std::string const PEAK_CENTRE_ERROR{"PeakCentreError"};
std::string const RANGE_LOWER{"RangeLower"};
std::string const RANGE_UPPER{"RangeUpper"};
std::string const START_INDEX{"StartWorkspaceIndex"};
} // namespace Prop

std::string const LINEAR_BACKGROUND{"Linear"};
std::string const FLAT_BACKGROUND{"Flat"};
std::string const FALLBACK_STATUS{"Fit failed; using initial peak centre"};

// Give every integrated spectrum common bin edges so that Transpose accepts the workspace. These edges become the
// unused vertical axis of the detector profile and do not affect the fitted workspace-index coordinates.
void setCommonBinEdgesForTranspose(Mantid::API::MatrixWorkspace &workspace) {
  for (size_t index = 0; index < workspace.getNumberHistograms(); ++index) {
    auto &x = workspace.mutableX(index);
    x.front() = 0.0;
    x.back() = 1.0;
  }
}

double median(const Mantid::HistogramData::HistogramY &y) {
  auto finiteValues = std::vector<double>{};
  finiteValues.reserve(y.size());
  std::copy_if(y.cbegin(), y.cend(), std::back_inserter(finiteValues),
               [](double const value) { return std::isfinite(value); });
  if (finiteValues.empty()) {
    throw std::runtime_error("FitSpecularPeak could not identify an initial peak centre.");
  }
  std::sort(finiteValues.begin(), finiteValues.end());
  auto const middle = finiteValues.size() / 2;
  return finiteValues.size() % 2 == 0 ? 0.5 * (finiteValues[middle - 1] + finiteValues[middle]) : finiteValues[middle];
}

struct PeakParameters {
  double centre;
  double height;
  std::optional<double> fwhm;
};

PeakParameters estimatePeak(const Mantid::API::MatrixWorkspace &profile, double const background) {
  auto const &x = profile.x(0);
  auto const &y = profile.y(0);
  auto maxIndex = std::optional<size_t>{};
  for (size_t index = 0; index < y.size(); ++index) {
    if (std::isfinite(y[index]) && (!maxIndex || y[index] > y[*maxIndex])) {
      maxIndex = index;
    }
  }
  if (!maxIndex) {
    throw std::runtime_error("FitSpecularPeak could not identify an initial peak centre.");
  }

  auto const height = y[*maxIndex] - background;
  if (height <= 0.0) {
    return {x[*maxIndex], 0.0, std::nullopt};
  }

  auto const halfHeight = background + 0.5 * height;
  auto left = std::optional<size_t>{};
  for (size_t index = *maxIndex; index > 0; --index) {
    if (std::isfinite(y[index - 1]) && y[index - 1] < halfHeight) {
      left = index - 1;
      break;
    }
  }
  auto right = std::optional<size_t>{};
  for (size_t index = *maxIndex + 1; index < y.size(); ++index) {
    if (std::isfinite(y[index]) && y[index] < halfHeight) {
      right = index;
      break;
    }
  }

  auto fwhm = std::optional<double>{};
  if (left && right) {
    auto const width = x[*right] - x[*left];
    if (width > 0.0) {
      fwhm = width;
    }
  }
  return {x[*maxIndex], height, fwhm};
}

bool fitConverged(const std::string &status) {
  return status == Mantid::API::MinimizerStatus::SUCCESS ||
         status == Mantid::API::MinimizerStatus::CHANGES_IN_FUNCTION_TOO_SMALL ||
         status == Mantid::API::MinimizerStatus::CHANGES_IN_PARAMETER_TOO_SMALL;
}
} // namespace

namespace Mantid::Reflectometry {

DECLARE_ALGORITHM(FitSpecularPeak)

const std::string FitSpecularPeak::name() const { return "FitSpecularPeak"; }

int FitSpecularPeak::version() const { return 1; }

const std::string FitSpecularPeak::category() const { return "Reflectometry"; }

const std::string FitSpecularPeak::summary() const {
  return "Integrates a reflectometry detector workspace and fits a background and Gaussian to locate the specular "
         "peak.";
}

const std::vector<std::string> FitSpecularPeak::seeAlso() const { return {"FindReflectometryLines", "FindPeaks"}; }

void FitSpecularPeak::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::INPUT_WS, "", Kernel::Direction::Input),
      "A reflectometry workspace containing detector spectra.");

  auto nonNegative = std::make_shared<Kernel::BoundedValidator<int>>();
  nonNegative->setLower(0);
  declareProperty(Prop::START_INDEX, 0, nonNegative,
                  "Workspace index of the first spectrum to include in the detector profile.");
  declareProperty(Prop::END_INDEX, EMPTY_INT(), nonNegative,
                  "Workspace index of the last spectrum to include in the detector profile.");
  declareProperty(Prop::RANGE_LOWER, EMPTY_DBL(), "Lower X limit used when integrating each spectrum.");
  declareProperty(Prop::RANGE_UPPER, EMPTY_DBL(), "Upper X limit used when integrating each spectrum.");

  auto const backgrounds = std::vector<std::string>{LINEAR_BACKGROUND, FLAT_BACKGROUND};
  declareProperty(Prop::BACKGROUND_TYPE, LINEAR_BACKGROUND, std::make_shared<Kernel::StringListValidator>(backgrounds),
                  "Background function fitted with the Gaussian. Choose Linear or Flat.");

  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      Prop::OUTPUT_PROFILE_WS, "", Kernel::Direction::Output, API::PropertyMode::Optional),
                  "The integrated detector profile used for peak fitting, with X values corresponding to input "
                  "workspace indices.");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      Prop::OUTPUT_FIT_WS, "", Kernel::Direction::Output, API::PropertyMode::Optional),
                  "The Fit output containing the data, fitted curve, and residuals. Not set when the fit fails.");
  declareProperty(Prop::PEAK_CENTRE, EMPTY_DBL(), "The fractional workspace index of the specular peak.",
                  Kernel::Direction::Output);
  declareProperty(Prop::PEAK_CENTRE_ERROR, EMPTY_DBL(), "The uncertainty in the optimized peak centre.",
                  Kernel::Direction::Output);
  declareProperty(Prop::OUTPUT_STATUS, std::string{}, "Whether the optimized or initial peak centre was returned.",
                  Kernel::Direction::Output);
}

std::map<std::string, std::string> FitSpecularPeak::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_sptr workspace = getProperty(Prop::INPUT_WS);
  // Direct validation can receive a WorkspaceGroup before the framework dispatches its members.
  if (!workspace) {
    return issues;
  }

  int const startIndexProperty = getProperty(Prop::START_INDEX);
  auto const startIndex = static_cast<size_t>(startIndexProperty);
  if (startIndex >= workspace->getNumberHistograms()) {
    issues[Prop::START_INDEX] = "The index must be smaller than the number of spectra in the input workspace.";
  }
  if (!isDefault(Prop::END_INDEX)) {
    int const endIndexProperty = getProperty(Prop::END_INDEX);
    auto const endIndex = static_cast<size_t>(endIndexProperty);
    if (endIndex >= workspace->getNumberHistograms()) {
      issues[Prop::END_INDEX] = "The index must be smaller than the number of spectra in the input workspace.";
    } else if (startIndex > endIndex) {
      issues[Prop::END_INDEX] = "The index must not be smaller than StartWorkspaceIndex.";
    }
  }
  if (!isDefault(Prop::RANGE_LOWER) && !isDefault(Prop::RANGE_UPPER)) {
    double const lower = getProperty(Prop::RANGE_LOWER);
    double const upper = getProperty(Prop::RANGE_UPPER);
    if (lower >= upper) {
      issues[Prop::RANGE_UPPER] = "RangeUpper must be greater than RangeLower.";
    }
  }
  return issues;
}

API::MatrixWorkspace_sptr FitSpecularPeak::createProfile(const API::MatrixWorkspace_sptr &inputWorkspace) {
  auto integration = createChildAlgorithm("Integration");
  integration->setProperty("InputWorkspace", inputWorkspace);
  integration->setProperty("OutputWorkspace", "__unused_fit_specular_peak");
  int const startIndexProperty = getProperty(Prop::START_INDEX);
  integration->setProperty("StartWorkspaceIndex", startIndexProperty);
  if (!isDefault(Prop::END_INDEX)) {
    int const endIndexProperty = getProperty(Prop::END_INDEX);
    integration->setProperty("EndWorkspaceIndex", endIndexProperty);
  }
  if (!isDefault(Prop::RANGE_LOWER)) {
    integration->setProperty("RangeLower", static_cast<double>(getProperty(Prop::RANGE_LOWER)));
  }
  if (!isDefault(Prop::RANGE_UPPER)) {
    integration->setProperty("RangeUpper", static_cast<double>(getProperty(Prop::RANGE_UPPER)));
  }
  integration->execute();
  API::MatrixWorkspace_sptr integratedWorkspace = integration->getProperty("OutputWorkspace");
  setCommonBinEdgesForTranspose(*integratedWorkspace);

  auto transpose = createChildAlgorithm("Transpose");
  transpose->setProperty("InputWorkspace", integratedWorkspace);
  transpose->setProperty("OutputWorkspace", "__unused_fit_specular_peak");
  transpose->execute();
  API::MatrixWorkspace_sptr profileWorkspace = transpose->getProperty("OutputWorkspace");

  auto &x = profileWorkspace->mutableX(0);
  auto const firstWorkspaceIndex = static_cast<double>(startIndexProperty);
  for (size_t index = 0; index < x.size(); ++index) {
    x[index] = firstWorkspaceIndex + static_cast<double>(index);
  }
  return profileWorkspace;
}

void FitSpecularPeak::exec() {
  API::MatrixWorkspace_sptr inputWorkspace = getProperty(Prop::INPUT_WS);
  auto profileWorkspace = createProfile(inputWorkspace);
  if (!isDefault(Prop::OUTPUT_PROFILE_WS)) {
    setProperty(Prop::OUTPUT_PROFILE_WS, profileWorkspace);
  }

  auto const backgroundType = getPropertyValue(Prop::BACKGROUND_TYPE);
  auto const backgroundLevel = median(profileWorkspace->y(0));
  auto const initialPeak = estimatePeak(*profileWorkspace, backgroundLevel);
  if (!initialPeak.fwhm) {
    g_log.warning() << "Could not estimate the specular peak width. Using the initial peak centre.\n";
    setProperty(Prop::PEAK_CENTRE, initialPeak.centre);
    setProperty(Prop::OUTPUT_STATUS, FALLBACK_STATUS);
    return;
  }

  auto function = API::FunctionFactory::Instance().createFunction("CompositeFunction");
  auto composite =
      Kernel::DynamicPointerCastHelper::dynamicPointerCastWithCheck<API::CompositeFunction, API::IFunction>(function);
  function = API::FunctionFactory::Instance().createFunction("Gaussian");
  auto gaussian =
      Kernel::DynamicPointerCastHelper::dynamicPointerCastWithCheck<API::IPeakFunction, API::IFunction>(function);
  gaussian->setCentre(initialPeak.centre);
  gaussian->setFwhm(*initialPeak.fwhm);
  gaussian->setHeight(initialPeak.height);
  composite->addFunction(gaussian);

  auto backgroundFunction = API::FunctionFactory::Instance().createFunction(
      backgroundType == FLAT_BACKGROUND ? "FlatBackground" : "LinearBackground");
  backgroundFunction->setParameter("A0", backgroundLevel);
  if (backgroundType == LINEAR_BACKGROUND) {
    backgroundFunction->setParameter("A1", 0.0);
  }
  composite->addFunction(std::move(backgroundFunction));

  auto fit = createChildAlgorithm("Fit");
  fit->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(composite));
  fit->setProperty("InputWorkspace", profileWorkspace);
  fit->setProperty("WorkspaceIndex", 0);
  fit->setProperty("StartX", initialPeak.centre - 3.0 * *initialPeak.fwhm);
  fit->setProperty("EndX", initialPeak.centre + 3.0 * *initialPeak.fwhm);
  fit->setProperty("IgnoreInvalidData", true);
  fit->setProperty("CalcErrors", true);
  if (!isDefault(Prop::OUTPUT_FIT_WS)) {
    fit->setProperty("Output", "__unused_fit_specular_peak");
  }

  try {
    fit->execute();
  } catch (std::exception const &error) {
    g_log.warning() << "Specular peak fit failed: " << error.what() << ". Using the initial peak centre.\n";
    setProperty(Prop::PEAK_CENTRE, initialPeak.centre);
    setProperty(Prop::OUTPUT_STATUS, FALLBACK_STATUS);
    return;
  }

  std::string const fitStatus = fit->getProperty("OutputStatus");
  auto const fittedCentre = gaussian->centre();
  auto const &profileX = profileWorkspace->x(0);
  if (!fitConverged(fitStatus) || !std::isfinite(fittedCentre) || fittedCentre < profileX.front() ||
      fittedCentre > profileX.back()) {
    g_log.warning() << "Specular peak fit was not successful. Using the initial peak centre.\n";
    setProperty(Prop::PEAK_CENTRE, initialPeak.centre);
    setProperty(Prop::OUTPUT_STATUS, FALLBACK_STATUS);
    return;
  }

  setProperty(Prop::PEAK_CENTRE, fittedCentre);
  auto const centreError = gaussian->getError(gaussian->parameterIndex("PeakCentre"));
  if (std::isfinite(centreError)) {
    setProperty(Prop::PEAK_CENTRE_ERROR, centreError);
  }
  setProperty(Prop::OUTPUT_STATUS, Mantid::API::MinimizerStatus::SUCCESS);
  if (!isDefault(Prop::OUTPUT_FIT_WS)) {
    API::MatrixWorkspace_sptr fitWorkspace = fit->getProperty("OutputWorkspace");
    setProperty(Prop::OUTPUT_FIT_WS, fitWorkspace);
  }
}

} // namespace Mantid::Reflectometry
