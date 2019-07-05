// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Experiment.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <cmath>

namespace MantidQt {
namespace CustomInterfaces {

Experiment::Experiment()
    : m_analysisMode(AnalysisMode::PointDetector),
      m_reductionType(ReductionType::Normal),
      m_summationType(SummationType::SumInLambda), m_includePartialBins(false),
      m_debug(false), m_polarizationCorrections(PolarizationCorrections(
                          PolarizationCorrectionType::None)),
      m_floodCorrections(FloodCorrections(FloodCorrectionType::Workspace)),
      m_transmissionStitchOptions(),
      m_stitchParameters(std::map<std::string, std::string>()),
      m_perThetaDefaults(std::vector<PerThetaDefaults>({PerThetaDefaults(
          boost::none, TransmissionRunPair(), boost::none, RangeInQ(),
          boost::none, ProcessingInstructions())})) {}

Experiment::Experiment(AnalysisMode analysisMode, ReductionType reductionType,
                       SummationType summationType, bool includePartialBins,
                       bool debug,
                       PolarizationCorrections polarizationCorrections,
                       FloodCorrections floodCorrections,
                       TransmissionStitchOptions transmissionStitchOptions,
                       // cppcheck-suppress passedByValue
                       std::map<std::string, std::string> stitchParameters,
                       // cppcheck-suppress passedByValue
                       std::vector<PerThetaDefaults> perThetaDefaults)
    : m_analysisMode(analysisMode), m_reductionType(reductionType),
      m_summationType(summationType), m_includePartialBins(includePartialBins),
      m_debug(debug),
      m_polarizationCorrections(std::move(polarizationCorrections)),
      m_floodCorrections(std::move(floodCorrections)),
      m_transmissionStitchOptions(std::move(transmissionStitchOptions)),
      m_stitchParameters(std::move(stitchParameters)),
      m_perThetaDefaults(std::move(perThetaDefaults)) {}

AnalysisMode Experiment::analysisMode() const { return m_analysisMode; }
ReductionType Experiment::reductionType() const { return m_reductionType; }
SummationType Experiment::summationType() const { return m_summationType; }
bool Experiment::includePartialBins() const { return m_includePartialBins; }
bool Experiment::debug() const { return m_debug; }
PolarizationCorrections const &Experiment::polarizationCorrections() const {
  return m_polarizationCorrections;
}
FloodCorrections const &Experiment::floodCorrections() const {
  return m_floodCorrections;
}

TransmissionStitchOptions Experiment::transmissionStitchOptions() const {
  return m_transmissionStitchOptions;
}

std::map<std::string, std::string> Experiment::stitchParameters() const {
  return m_stitchParameters;
}

std::string Experiment::stitchParametersString() const {
  return MantidQt::MantidWidgets::optionsToString(m_stitchParameters);
}

std::vector<PerThetaDefaults> const &Experiment::perThetaDefaults() const {
  return m_perThetaDefaults;
}

std::vector<PerThetaDefaults::ValueArray>
Experiment::perThetaDefaultsArray() const {
  auto result = std::vector<PerThetaDefaults::ValueArray>();
  for (auto const &perThetaDefaults : m_perThetaDefaults)
    result.push_back(perThetaDefaultsToArray(perThetaDefaults));
  return result;
}

PerThetaDefaults const *Experiment::defaultsForTheta(double thetaAngle,
                                                     double tolerance) const {
  auto nonWildcardMatch = std::find_if(
      m_perThetaDefaults.cbegin(), m_perThetaDefaults.cend(),
      [thetaAngle, tolerance](PerThetaDefaults const &candiate) -> bool {
        return !candiate.isWildcard() &&
               std::abs(thetaAngle - candiate.thetaOrWildcard().get()) <=
                   tolerance;
      });
  if (nonWildcardMatch != m_perThetaDefaults.cend()) {
    return &(*nonWildcardMatch);
  } else {
    return wildcardDefaults();
  }
}

PerThetaDefaults const *Experiment::wildcardDefaults() const {
  auto wildcardMatch =
      std::find_if(m_perThetaDefaults.cbegin(), m_perThetaDefaults.cend(),
                   [](PerThetaDefaults const &candidate) -> bool {
                     return candidate.isWildcard();
                   });
  if (wildcardMatch != m_perThetaDefaults.cend()) {
    return &(*wildcardMatch);
  } else {
    return nullptr;
  }
}

bool operator!=(Experiment const &lhs, Experiment const &rhs) {
  return !(lhs == rhs);
}

bool operator==(Experiment const &lhs, Experiment const &rhs) {
  return lhs.analysisMode() == rhs.analysisMode() &&
         lhs.reductionType() == rhs.reductionType() &&
         lhs.summationType() == rhs.summationType() &&
         lhs.includePartialBins() == rhs.includePartialBins() &&
         lhs.debug() == rhs.debug() &&
         lhs.polarizationCorrections() == rhs.polarizationCorrections() &&
         lhs.floodCorrections() == rhs.floodCorrections() &&
         lhs.transmissionStitchOptions() == rhs.transmissionStitchOptions() &&
         lhs.stitchParameters() == rhs.stitchParameters() &&
         lhs.perThetaDefaults() == rhs.perThetaDefaults();
}
} // namespace CustomInterfaces
} // namespace MantidQt
