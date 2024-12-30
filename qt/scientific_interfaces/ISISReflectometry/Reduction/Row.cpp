// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Row.h"
#include "Common/Map.h"
#include "IGroup.h"
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

Row::Row(std::vector<std::string> runNumbers, double theta,

         TransmissionRunPair transmissionRuns, RangeInQ qRange, boost::optional<double> scaleFactor,
         ReductionOptionsMap reductionOptions,

         ReductionWorkspaces reducedWorkspaceNames)
    : Item(), m_runNumbers(std::move(runNumbers)), m_theta(theta), m_qRange(std::move(qRange)), m_qRangeOutput(),
      m_scaleFactor(std::move(scaleFactor)), m_transmissionRuns(std::move(transmissionRuns)),
      m_reducedWorkspaceNames(std::move(reducedWorkspaceNames)), m_reductionOptions(std::move(reductionOptions)),
      m_lookupIndex(std::nullopt), m_parent(nullptr) {
  std::sort(m_runNumbers.begin(), m_runNumbers.end());
}

bool Row::isGroup() const { return false; }

bool Row::isPreview() const { return false; }

std::vector<std::string> const &Row::runNumbers() const { return m_runNumbers; }

TransmissionRunPair const &Row::transmissionWorkspaceNames() const { return m_transmissionRuns; }

double Row::theta() const { return m_theta; }

RangeInQ const &Row::qRange() const { return m_qRange; }

RangeInQ const &Row::qRangeOutput() const { return m_qRangeOutput; }

boost::optional<double> Row::scaleFactor() const { return m_scaleFactor; }

ReductionOptionsMap const &Row::reductionOptions() const { return m_reductionOptions; }

ReductionWorkspaces const &Row::reducedWorkspaceNames() const { return m_reducedWorkspaceNames; }

const std::optional<size_t> &Row::lookupIndex() const { return m_lookupIndex; }

void Row::setOutputNames(std::vector<std::string> const &outputNames) {
  if (outputNames.size() != 3)
    throw std::runtime_error("Invalid number of output workspaces for row");

  m_reducedWorkspaceNames.setOutputNames(outputNames[0], outputNames[1], outputNames[2]);
}

void Row::setOutputQRange(RangeInQ qRange) { m_qRangeOutput = std::move(qRange); }

void Row::setLookupIndex(std::optional<size_t> lookupIndex) { m_lookupIndex = std::move(lookupIndex); }

void Row::resetOutputs() {
  m_reducedWorkspaceNames.resetOutputNames();
  m_qRangeOutput = RangeInQ();
}

Row mergedRow(Row const &rowA, Row const &rowB) { return rowA.withExtraRunNumbers(rowB.runNumbers()); }

bool Row::hasOutputWorkspace(std::string const &wsName) const { return m_reducedWorkspaceNames.hasOutputName(wsName); }

void Row::renameOutputWorkspace(std::string const &oldName, std::string const &newName) {
  m_reducedWorkspaceNames.renameOutput(oldName, newName);
}

void Row::setParent(IGroup *parent) const { m_parent = parent; }

IGroup *Row::getParent() const { return m_parent; }

void Row::updateParent() {
  if (m_parent) {
    m_parent->notifyChildStateChanged();
  }
}

Row Row::withExtraRunNumbers(std::vector<std::string> const &extraRunNumbers) const {
  // If both lists of run numbers are the same then there's nothing to merge
  if (extraRunNumbers == m_runNumbers)
    return *this;

  auto newRunNumbers = std::vector<std::string>();
  newRunNumbers.reserve(m_runNumbers.size() + extraRunNumbers.size());
  boost::range::set_union(m_runNumbers, extraRunNumbers, std::back_inserter(newRunNumbers));
  auto wsNames = workspaceNames(newRunNumbers, transmissionWorkspaceNames());
  return Row(newRunNumbers, theta(), transmissionWorkspaceNames(), qRange(), scaleFactor(), reductionOptions(),
             wsNames);
}

int Row::totalItems() const { return 1; }

int Row::completedItems() const {
  if (complete())
    return 1;

  return 0;
}

void Row::resetState(bool resetChildren) {
  Item::resetState(resetChildren);
  updateParent();
}

void Row::setStarting() {
  Item::setStarting();
  updateParent();
}

void Row::setRunning() {
  Item::setRunning();
  updateParent();
}

void Row::setSuccess() {
  Item::setSuccess();
  updateParent();
}

void Row::setError(std::string const &msg) {
  Item::setError(msg);
  updateParent();
}

bool operator!=(Row const &lhs, Row const &rhs) { return !(lhs == rhs); }

bool operator==(Row const &lhs, Row const &rhs) {
  return lhs.runNumbers() == rhs.runNumbers() && lhs.theta() == rhs.theta() && lhs.qRange() == rhs.qRange() &&
         lhs.scaleFactor() == rhs.scaleFactor() &&
         lhs.transmissionWorkspaceNames() == rhs.transmissionWorkspaceNames() &&
         lhs.reducedWorkspaceNames() == rhs.reducedWorkspaceNames() && lhs.reductionOptions() == rhs.reductionOptions();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
