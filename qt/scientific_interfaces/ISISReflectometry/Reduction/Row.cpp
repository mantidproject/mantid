#include "Row.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename ReducedWorkspaceNames>
Row<ReducedWorkspaceNames>::Row(
    std::vector<std::string> runNumbers, double theta,
    std::pair<std::string, std::string> transmissionRuns, boost::optional<RangeInQ> qRange,
    double scaleFactor,
    boost::optional<ReducedWorkspaceNames> reducedWorkspaceNames,
    ReductionOptionsMap reductionOptions)
    : m_runNumbers(std::move(runNumbers)), m_theta(std::move(theta)),
      m_qRange(std::move(qRange)), m_scaleFactor(std::move(scaleFactor)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_reducedWorkspaceNames(std::move(reducedWorkspaceNames)),
      m_reductionOptions(std::move(reductionOptions)) {}

template <typename ReducedWorkspaceNames>
std::vector<std::string> const &Row<ReducedWorkspaceNames>::runNumbers() const {
  return m_runNumbers;
}

template <typename ReducedWorkspaceNames>
double Row<ReducedWorkspaceNames>::theta() const {
  return m_theta;
}

template <typename ReducedWorkspaceNames>
boost::optional<RangeInQ> const &Row<ReducedWorkspaceNames>::qRange() const {
  return m_qRange;
}

template <typename ReducedWorkspaceNames>
double Row<ReducedWorkspaceNames>::scaleFactor() const {
  return m_scaleFactor;
}

template <typename ReducedWorkspaceNames>
ReductionOptionsMap const &
Row<ReducedWorkspaceNames>::reductionOptions() const {
  return m_reductionOptions;
}

template <typename ReducedWorkspaceNames>
boost::optional<ReducedWorkspaceNames> const &
Row<ReducedWorkspaceNames>::reducedWorkspaceNames() const {
  return m_reducedWorkspaceNames;
}

template <typename ReducedWorkspaceNames>
bool operator==(Row<ReducedWorkspaceNames> const &lhs,
                Row<ReducedWorkspaceNames> const &rhs) {
  return lhs.runNumbers() == rhs.runNumbers() && lhs.theta() == rhs.theta() &&
         lhs.qRange() == rhs.qRange() &&
         lhs.scaleFactor() == rhs.scaleFactor() &&
         lhs.reductionOptions() == rhs.reductionOptions() &&
         lhs.reducedWorkspaceNames() == rhs.reducedWorkspaceNames();
}

template <typename ReducedWorkspaceNames>
bool operator!=(Row<ReducedWorkspaceNames> const &lhs,
                Row<ReducedWorkspaceNames> const &rhs) {
  return !(lhs == rhs);
}

template class Row<SlicedReductionWorkspaces>;
template bool operator==(SlicedRow const &, SlicedRow const &);
template bool operator!=(SlicedRow const &, SlicedRow const &);

template class Row<ReductionWorkspaces>;
template bool operator==(SingleRow const &, SingleRow const &);
template bool operator!=(SingleRow const &, SingleRow const &);
}
}
