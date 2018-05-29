#include "Row.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename ReducedWorkspaceNames>
Row<ReducedWorkspaceNames>::Row(
    std::vector<std::string> runNumbers, double theta,
    std::pair<std::string, std::string> transmissionRuns, RangeInQ qRange,
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
RangeInQ const &Row<ReducedWorkspaceNames>::qRange() const {
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

template <typename ReducedWorkspaceNames>
auto Row<ReducedWorkspaceNames>::withRunNumbers(
    std::vector<std::string> const &runNumbers) const -> Row {
  return Row(runNumbers, m_theta, m_transmissionRuns, m_qRange, m_scaleFactor,
             m_reducedWorkspaceNames, m_reductionOptions);
}

template <typename ReducedWorkspaceNames>
auto Row<ReducedWorkspaceNames>::withTheta(double theta) const -> Row {
  return Row(m_runNumbers, theta, m_transmissionRuns, m_qRange, m_scaleFactor,
             m_reducedWorkspaceNames, m_reductionOptions);
}

template <typename ReducedWorkspaceNames>
auto Row<ReducedWorkspaceNames>::withQRange(RangeInQ const &qRange) const
    -> Row {
  return Row(m_runNumbers, m_theta, m_transmissionRuns, qRange, m_scaleFactor,
             m_reducedWorkspaceNames, m_reductionOptions);
}

template <typename ReducedWorkspaceNames>
auto Row<ReducedWorkspaceNames>::withScaleFactor(double scaleFactor) const
    -> Row {
  return Row(m_runNumbers, m_theta, m_transmissionRuns, m_qRange, scaleFactor,
             m_reducedWorkspaceNames, m_reductionOptions);
}

template <typename ReducedWorkspaceNames>
auto Row<ReducedWorkspaceNames>::withReductionOptions(
    ReductionOptionsMap const &reductionOptions) const -> Row {
  return Row(m_runNumbers, m_theta, m_transmissionRuns, m_qRange, m_scaleFactor,
             m_reducedWorkspaceNames, reductionOptions);
}

template <typename ReducedWorkspaceNames>
auto Row<ReducedWorkspaceNames>::withTransmissionRunNumbers(
    std::pair<std::string, std::string> const &transmissionRuns) const -> Row {
  return Row(m_runNumbers, m_theta, transmissionRuns, m_qRange, m_scaleFactor,
             m_reducedWorkspaceNames, m_reductionOptions);
}

template <typename ReducedWorkspaceNames>
auto Row<ReducedWorkspaceNames>::withReducedWorkspaceNames(
    boost::optional<ReducedWorkspaceNames> const &workspaceNames) const -> Row {
  return Row(m_runNumbers, m_theta, m_transmissionRuns, m_qRange, m_scaleFactor,
             workspaceNames, m_reductionOptions);
}

template class Row<SlicedReductionWorkspaces>;
template bool operator==(SlicedRow const &, SlicedRow const &);
template bool operator!=(SlicedRow const &, SlicedRow const &);

template class Row<ReductionWorkspaces>;
template bool operator==(SingleRow const &, SingleRow const &);
template bool operator!=(SingleRow const &, SingleRow const &);
}
}
