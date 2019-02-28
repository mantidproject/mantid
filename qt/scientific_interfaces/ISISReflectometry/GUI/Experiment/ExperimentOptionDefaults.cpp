// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ExperimentOptionDefaults.h"
#include "Common/OptionDefaults.h"
#include "MantidAPI/AlgorithmManager.h"
#include "Reduction/Experiment.h"

namespace MantidQt {
namespace CustomInterfaces {
Experiment
experimentDefaults(Mantid::Geometry::Instrument_const_sptr instrument) {
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

  // We currently don't specify stitch parameters in the parameters file
  // although we
  auto stitchParameters = std::map<std::string, std::string>();

  // For per-theta defaults, we can only specify defaults for the wildcard row
  // i.e.
  // where theta is empty. It probably doesn't make sense to specify
  // tranmsission runs
  // so leave that empty.
  auto theta = boost::none;
  auto transmissionRuns = TransmissionRunPair();
  auto qRange = RangeInQ(
      defaults.getOptionalValue<double>("MomentumTransferMin", "QMin"),
      defaults.getOptionalValue<double>("MomentumTransferStep", "dQ/Q"),
      defaults.getOptionalValue<double>("MomentumTransferMax", "QMax"));
  auto scaleFactor =
      defaults.getOptionalValue<double>("ScaleFactor", "ScaleFactor");
  auto processingInstructions = defaults.getOptionalValue<std::string>(
      "ProcessingInstructions", "ProcessingInstructions");
  auto perThetaDefaults = std::vector<PerThetaDefaults>();
  perThetaDefaults.emplace_back(theta, transmissionRuns, qRange, scaleFactor,
                                processingInstructions);

  return Experiment(
      analysisMode, reductionType, summationType, includePartialBins, debug,
      std::move(polarizationCorrections), std::move(floodCorrections),
      std::move(transmissionRunRange), std::move(stitchParameters),
      std::move(perThetaDefaults));
}
} // namespace CustomInterfaces
} // namespace MantidQt
