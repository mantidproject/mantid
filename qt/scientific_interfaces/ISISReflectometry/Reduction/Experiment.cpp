#include "Experiment.h"
#include <cmath>

namespace MantidQt {
namespace CustomInterfaces {

Experiment::Experiment(AnalysisMode analysisMode, ReductionType reductionType,
                       SummationType summationType,
                       PolarizationCorrections polarizationCorrections,
                       RangeInLambda transmissionRunRange,
                       std::string stitchParameters,
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

bool Experiment::thetaValuesAreUnique(
    std::vector<PerThetaDefaults> perThetaDefaults, double tolerance) {
  auto thetaLt = [](PerThetaDefaults const &lhs, PerThetaDefaults const &rhs)
                     -> bool { return lhs.theta() < rhs.theta(); };
  auto thetaWithinRange =
      [tolerance](PerThetaDefaults const &lhs, PerThetaDefaults const &rhs)
          -> bool { return std::abs(lhs.theta() - rhs.theta()) < tolerance; };

  std::sort(perThetaDefaults.begin(), perThetaDefaults.end(), thetaLt);
  return std::adjacent_find(perThetaDefaults.cbegin(), perThetaDefaults.cend(),
                            thetaWithinRange) == perThetaDefaults.cend();
}

RangeInLambda const &Experiment::transissionRunRange() const {
  return m_transmissionRunRange;
}
std::string Experiment::stitchParameters() const { return m_stitchParameters; }
std::vector<PerThetaDefaults> const &Experiment::perThetaDefaults() const {
  return m_perThetaDefaults;
}

PerThetaDefaults const *Experiment::defaultsForTheta(double thetaAngle,
                                                     double tolerance) const {
  auto smallestIt =
      std::min_element(m_perThetaDefaults.cbegin(), m_perThetaDefaults.cend(),
                       [thetaAngle](PerThetaDefaults const &lhs,
                                    PerThetaDefaults const &rhs) -> bool {
                         return std::abs(thetaAngle - lhs.theta()) <
                                std::abs(thetaAngle - rhs.theta());
                       });

  auto const *closestCandidate = &(*smallestIt);
  if (std::abs(thetaAngle - closestCandidate->theta()) <= tolerance) {
    return closestCandidate;
  } else {
    return nullptr;
  }
}
}
}
