// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReductionWorkspaces.h"
#include "Common/Map.h"

namespace MantidQt {
namespace CustomInterfaces {
ReductionWorkspaces::ReductionWorkspaces(
    // cppcheck-suppress passedByValue
    std::vector<std::string> inputRunNumbers,
    // cppcheck-suppress passedByValue
    TransmissionRunPair transmissionRuns)
    : m_inputRunNumbers(std::move(inputRunNumbers)),
      m_transmissionRuns(transmissionRuns), m_iVsLambda(), m_iVsQ(),
      m_iVsQBinned() {}

std::vector<std::string> const &ReductionWorkspaces::inputRunNumbers() const {
  return m_inputRunNumbers;
}

TransmissionRunPair const &ReductionWorkspaces::transmissionRuns() const {
  return m_transmissionRuns;
}

std::string const &ReductionWorkspaces::iVsLambda() const {
  return m_iVsLambda;
}

std::string const &ReductionWorkspaces::iVsQ() const { return m_iVsQ; }

std::string const &ReductionWorkspaces::iVsQBinned() const {
  return m_iVsQBinned;
}

void ReductionWorkspaces::setOutputNames(std::string iVsLambda,
                                         std::string iVsQ,
                                         std::string iVsQBinned) {
  m_iVsLambda = std::move(iVsLambda);
  m_iVsQ = std::move(iVsQ);
  m_iVsQBinned = std::move(iVsQBinned);
}

void ReductionWorkspaces::resetOutputNames() {
  m_iVsLambda = std::string();
  m_iVsQ = std::string();
  m_iVsQBinned = std::string();
}

bool ReductionWorkspaces::hasOutputName(std::string const &wsName) const {
  return m_iVsLambda == wsName || m_iVsQ == wsName || m_iVsQBinned == wsName;
}

void ReductionWorkspaces::renameOutput(std::string const &oldName,
                                       std::string const &newName) {
  if (m_iVsLambda == oldName)
    m_iVsLambda = newName;
  else if (m_iVsQ == oldName)
    m_iVsQ = newName;
  else if (m_iVsQBinned == oldName)
    m_iVsQBinned = newName;
}

bool operator==(ReductionWorkspaces const &lhs,
                ReductionWorkspaces const &rhs) {
  return lhs.inputRunNumbers() == rhs.inputRunNumbers() &&
         lhs.transmissionRuns() == rhs.transmissionRuns();
}

bool operator!=(ReductionWorkspaces const &lhs,
                ReductionWorkspaces const &rhs) {
  return !(lhs == rhs);
}

ReductionWorkspaces
workspaceNames(std::vector<std::string> const &inputRunNumbers,
               TransmissionRunPair const &transmissionRuns) {
  return ReductionWorkspaces(inputRunNumbers, transmissionRuns);
}

std::string postprocessedWorkspaceName(
    std::vector<std::vector<std::string> const *> const &summedRunNumbers) {
  auto summedRunList =
      map(summedRunNumbers,
          [](std::vector<std::string> const *summedRuns) -> std::string {
            return boost::algorithm::join(*summedRuns, "+");
          });
  return boost::algorithm::join(summedRunList, "_");
}
} // namespace CustomInterfaces
} // namespace MantidQt
