#include "Experiment.h"
namespace MantidQt {
namespace CustomInterfaces {

Experiment::Experiment(AnalysisMode analysisMode, ReductionType reductionType,
                       SummationType summationType,
                       PolarizationCorrections polarizationCorrections,
                       RangeInLambda transmissionRunRange,
                       std::string stitchParameters,
                       std::vector<RowTemplate> rowTemplate)
    : m_analysisMode(analysisMode), m_reductionType(reductionType),
      m_summationType(summationType),
      m_polarizatioCorrections(polarizationCorrections), m_transmissionRuns(transmissionRunRange),
      m_stitchParameters(std::move(stitchParameters)),
      m_rowTemplates(std::move(rowTemplates)) {}

AnalysisMode Experiment::analysisMode() const {
  return m_analysisMode;
}
ReductionType reductionType() const {
  return m_reductionType;
}
SummationType summationType() const {
  return m_summationType;
}
PolarizationCorrections const &polarizationCorrections() const {
  return m_polarizationCorrections;
}

RangeInLambda const &transissionRunRange() const {
  return m_transmissionRunRange;
}
std::string stitchParameters() const {
  return m_stitchParameters;
}
std::vector<RowTemplate> const &rowTemplates() const {
  return m_rowTemplates;
}

RowTemplate const* rowTemplateForTheta(double thetaAngle, double tolerance) const {
  auto smallestIt = std::min_element(m_rowTemplates, [thetaAngle](RowTemplate const& lhs, RowTemplate const& rhs) -> bool {
    return std::abs(thetaAngle - lhs.theta());
  });

  auto const* closestCandidate = &(*smallestIt);
  if (std::abs(thetaAngle - closestCandidate->theta()) <= tolerance) {
    return closestCandidate;
  } else {
    return nullptr;
  }
}
}
}
