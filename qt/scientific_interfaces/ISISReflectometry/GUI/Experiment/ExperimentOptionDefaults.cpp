// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ExperimentOptionDefaults.h"

#include <utility>

#include "Common/OptionDefaults.h"
#include "LookupTableValidator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "Reduction/Experiment.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");

std::string stringValueOrEmpty(std::optional<double> value) { return value ? std::to_string(value.value()) : ""; }

std::map<std::string, std::string> getStitchParams(OptionDefaults const &stitchDefaults) {
  std::map<std::string, std::string> initialStitchParameters;
  auto const &alg = Mantid::API::AlgorithmManager::Instance().create("Stitch1DMany");
  auto const &algPropNames = alg->getDeclaredPropertyNames();
  for (auto const &algPropName : algPropNames) {
    std::string const &defaultPropName = "Stitch" + algPropName;
    auto const &propValue = stitchDefaults.getStringOrEmpty(algPropName, defaultPropName);
    if (!propValue.empty()) {
      initialStitchParameters[algPropName] = propValue;
    }
  }
  return initialStitchParameters;
}

Experiment getExperimentDefaults(Mantid::Geometry::Instrument_const_sptr instrument) {
  // Looks for defaults for use in ReflectometryReductionOneAuto algorithm
  auto defaults = OptionDefaults(instrument, "ReflectometryReductionOneAuto");

  auto analysisMode =
      analysisModeFromString(defaults.getStringOrDefault("AnalysisMode", "AnalysisMode", "PointDetectorAnalysis"));
  auto reductionType = reductionTypeFromString(defaults.getStringOrDefault("ReductionType", "ReductionType", "Normal"));
  auto summationType =
      summationTypeFromString(defaults.getStringOrDefault("SummationType", "SummationType", "SumInLambda"));
  auto includePartialBins = defaults.getBoolOrFalse("IncludePartialBins", "IncludePartialBins");
  auto debug = defaults.getBoolOrFalse("Debug", "Debug");

  auto backgroundSubtractionMethod =
      defaults.getStringOrEmpty("BackgroundCalculationMethod", "BackgroundCalculationMethod");
  auto subtractBackground = !backgroundSubtractionMethod.empty();
  auto backgroundSubtractionType = subtractBackground ? backgroundSubtractionTypeFromString(backgroundSubtractionMethod)
                                                      : BackgroundSubtractionType::PerDetectorAverage;
  auto degreeOfPolynomial = defaults.getIntOrZero("DegreeOfPolynomial", "DegreeOfPolynomial");
  auto costFunction =
      costFunctionTypeFromString(defaults.getStringOrDefault("CostFunction", "CostFunction", "Least squares"));
  auto backgroundSubtraction =
      BackgroundSubtraction(subtractBackground, backgroundSubtractionType, degreeOfPolynomial, costFunction);

  auto polarizationCorrectionType = polarizationCorrectionTypeFromString(
      defaults.getStringOrDefault("PolarizationAnalysis", "PolarizationAnalysis", "None"));
  auto polarizationCorrections = PolarizationCorrections(polarizationCorrectionType);

  auto floodCorrectionType =
      floodCorrectionTypeFromString(defaults.getStringOrDefault("FloodCorrection", "FloodCorrection", "Workspace"));
  auto floodWorkspace = defaults.getOptionalValue<std::string>("FloodWorkspace", "FloodWorkspace");
  auto floodCorrections = FloodCorrections(floodCorrectionType, floodWorkspace);

  auto transmissionRunRange = RangeInLambda(defaults.getDoubleOrZero("StartOverlap", "TransRunStartOverlap"),
                                            defaults.getDoubleOrZero("EndOverlap", "TransRunEndOverlap"));
  if (!transmissionRunRange.isValid(false))
    throw std::invalid_argument("Transmission run overlap range is invalid");

  auto transmissionStitchParams = defaults.getStringOrEmpty("Params", "TransmissionStitchParams");
  auto transmissionScaleRHS = defaults.getBoolOrTrue("ScaleRHSWorkspace", "TransmissionScaleRHS");

  auto transmissionStitchOptions =
      TransmissionStitchOptions(transmissionRunRange, transmissionStitchParams, transmissionScaleRHS);

  // Looks for default Output Stitch Properties for use in Stitch1DMany algorithm
  auto const &stitchDefaults = OptionDefaults(std::move(instrument), "Stitch1DMany");
  auto const &stitchParameters = getStitchParams(stitchDefaults);

  // For per-theta defaults, we can only specify defaults for the wildcard row
  // i.e.  where theta is empty. It probably doesn't make sense to specify
  // tranmsission runs so leave that empty.
  auto const theta = std::string("");
  auto const title = std::string("");
  auto const firstTransmissionRun = std::string("");
  auto const secondTransmissionRun = std::string("");
  auto const transmissionProcessingInstructions =
      defaults.getStringOrEmpty("TransmissionProcessingInstructions", "TransmissionProcessingInstructions");
  auto const qMin = stringValueOrEmpty(defaults.getOptionalValue<double>("MomentumTransferMin", "QMin"));
  auto const qMax = stringValueOrEmpty(defaults.getOptionalValue<double>("MomentumTransferMax", "QMax"));
  auto const qStep = stringValueOrEmpty(defaults.getOptionalValue<double>("MomentumTransferStep", "dQ/Q"));
  auto const maybeScaleFactor = defaults.getOptionalValue<double>("ScaleFactor", "ScaleFactor");
  auto const scaleFactor = stringValueOrEmpty(maybeScaleFactor);
  auto const processingInstructions = defaults.getStringOrEmpty("ProcessingInstructions", "ProcessingInstructions");
  auto const backgroundProcessingInstructions =
      defaults.getStringOrEmpty("BackgroundProcessingInstructions", "BackgroundProcessingInstructions");
  auto lookupRow = LookupRow::ValueArray{{theta, title, firstTransmissionRun, secondTransmissionRun,
                                          transmissionProcessingInstructions, qMin, qMax, qStep, scaleFactor,
                                          processingInstructions, backgroundProcessingInstructions}};
  auto lookupTable = std::vector<LookupRow::ValueArray>{lookupRow};
  auto validate = LookupTableValidator();
  auto const tolerance = 0.0; // irrelevant because theta is empty
  auto lookupTableValidationResult = validate(lookupTable, tolerance);
  if (!lookupTableValidationResult.isValid())
    throw std::invalid_argument("Errors were found in the lookup table values");

  return Experiment(analysisMode, reductionType, summationType, includePartialBins, debug, backgroundSubtraction,
                    polarizationCorrections, std::move(floodCorrections), std::move(transmissionStitchOptions),
                    stitchParameters, lookupTableValidationResult.assertValid());
}
} // unnamed namespace

Experiment ExperimentOptionDefaults::get(Mantid::Geometry::Instrument_const_sptr instrument) {
  return getExperimentDefaults(instrument);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
