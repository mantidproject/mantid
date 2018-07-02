#include "Experiment.h"
#include <cmath>

namespace MantidQt {
namespace CustomInterfaces {

Experiment::Experiment(AnalysisMode analysisMode, ReductionType reductionType,
                       SummationType summationType,
                       PolarizationCorrections polarizationCorrections,
                       RangeInLambda transmissionRunRange,
                       std::string stitchParameters,
                       std::vector<RowTemplate> rowTemplates)
    : m_analysisMode(analysisMode), m_reductionType(reductionType),
      m_summationType(summationType),
      m_polarizationCorrections(std::move(polarizationCorrections)),
      m_transmissionRunRange(std::move(transmissionRunRange)),
      m_stitchParameters(std::move(stitchParameters)),
      m_rowTemplates(std::move(rowTemplates)) {}

AnalysisMode Experiment::analysisMode() const { return m_analysisMode; }
ReductionType Experiment::reductionType() const { return m_reductionType; }
SummationType Experiment::summationType() const { return m_summationType; }
PolarizationCorrections const &Experiment::polarizationCorrections() const {
  return m_polarizationCorrections;
}

RangeInLambda const &Experiment::transissionRunRange() const {
  return m_transmissionRunRange;
}
std::string Experiment::stitchParameters() const { return m_stitchParameters; }
std::vector<RowTemplate> const &Experiment::rowTemplates() const {
  return m_rowTemplates;
}

RowTemplate const *Experiment::rowTemplateForTheta(double thetaAngle,
                                                   double tolerance) const {
  auto smallestIt = std::min_element(
      m_rowTemplates.cbegin(),
      m_rowTemplates.cend(),
      [thetaAngle](RowTemplate const &lhs, RowTemplate const &rhs)
          -> bool { return std::abs(thetaAngle - lhs.theta()) < std::abs(thetaAngle - rhs.theta()); });

  auto const *closestCandidate = &(*smallestIt);
  if (std::abs(thetaAngle - closestCandidate->theta()) <= tolerance) {
    return closestCandidate;
  } else {
    return nullptr;
  }
}
}
}
