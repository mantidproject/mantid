#include "PerThetaDefaults.h"

namespace MantidQt {
namespace CustomInterfaces {

PerThetaDefaults::PerThetaDefaults(
    boost::optional<double> theta,
    std::pair<std::string, std::string> transmissionRuns,
    boost::optional<RangeInQ> qRange, boost::optional<double> scaleFactor,
    ReductionOptionsMap reductionOptions)
    : m_theta(std::move(theta)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_qRange(std::move(qRange)), m_scaleFactor(std::move(scaleFactor)),
      m_reductionOptions(std::move(reductionOptions)) {}

std::pair<std::string, std::string> const &
PerThetaDefaults::transmissionWorkspaceNames() const {
  return m_transmissionRuns;
}

bool PerThetaDefaults::isWildcard() const { return !m_theta.is_initialized(); }

boost::optional<double> PerThetaDefaults::thetaOrWildcard() const {
  return m_theta;
}

boost::optional<RangeInQ> const &PerThetaDefaults::qRange() const {
  return m_qRange;
}

boost::optional<double> PerThetaDefaults::scaleFactor() const {
  return m_scaleFactor;
}

ReductionOptionsMap const &PerThetaDefaults::reductionOptions() const {
  return m_reductionOptions;
}

bool operator==(PerThetaDefaults const &lhs, PerThetaDefaults const &rhs) {
  return lhs.thetaOrWildcard() == rhs.thetaOrWildcard() &&
         lhs.qRange() == rhs.qRange() &&
         lhs.scaleFactor() == rhs.scaleFactor() &&
         lhs.reductionOptions() == rhs.reductionOptions();
}

bool operator!=(PerThetaDefaults const &lhs, PerThetaDefaults const &rhs) {
  return !(lhs == rhs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
