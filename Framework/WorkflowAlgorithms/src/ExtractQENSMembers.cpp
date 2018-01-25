#include "MantidWorkflowAlgorithms/ExtractQENSMembers.h"

#include "MantidAPI/Axis.h"

#include <boost/numeric/conversion/cast.hpp>

namespace {
Mantid::Kernel::Logger g_log("ExtractQENSMembers");
}

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractQENSMembers)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ExtractQENSMembers::name() const {
  return "ExtractQENSMembers";
}

/// Algorithm's version for identification. @see Algorithm::version
int ExtractQENSMembers::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ExtractQENSMembers::category() const {
  return "Workflow\\MIDAS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ExtractQENSMembers::summary() const {
  return "Extracts the fit members from a QENS fit";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ExtractQENSMembers::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "The input workspace used in the fit.");
  declareProperty(make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "ResultWorkspace", "", Direction::Input),
                  "The result group workspace produced in a QENS fit.");
  declareProperty(make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The output workspace group, containing the fit members.");
}

void ExtractQENSMembers::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  WorkspaceGroup_sptr resultWS = getProperty("ResultWorkspace");
  MatrixWorkspace_sptr initialWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(resultWS->getItem(0));
  auto qValues = getQValues(getProperty("InputWorkspace"));
  auto members =
      getAxisLabels(boost::dynamic_pointer_cast<MatrixWorkspace>(initialWS), 1);
  auto memberWorkspaces = createMembersWorkspaces(initialWS, members);

  for (size_t i = 1u; i < resultWS->size(); ++i)
    appendToMembers(
        boost::dynamic_pointer_cast<MatrixWorkspace>(resultWS->getItem(i)),
        memberWorkspaces);

  std::string outputWSName = getProperty("OutputWorkspace");
  auto workspaceNames =
      addMembersToADS(members, memberWorkspaces, outputWSName);
  setProperty("OutputWorkspace", groupWorkspaces(workspaceNames));
}

/**
 * Extracts the Q-Values from the specified workspace.
 *
 * @param workspace The workspace whose Q-Values to extract.
 * @return          The extracted Q-Values.
 */
std::vector<double>
ExtractQENSMembers::getQValues(MatrixWorkspace_sptr workspace) {
  auto getQs = createChildAlgorithm("GetQsInQENSData", -1.0, -1.0, false);
  getQs->setProperty("InputWorkspace", workspace);
  getQs->executeAsChildAlg();
  return getQs->getProperty("Qvalues");
}

/**
 * Retrieves the axis labels from the axis with the specified index, in the
 * specified workspace.
 *
 * @param workspace The workspace whose axis labels to retrieve.
 * @param axisIndex The index of the axis whose labels retrieve.
 * @return          The retrieved axis labels.
 */
std::vector<std::string>
ExtractQENSMembers::getAxisLabels(MatrixWorkspace_sptr workspace,
                                  size_t axisIndex) const {
  auto axis = workspace->getAxis(axisIndex);
  std::vector<std::string> labels;
  labels.reserve(axis->length());

  for (size_t i = 0u; i < axis->length(); ++i)
    labels.emplace_back(axis->label(i));
  return labels;
}

/**
 * Extracts the specified spectrum from the specified spectrum.
 *
 * @param inputWS   The workspace whose spectrum to extract.
 * @param spectrum  The spectrum to extract.
 * @return          A workspace containing the extracted spectrum.
 */
MatrixWorkspace_sptr
ExtractQENSMembers::extractSpectrum(MatrixWorkspace_sptr inputWS,
                                    size_t spectrum) {
  auto extractAlg = createChildAlgorithm("ExtractSpectra", -1.0, -1.0, false);
  extractAlg->setProperty("InputWorkspace", inputWS);
  extractAlg->setProperty("OutputWorkspace", "__extracted");
  extractAlg->setProperty("StartWorkspaceIndex",
                          boost::numeric_cast<int>(spectrum));
  extractAlg->setProperty("EndWorkspaceIndex",
                          boost::numeric_cast<int>(spectrum));
  return extractAlg->getProperty("OutputWorkspace");
}

/**
 * Appends the spectra of a specified workspace to another specified input
 * workspace.
 *
 * @param inputWS           The input workspace to append to.
 * @param spectraWorkspace  The workspace whose spectra to append.
 */
MatrixWorkspace_sptr
ExtractQENSMembers::appendSpectra(MatrixWorkspace_sptr inputWS,
                                  MatrixWorkspace_sptr spectraWorkspace) {
  auto appendAlg = createChildAlgorithm("AppendSpectra", -1.0, -1.0, false);
  appendAlg->setProperty("InputWorkspace1", inputWS);
  appendAlg->setProperty("InputWorkspace2", spectraWorkspace);
  appendAlg->setProperty("OutputWorkspace", inputWS);
  appendAlg->executeAsChildAlg();
  return appendAlg->getProperty("OutputWorkspace");
}

/**
 * Groups the workspaces with the specified name.
 *
 * @param workspaceNames  A vector of the names of the workspaces to group.
 * @return                The group workspace.
 */
WorkspaceGroup_sptr ExtractQENSMembers::groupWorkspaces(
    const std::vector<std::string> &workspaceNames) {
  auto groupAlg = createChildAlgorithm("GroupWorkspaces", -1.0, -1.0, false);
  groupAlg->setProperty("InputWorkspaces", workspaceNames);
  groupAlg->setProperty("OutputWorkspace", "__grouped");
  groupAlg->execute();
  return groupAlg->getProperty("OutputWorkspace");
}

/**
 * Creates the member workspaces from an initial result workspace and the member
 * names.
 *
 * @param initialWS The initial result workspace.
 * @param members   A vector of the member names.
 * @return          A vector of the created members workspaces.
 */
std::vector<MatrixWorkspace_sptr> ExtractQENSMembers::createMembersWorkspaces(
    MatrixWorkspace_sptr initialWS, const std::vector<std::string> &members) {
  std::vector<MatrixWorkspace_sptr> memberWorkspaces;
  memberWorkspaces.reserve(members.size());
  for (size_t i = 0u; i < members.size(); ++i)
    memberWorkspaces.push_back(extractSpectrum(initialWS, i));
  return memberWorkspaces;
}

void ExtractQENSMembers::appendToMembers(
    MatrixWorkspace_sptr resultWS,
    const std::vector<Mantid::API::MatrixWorkspace_sptr> &members) {
  for (size_t i = 0u; i < members.size(); ++i)
    appendSpectra(members[i], extractSpectrum(resultWS, i));
}

std::vector<std::string> ExtractQENSMembers::addMembersToADS(
    const std::vector<std::string> &members,
    const std::vector<Mantid::API::MatrixWorkspace_sptr> &memberWorkspaces,
    const std::string &outputWSName) {
  std::vector<std::string> workspaceNames;
  workspaceNames.reserve(members.size());
  for (size_t i = 0u; i < members.size(); ++i) {
    const auto name = outputWSName + "_" + members[i];
    AnalysisDataService::Instance().addOrReplace(name, memberWorkspaces[i]);
    workspaceNames.emplace_back(name);
  }
  return workspaceNames;
}

} // namespace Algorithms
} // namespace Mantid