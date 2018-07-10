#include "Experiment.h"
#include <cmath>
#include <iostream>

namespace MantidQt {
namespace CustomInterfaces {

Experiment::Experiment(AnalysisMode analysisMode, ReductionType reductionType,
                       SummationType summationType,
                       PolarizationCorrections polarizationCorrections,
                       RangeInLambda transmissionRunRange,
                       std::map<std::string, std::string> stitchParameters,
                       std::vector<PerThetaDefaults> perThetaDefaults)
    : m_analysisMode(analysisMode), m_reductionType(reductionType),
      m_summationType(summationType),
      m_polarizationCorrections(std::move(polarizationCorrections)),
      m_transmissionRunRange(std::move(transmissionRunRange)),
      m_stitchParameters(std::move(stitchParameters)),
      m_perThetaDefaults(std::move(perThetaDefaults)) {}

AnalysisMode Experiment::analysisMode() const { return m_analysisMode; }
ReductionType Experiment::reductionType() const { return m_reductionType; }
SummationType Experiment::summationType() const { return m_summationType; }
PolarizationCorrections const &Experiment::polarizationCorrections() const {
  return m_polarizationCorrections;
}

int Experiment::countWildcards(
    std::vector<PerThetaDefaults> const &perThetaDefaults) {
  return static_cast<int>(
      std::count_if(perThetaDefaults.cbegin(), perThetaDefaults.cend(),
                    [](PerThetaDefaults const &defaults)
                        -> bool { return defaults.isWildcard(); }));
}

// Need to cope with no wildcard.
ThetaValuesValidationResult
Experiment::validateThetaValues(std::vector<PerThetaDefaults> perThetaDefaults,
                                double tolerance) {
  if (!perThetaDefaults.empty()) {
    auto wildcardCount = countWildcards(perThetaDefaults);
    if (wildcardCount <= 1) {
      auto thetaLt =
          [](PerThetaDefaults const &lhs, PerThetaDefaults const &rhs) -> bool {
            if (lhs.isWildcard())
              return true;
            else if (rhs.isWildcard())
              return false;
            else
              return lhs.thetaOrWildcard().get() < rhs.thetaOrWildcard().get();
          };

      auto thetaWithinRange = [tolerance](PerThetaDefaults const &lhs,
                                          PerThetaDefaults const &rhs) -> bool {
        std::cout << "lhs: " << lhs.thetaOrWildcard().get()
                  << ", rhs:" << rhs.thetaOrWildcard().get()
                  << ", tolerance: " << tolerance << std::endl;

        return std::abs(lhs.thetaOrWildcard().get() -
                        rhs.thetaOrWildcard().get()) < tolerance;
      };

      std::sort(perThetaDefaults.begin(), perThetaDefaults.end(), thetaLt);

      for (const auto &def : perThetaDefaults) {
        std::cout << (def.isWildcard() ? "*" : "-") << std::endl;
      }

      return std::adjacent_find(perThetaDefaults.cbegin() + wildcardCount,
                                perThetaDefaults.cend(),
                                thetaWithinRange) == perThetaDefaults.cend()
                 ? ThetaValuesValidationResult::Ok
                 : ThetaValuesValidationResult::NonUniqueTheta;
    } else {
      return ThetaValuesValidationResult::MultipleWildcards;
    }
  } else {
    return ThetaValuesValidationResult::Ok;
  }
}

RangeInLambda const &Experiment::transissionRunRange() const {
  return m_transmissionRunRange;
}

std::map<std::string, std::string> Experiment::stitchParameters() const {
  return m_stitchParameters;
}

std::vector<PerThetaDefaults> const &Experiment::perThetaDefaults() const {
  return m_perThetaDefaults;
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
    auto wildcardMatch =
        std::find_if(m_perThetaDefaults.cbegin(), m_perThetaDefaults.cend(),
                     [](PerThetaDefaults const &candidate)
                         -> bool { return candidate.isWildcard(); });
    if (wildcardMatch != m_perThetaDefaults.cend()) {
      return &(*wildcardMatch);
    } else {
      return nullptr;
    }
  }
}
}
}
