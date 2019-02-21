// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Row.h"
#include "Common/Map.h"
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {

Row::Row( // cppcheck-suppress passedByValue
    std::vector<std::string> runNumbers, double theta,
    // cppcheck-suppress passedByValue
    TransmissionRunPair transmissionRuns, RangeInQ qRange,
    boost::optional<double> scaleFactor, ReductionOptionsMap reductionOptions,
    // cppcheck-suppress passedByValue
    ReductionWorkspaces reducedWorkspaceNames)
    : Item(), m_runNumbers(std::move(runNumbers)), m_theta(std::move(theta)),
      m_qRange(std::move(qRange)), m_scaleFactor(std::move(scaleFactor)),
      m_transmissionRuns(std::move(transmissionRuns)),
      m_reducedWorkspaceNames(std::move(reducedWorkspaceNames)),
      m_reductionOptions(std::move(reductionOptions)) {
  std::sort(m_runNumbers.begin(), m_runNumbers.end());
}

bool Row::isGroup() const { return false; }

std::vector<std::string> const &Row::runNumbers() const { return m_runNumbers; }

TransmissionRunPair const &Row::transmissionWorkspaceNames() const {
  return m_transmissionRuns;
}

double Row::theta() const { return m_theta; }

RangeInQ const &Row::qRange() const { return m_qRange; }

boost::optional<double> Row::scaleFactor() const { return m_scaleFactor; }

ReductionOptionsMap const &Row::reductionOptions() const {
  return m_reductionOptions;
}

ReductionWorkspaces const &Row::reducedWorkspaceNames() const {
  return m_reducedWorkspaceNames;
}

void Row::setOutputNames(std::vector<std::string> const &outputNames) {
  if (outputNames.size() != 3)
    throw std::runtime_error("Invalid number of output workspaces for row");

  m_reducedWorkspaceNames.setOutputNames(outputNames[0], outputNames[1],
                                         outputNames[2]);
}

void Row::resetOutputNames() { m_reducedWorkspaceNames.resetOutputNames(); }

Row mergedRow(Row const &rowA, Row const &rowB) {
  return rowA.withExtraRunNumbers(rowB.runNumbers());
}

bool Row::hasOutputWorkspace(std::string const &wsName) const {
  return m_reducedWorkspaceNames.hasOutputName(wsName);
}

void Row::renameOutputWorkspace(std::string const &oldName,
                                std::string const &newName) {
  m_reducedWorkspaceNames.renameOutput(oldName, newName);
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
} // namespace CustomInterfaces
} // namespace MantidQt
