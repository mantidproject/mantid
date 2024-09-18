// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/ExtractQENSMembers.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/ArrayProperty.h"

#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>

namespace {
Mantid::Kernel::Logger g_log("ExtractQENSMembers");
}

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractQENSMembers)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ExtractQENSMembers::name() const { return "ExtractQENSMembers"; }

/// Algorithm's version for identification. @see Algorithm::version
int ExtractQENSMembers::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ExtractQENSMembers::category() const { return "Workflow\\MIDAS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ExtractQENSMembers::summary() const { return "Extracts the fit members from a QENS fit"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ExtractQENSMembers::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Optional),
                  "The input workspace used in the fit. Ignored if 'InputWorkspaces' "
                  "property is provided.");
  declareProperty(std::make_unique<ArrayProperty<std::string>>("InputWorkspaces", ""),
                  "List of the workspaces used in the fit.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("ResultWorkspace", "", Direction::Input),
                  "The result group workspace produced in a QENS fit.");
  declareProperty("RenameConvolvedMembers", false,
                  "If true, renames the n-th 'Convolution' member, to the n-th "
                  "supplied name in the ConvolvedMembers property.");
  declareProperty(std::make_unique<ArrayProperty<std::string>>("ConvolvedMembers"),
                  "A list of the names of the members which were convolved "
                  "before being output by the fit routine. These must be "
                  "provided in the same order as originally provided to the "
                  "fit.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace group, containing the fit members.");
}

std::map<std::string, std::string> ExtractQENSMembers::validateInputs() {
  std::map<std::string, std::string> errors;
  std::vector<std::string> workspaceNames = getProperty("InputWorkspaces");
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  if (workspaceNames.empty() && !inputWorkspace)
    errors["InputWorkspace"] = "Neither the InputWorkspace or InputWorkspaces "
                               "property have been defined.";
  return errors;
}

void ExtractQENSMembers::exec() {
  auto inputWorkspaces = getInputWorkspaces();
  WorkspaceGroup_sptr resultWS = getProperty("ResultWorkspace");
  MatrixWorkspace_sptr initialWS = std::dynamic_pointer_cast<MatrixWorkspace>(resultWS->getItem(0));
  const auto qValues = getQValues(inputWorkspaces);
  auto members = getAxisLabels(initialWS, 1);

  bool renameConvolved = getProperty("RenameConvolvedMembers");
  if (renameConvolved)
    members = renameConvolvedMembers(members, getProperty("ConvolvedMembers"));

  auto memberWorkspaces = createMembersWorkspaces(initialWS, members);

  for (auto i = 1u; i < resultWS->size(); ++i)
    appendToMembers(std::dynamic_pointer_cast<MatrixWorkspace>(resultWS->getItem(i)), memberWorkspaces);
  setNumericAxis(memberWorkspaces, qValues, 1);

  std::string outputWSName = getProperty("OutputWorkspace");
  auto workspaceNames = addMembersToADS(members, memberWorkspaces, outputWSName);
  setProperty("OutputWorkspace", groupWorkspaces(workspaceNames));
}

std::vector<MatrixWorkspace_sptr> ExtractQENSMembers::getInputWorkspaces() const {
  const std::vector<std::string> workspaceNames = getProperty("InputWorkspaces");
  std::vector<MatrixWorkspace_sptr> workspaces;

  if (!workspaceNames.empty()) {
    workspaces.reserve(workspaceNames.size());
    auto &ADS = AnalysisDataService::Instance();
    std::transform(workspaceNames.cbegin(), workspaceNames.cend(), std::back_inserter(workspaces),
                   [&ADS](const auto &name) { return ADS.retrieveWS<MatrixWorkspace>(name); });
  } else
    workspaces.emplace_back(getProperty("InputWorkspace"));
  return workspaces;
}

/**
 * Extracts the Q-Values from the specified workspace.
 *
 * @param workspaces The workspaces whose Q-Values to extract.
 * @return            The extracted Q-Values.
 */
std::vector<double> ExtractQENSMembers::getQValues(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  std::vector<double> qValues;

  for (const auto &workspace : workspaces) {
    auto getQs = createChildAlgorithm("GetQsInQENSData", -1.0, -1.0, false);
    getQs->setProperty("InputWorkspace", workspace);
    getQs->executeAsChildAlg();
    const std::vector<double> values = getQs->getProperty("Qvalues");
    qValues.insert(std::end(qValues), std::begin(values), std::end(values));
  }
  return qValues;
}

/**
 * Retrieves the axis labels from the axis with the specified index, in the
 * specified workspace.
 *
 * @param workspace The workspace whose axis labels to retrieve.
 * @param axisIndex The index of the axis whose labels retrieve.
 * @return          The retrieved axis labels.
 */
std::vector<std::string> ExtractQENSMembers::getAxisLabels(const MatrixWorkspace_sptr &workspace,
                                                           size_t axisIndex) const {
  auto axis = workspace->getAxis(axisIndex);
  std::vector<std::string> labels;
  labels.reserve(axis->length());

  for (auto i = 0u; i < axis->length(); ++i)
    labels.emplace_back(axis->label(i));
  return labels;
}

/**
 * Renames the convolved members in the specified vector of members, to the
 * respective names in the specified new names vector.
 *
 * @param members   A vector of the members.
 * @param newNames  The names to use in renaming.
 * @return          A vector of the members, with the convolved members renamed.
 */
