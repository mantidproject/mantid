#include "Row.h"
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {

template <typename ReducedWorkspaceNames>
Row<ReducedWorkspaceNames>::Row(
    std::vector<std::string> runNumbers, double theta,
    std::pair<std::string, std::string> transmissionRuns,
    boost::optional<RangeInQ> qRange, boost::optional<double> scaleFactor,
    ReductionOptionsMap reductionOptions,
    ReducedWorkspaceNames reducedWorkspaceNames)
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
template <typename WorkspaceNamesFactory>
Row<ReducedWorkspaceNames> Row<ReducedWorkspaceNames>::withExtraRunNumbers(
    std::vector<std::string> const &extraRunNumbers,
    WorkspaceNamesFactory const &workspaceNames) const {
  auto newRunNumbers = m_runNumbers;
  newRunNumbers.reserve(newRunNumbers.size() + extraRunNumbers.size());
  newRunNumbers.insert(newRunNumbers.end(), extraRunNumbers.begin(),
                       extraRunNumbers.end());
  auto wsNames = workspaceNames(newRunNumbers, transmissionWorkspaceNames());
  return Row(newRunNumbers, theta(), qRange(), transmissionWorkspaceNames(),
             scaleFactor(), reductionOptions(), wsNames);
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
boost::optional<double> Row<ReducedWorkspaceNames>::scaleFactor() const {
  return m_scaleFactor;
}

template <typename ReducedWorkspaceNames>
ReductionOptionsMap const &
Row<ReducedWorkspaceNames>::reductionOptions() const {
  return m_reductionOptions;
}

template <typename ReducedWorkspaceNames>
ReducedWorkspaceNames const &
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

class ThetaVisitor : boost::static_visitor<double> {
public:
  template <typename Row>
  double operator()(Row const& row) const {
    return row.theta();
  }
};

template class Row<SlicedReductionWorkspaces>;
template bool operator==(SlicedRow const &, SlicedRow const &);
template bool operator!=(SlicedRow const &, SlicedRow const &);

template class Row<ReductionWorkspaces>;
template bool operator==(UnslicedRow const &, UnslicedRow const &);
template bool operator!=(UnslicedRow const &, UnslicedRow const &);
}
}
