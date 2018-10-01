#include "Row.h"
#include "../Map.h"
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {

Row::Row(std::vector<std::string> runNumbers, double theta,
         std::pair<std::string, std::string> transmissionRuns,
         boost::optional<RangeInQ> qRange, boost::optional<double> scaleFactor,
         ReductionOptionsMap reductionOptions,
         ReductionWorkspaces reducedWorkspaceNames)
    : m_runNumbers(std::move(runNumbers)), m_theta(std::move(theta)),
      m_qRange(std::move(qRange)), m_scaleFactor(std::move(scaleFactor)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_reducedWorkspaceNames(std::move(reducedWorkspaceNames)),
      m_reductionOptions(std::move(reductionOptions)) {
  std::sort(m_runNumbers.begin(), m_runNumbers.end());
}

std::vector<std::string> const &Row::runNumbers() const { return m_runNumbers; }

std::pair<std::string, std::string> const &
Row::transmissionWorkspaceNames() const {
  return m_transmissionRuns;
}

double Row::theta() const { return m_theta; }

boost::optional<RangeInQ> const &Row::qRange() const { return m_qRange; }

boost::optional<double> Row::scaleFactor() const { return m_scaleFactor; }

ReductionOptionsMap const &Row::reductionOptions() const {
  return m_reductionOptions;
}

ReductionWorkspaces const &Row::reducedWorkspaceNames() const {
  return m_reducedWorkspaceNames;
}

Row Row::withExtraRunNumbers(
    std::vector<std::string> const &extraRunNumbers) const {
  auto newRunNumbers = std::vector<std::string>();
  newRunNumbers.reserve(m_runNumbers.size() + extraRunNumbers.size());
  boost::range::set_union(m_runNumbers, extraRunNumbers,
                          std::back_inserter(newRunNumbers));
  auto wsNames = workspaceNames(newRunNumbers, transmissionWorkspaceNames());
  return Row(newRunNumbers, theta(), transmissionWorkspaceNames(), qRange(),
             scaleFactor(), reductionOptions(), wsNames);
}

Row mergedRow(Row const &rowA, Row const &rowB) {
  return rowA.withExtraRunNumbers(rowB.runNumbers());
}
} // namespace CustomInterfaces
} // namespace MantidQt