std::vector<std::string> ExtractQENSMembers::renameConvolvedMembers(const std::vector<std::string> &members,
                                                                    const std::vector<std::string> &newNames) const {
  std::vector<std::string> newMembers;
  newMembers.reserve(members.size());
  auto index = 0u;

  for (const auto &member : members) {
    if (member == "Convolution" && index < newNames.size())
      newMembers.emplace_back(newNames[index++]);
    else
      newMembers.emplace_back(member);
  }
  return newMembers;
}

/**
 * Extracts the specified spectrum from the specified spectrum.
 *
 * @param inputWS   The workspace whose spectrum to extract.
 * @param spectrum  The spectrum to extract.
 * @return          A workspace containing the extracted spectrum.
 */
MatrixWorkspace_sptr ExtractQENSMembers::extractSpectrum(const MatrixWorkspace_sptr &inputWS, size_t spectrum) {
  auto extractAlg = createChildAlgorithm("ExtractSpectra", -1.0, -1.0, false);
  extractAlg->setProperty("InputWorkspace", inputWS);
  extractAlg->setProperty("OutputWorkspace", "__extracted");
  extractAlg->setProperty("StartWorkspaceIndex", boost::numeric_cast<int>(spectrum));
  extractAlg->setProperty("EndWorkspaceIndex", boost::numeric_cast<int>(spectrum));
  extractAlg->executeAsChildAlg();
  return extractAlg->getProperty("OutputWorkspace");
}

/**
 * Appends the spectra of a specified workspace to another specified input
 * workspace.
 *
 * @param inputWS           The input workspace to append to.
 * @param spectraWorkspace  The workspace whose spectra to append.
 */
MatrixWorkspace_sptr ExtractQENSMembers::appendSpectra(const MatrixWorkspace_sptr &inputWS,
                                                       const MatrixWorkspace_sptr &spectraWorkspace) {
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
WorkspaceGroup_sptr ExtractQENSMembers::groupWorkspaces(const std::vector<std::string> &workspaceNames) {
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
std::vector<MatrixWorkspace_sptr> ExtractQENSMembers::createMembersWorkspaces(const MatrixWorkspace_sptr &initialWS,
                                                                              const std::vector<std::string> &members) {
  std::vector<MatrixWorkspace_sptr> memberWorkspaces;
  memberWorkspaces.reserve(members.size());
  for (auto i = 0u; i < members.size(); ++i)
    memberWorkspaces.emplace_back(extractSpectrum(initialWS, i));
  return memberWorkspaces;
}

/**
 * Appends the n-th spectra in the specified result workspace to the n-th
 * specified member workspace.
 *
 * @param resultWS  The result workspace.
 * @param members   A vector containing the member workspaces.
 */
void ExtractQENSMembers::appendToMembers(const MatrixWorkspace_sptr &resultWS,
                                         std::vector<Mantid::API::MatrixWorkspace_sptr> &members) {
  for (auto i = 0u; i < members.size(); ++i)
    members[i] = appendSpectra(members[i], extractSpectrum(resultWS, i));
}

/**
 * Creates and sets a numeric axis, filled with the specified values, on each of
 * the specified workspaces at the specified axis index.
 *
 * @param workspaces  The workspaces whose axis to set.
 * @param values      The values used to create the numeric axis.
 * @param axisIndex   The index of the axis to set.
 */
void ExtractQENSMembers::setNumericAxis(const std::vector<MatrixWorkspace_sptr> &workspaces,
                                        const std::vector<double> &values, size_t axisIndex) const {
  auto qAxis = NumericAxis(values.size());
  for (auto i = 0u; i < values.size(); ++i)
    qAxis.setValue(i, values[i]);

  for (auto &workspace : workspaces) {
    workspace->replaceAxis(axisIndex, std::make_unique<NumericAxis>(qAxis));
    workspace->setYUnitLabel("MomentumTransfer");
  }
}

/**
 * Adds the specified member workspaces to the analysis data service.
 *
 * @param members           The name of the members; corresponds to each member
 *                          workspace.
 * @param memberWorkspaces  The member workspaces.
 * @param outputWSName      The name of the output workspace; used as a prefix.
 * @return                  A vector containing the names of the added
 *                          workspaces.
 */
std::vector<std::string>
ExtractQENSMembers::addMembersToADS(const std::vector<std::string> &members,
                                    const std::vector<Mantid::API::MatrixWorkspace_sptr> &memberWorkspaces,
                                    const std::string &outputWSName) {
  std::unordered_map<std::string, size_t> nameCounts;
  std::vector<std::string> workspaceNames;
  workspaceNames.reserve(members.size());

  for (auto i = 0u; i < members.size(); ++i) {
    std::string count = "";

    if (nameCounts.find(members[i]) == nameCounts.end())
      nameCounts[members[i]] = 1;
    else
      count = std::to_string(++nameCounts[members[i]]);

    const auto wsName = outputWSName + "_" + members[i] + count;
    AnalysisDataService::Instance().addOrReplace(wsName, memberWorkspaces[i]);
    workspaceNames.emplace_back(wsName);
  }
  return workspaceNames;
}

} // namespace Mantid::Algorithms
