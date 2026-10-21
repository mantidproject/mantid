// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReductionWorkspaces.h"

#include <utility>

#include "Common/Map.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {
ReductionWorkspaces::ReductionWorkspaces(

    std::vector<std::string> inputRunNumbers,

    TransmissionRunPair transmissionRuns)
    : m_inputRunNumbers(std::move(inputRunNumbers)), m_transmissionRuns(std::move(transmissionRuns)), m_iVsLambda(),
      m_iVsQ(), m_iVsQBinned() {}

std::vector<std::string> const &ReductionWorkspaces::inputRunNumbers() const { return m_inputRunNumbers; }

TransmissionRunPair const &ReductionWorkspaces::transmissionRuns() const { return m_transmissionRuns; }

std::vector<std::string> const &ReductionWorkspaces::iVsLambda() const { return m_iVsLambda; }

std::vector<std::string> const &ReductionWorkspaces::iVsQ() const { return m_iVsQ; }

std::vector<std::string> const &ReductionWorkspaces::iVsQBinned() const { return m_iVsQBinned; }

void ReductionWorkspaces::setOutputNames(std::vector<std::string> iVsLambda, std::vector<std::string> iVsQ,
                                         std::vector<std::string> iVsQBinned) {
  m_iVsLambda = std::move(iVsLambda);
  m_iVsQ = std::move(iVsQ);
  m_iVsQBinned = std::move(iVsQBinned);
}

void ReductionWorkspaces::resetOutputNames() {
  m_iVsLambda = std::vector<std::string>();
  m_iVsQ = std::vector<std::string>();
  m_iVsQBinned = std::vector<std::string>();
}

bool ReductionWorkspaces::hasOutputName(std::string const &wsName) const {
  return std::any_of(m_iVsLambda.cbegin(), m_iVsLambda.cend(),
                     [&wsName](std::string lamWsName) { return lamWsName == wsName; }) ||
         std::any_of(m_iVsQ.cbegin(), m_iVsQ.cend(),
                     [&wsName](std::string lamWsName) { return lamWsName == wsName; }) ||
         std::any_of(m_iVsQBinned.cbegin(), m_iVsQBinned.cend(),
                     [&wsName](std::string lamWsName) { return lamWsName == wsName; });
}

void ReductionWorkspaces::renameOutput(std::string const &oldName, std::string const &newName) {
  std::replace(m_iVsLambda.begin(), m_iVsLambda.end(), oldName, newName);
  std::replace(m_iVsQ.begin(), m_iVsQ.end(), oldName, newName);
  std::replace(m_iVsQBinned.begin(), m_iVsQBinned.end(), oldName, newName);
}

bool operator==(ReductionWorkspaces const &lhs, ReductionWorkspaces const &rhs) {
  return lhs.inputRunNumbers() == rhs.inputRunNumbers() && lhs.transmissionRuns() == rhs.transmissionRuns();
}

bool operator!=(ReductionWorkspaces const &lhs, ReductionWorkspaces const &rhs) { return !(lhs == rhs); }

ReductionWorkspaces workspaceNames(std::vector<std::string> const &inputRunNumbers,
                                   TransmissionRunPair const &transmissionRuns) {
  return ReductionWorkspaces(inputRunNumbers, transmissionRuns);
}

std::string postprocessedWorkspaceName(std::vector<std::vector<std::string> const *> const &summedRunNumbers) {
  auto summedRunList = map(summedRunNumbers, [](std::vector<std::string> const *summedRuns) -> std::string {
    return boost::algorithm::join(*summedRuns, "+");
  });
  return boost::algorithm::join(summedRunList, "_");
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
