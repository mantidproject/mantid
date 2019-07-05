// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ExperimentOptionDefaults.h"
#include "Common/OptionDefaults.h"
#include "MantidAPI/AlgorithmManager.h"
#include "PerThetaDefaultsTableValidator.h"
#include "Reduction/Experiment.h"

namespace MantidQt {
namespace CustomInterfaces {

// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");

std::string stringValueOrEmpty(boost::optional<double> value) {
  return value ? std::to_string(*value) : "";
}

Experiment
getExperimentDefaults(Mantid::Geometry::Instrument_const_sptr instrument) {
  auto defaults = OptionDefaults(instrument);

  auto analysisMode = analysisModeFromString(defaults.getStringOrDefault(
      "AnalysisMode", "AnalysisMode", "PointDetectorAnalysis"));
  auto reductionType = reductionTypeFromString(
      defaults.getStringOrDefault("ReductionType", "ReductionType", "Normal"));
  auto summationType = summationTypeFromString(defaults.getStringOrDefault(
      "SummationType", "SummationType", "SumInLambda"));
  auto includePartialBins =
      defaults.getBoolOrFalse("IncludePartialBins", "IncludePartialBins");
  auto debug = defaults.getBoolOrFalse("Debug", "Debug");

  auto polarizationCorrectionType =
      polarizationCorrectionTypeFromString(defaults.getStringOrDefault(
          "PolarizationAnalysis", "PolarizationAnalysis", "None"));
  auto polarizationCorrections =
      PolarizationCorrections(polarizationCorrectionType);

  auto floodCorrectionType =
      floodCorrectionTypeFromString(defaults.getStringOrDefault(
          "FloodCorrection", "FloodCorrection", "Workspace"));
  auto floodWorkspace = defaults.getOptionalValue<std::string>(
      "FloodWorkspace", "FloodWorkspace");
  auto floodCorrections = FloodCorrections(floodCorrectionType, floodWorkspace);

  auto transmissionRunRange = RangeInLambda(
      defaults.getDoubleOrZero("StartOverlap", "TransRunStartOverlap"),
      defaults.getDoubleOrZero("EndOverlap", "TransRunEndOverlap"));
  if (!transmissionRunRange.isValid(false))
    throw std::invalid_argument("Transmission run overlap range is invalid");

  auto transmissionStitchParams =
      defaults.getStringOrEmpty("Params", "TransmissionStitchParams");
  auto transmissionScaleRHS =
      defaults.getBoolOrTrue("ScaleRHSWorkspace", "TransmissionScaleRHS");

  auto transmissionStitchOptions = TransmissionStitchOptions(
      transmissionRunRange, transmissionStitchParams, transmissionScaleRHS);

  // We currently don't specify stitch parameters in the parameters file
  auto stitchParameters = std::map<std::string, std::string>();

  // For per-theta defaults, we can only specify defaults for the wildcard row
  // i.e.  where theta is empty. It probably doesn't make sense to specify
  // tranmsission runs so leave that empty.
  auto const theta = std::string("");
  auto const firstTransmissionRun = std::string("");
  auto const secondTransmissionRun = std::string("");
  auto const transmissionProcessingInstructions =
      defaults.getStringOrEmpty("TransmissionProcessingInstructions",
                                "TransmissionProcessingInstructions");
  auto const qMin = stringValueOrEmpty(
      defaults.getOptionalValue<double>("MomentumTransferMin", "QMin"));
  auto const qMax = stringValueOrEmpty(
      defaults.getOptionalValue<double>("MomentumTransferMax", "QMax"));
  auto const qStep = stringValueOrEmpty(
      defaults.getOptionalValue<double>("MomentumTransferStep", "dQ/Q"));
  auto const maybeScaleFactor =
      defaults.getOptionalValue<double>("ScaleFactor", "ScaleFactor");
  auto const scaleFactor = stringValueOrEmpty(maybeScaleFactor);
  auto const processingInstructions = defaults.getStringOrEmpty(
      "ProcessingInstructions", "ProcessingInstructions");
  auto perThetaDefaultsRow = PerThetaDefaults::ValueArray{
      {theta, firstTransmissionRun, secondTransmissionRun,
       transmissionProcessingInstructions, qMin, qMax, qStep, scaleFactor,
       processingInstructions}};
  auto perThetaDefaults =
      std::vector<PerThetaDefaults::ValueArray>{perThetaDefaultsRow};
  auto validate = PerThetaDefaultsTableValidator();
  auto const tolerance = 0.0; // irrelevant because theta is empty
  auto perThetaValidationResult = validate(perThetaDefaults, tolerance);
  if (!perThetaValidationResult.isValid())
    throw std::invalid_argument(
        "Errors were found in the per-angle default values");

  return Experiment(
      analysisMode, reductionType, summationType, includePartialBins, debug,
      std::move(polarizationCorrections), std::move(floodCorrections),
      std::move(transmissionStitchOptions), std::move(stitchParameters),
      std::move(perThetaValidationResult.assertValid()));
}
} // unnamed namespace

Experiment ExperimentOptionDefaults::get(
    Mantid::Geometry::Instrument_const_sptr instrument) {
  return getExperimentDefaults(instrument);
}
} // namespace CustomInterfaces
} // namespace MantidQt
