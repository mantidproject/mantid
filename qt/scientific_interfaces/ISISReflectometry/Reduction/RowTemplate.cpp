#include "RowTemplate.h"

namespace MantidQt {
namespace CustomInterfaces {

RowTemplate::RowTemplate(double theta,
                         std::pair<std::string, std::string> transmissionRuns,
                         boost::optional<RangeInQ> qRange,
                         boost::optional<double> scaleFactor,
                         ReductionOptionsMap reductionOptions)
    : m_theta(std::move(theta)), m_qRange(std::move(qRange)),
      m_scaleFactor(std::move(scaleFactor)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_reductionOptions(std::move(reductionOptions)) {}

std::vector<std::string> const &RowTemplate::runNumbers() const {
  return m_runNumbers;
}

std::pair<std::string, std::string> const &
RowTemplate::transmissionWorkspaceNames() const {
  return m_transmissionRuns;
}

double RowTemplate::theta() const { return m_theta; }

boost::optional<RangeInQ> const &RowTemplate::qRange() const {
  return m_qRange;
}

boost::optional<double> RowTemplate::scaleFactor() const {
  return m_scaleFactor;
}

ReductionOptionsMap const &RowTemplate::reductionOptions() const {
  return m_reductionOptions;
}
}
}
