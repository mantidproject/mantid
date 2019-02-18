// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobRunner.h"
#include "BatchJobAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "RowProperties.h"

namespace MantidQt {
namespace CustomInterfaces {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::Workspace_sptr;

BatchJobRunner::BatchJobRunner(Batch batch)
    : m_batch(std::move(batch)), m_isProcessing(false), m_isAutoreducing(false),
      m_reprocessFailed(false), m_processAll(false) {}

bool BatchJobRunner::isProcessing() const { return m_isProcessing; }

bool BatchJobRunner::isAutoreducing() const { return m_isAutoreducing; }

void BatchJobRunner::resumeReduction() {
  m_isProcessing = true;
  // If the user has manually selected failed rows, reprocess them; otherwise
  // skip them
  m_reprocessFailed = m_batch.hasSelection();
  // If there are no selected rows, process everything
  m_processAll = !m_batch.hasSelection();
}

void BatchJobRunner::reductionPaused() { m_isProcessing = false; }

void BatchJobRunner::resumeAutoreduction() {
  m_isAutoreducing = true;
  m_isProcessing = true;
  m_reprocessFailed = true;
  m_processAll = true;
}

void BatchJobRunner::autoreductionPaused() { m_isAutoreducing = false; }

void BatchJobRunner::setReprocessFailedItems(bool reprocessFailed) {
  m_reprocessFailed = reprocessFailed;
}

/** Get algorithms and related properties for processing rows and groups
 * in the table
 */
std::deque<IConfiguredAlgorithm_sptr> BatchJobRunner::getAlgorithms() {
  auto algorithms = std::deque<IConfiguredAlgorithm_sptr>();
  auto &groups =
      m_batch.mutableRunsTable().mutableReductionJobs().mutableGroups();
  for (auto &group : groups) {
    addAlgorithmsForRowsInGroup(group, algorithms);
  }
  return algorithms;
}

/** Add the algorithms and related properties for processing all the rows
 * in a group
 * @param group : the group to get the row algorithms for
 * @param algorithms : the list of configured algorithms to add this group's
 * rows to
 */
void BatchJobRunner::addAlgorithmsForRowsInGroup(
    Group &group, std::deque<IConfiguredAlgorithm_sptr> &algorithms) {
  auto &rows = group.mutableRows();
  for (auto &row : rows) {
    if (row && row->requiresProcessing(m_reprocessFailed) &&
        (m_processAll || m_batch.isSelected(row.get()))) {
      addAlgorithmForRow(row.get(), algorithms);
    }
  }
}

/** Add the algorithm and related properties for processing a row
 * @param row : the row to get the configured algorithm for
 * @param algorithms : the list of configured algorithms to add this row to
 */
void BatchJobRunner::addAlgorithmForRow(
    Row &row, std::deque<IConfiguredAlgorithm_sptr> &algorithms) {
  // Create the algorithm
  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryISISLoadAndProcess");
  alg->setChild(true);

  // Set up input properties
  auto properties = AlgorithmRuntimeProps();
  RowProperties::updateEventProperties(properties, m_batch.slicing());
  RowProperties::updateExperimentProperties(properties, m_batch.experiment());
  RowProperties::updatePerThetaDefaultProperties(
      properties, m_batch.defaultsForTheta(row.theta()));
  RowProperties::updateInstrumentProperties(properties, m_batch.instrument());
  RowProperties::updateRowProperties(properties, row);

  // Store expected output property names. Must be in the correct order for
  // Row::algorithmComplete
  std::vector<std::string> outputWorkspaceProperties = {
      "OutputWorkspaceWavelength", "OutputWorkspace", "OutputWorkspaceBinned"};

  // Add the configured algorithm to the list
  auto jobAlgorithm = boost::make_shared<BatchJobAlgorithm>(
      alg, properties, outputWorkspaceProperties, &row);
  algorithms.emplace_back(std::move(jobAlgorithm));
}

void BatchJobRunner::algorithmStarted(IConfiguredAlgorithm_sptr algorithm) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  jobAlgorithm->item()->algorithmStarted();
}

void BatchJobRunner::algorithmComplete(IConfiguredAlgorithm_sptr algorithm) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  jobAlgorithm->item()->algorithmComplete(jobAlgorithm->outputWorkspaceNames());

  for (auto &kvp : jobAlgorithm->outputWorkspaceNameToWorkspace()) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(kvp.first,
                                                              kvp.second);
  }
}

void BatchJobRunner::algorithmError(IConfiguredAlgorithm_sptr algorithm,
                                    std::string const &message) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  jobAlgorithm->item()->algorithmError(message);
}

std::vector<std::string> BatchJobRunner::algorithmOutputWorkspacesToSave(
    IConfiguredAlgorithm_sptr algorithm) const {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto item = jobAlgorithm->item();

  if (item->isGroup())
    return getWorkspacesToSave(dynamic_cast<Group &>(*item));
  else
    return getWorkspacesToSave(dynamic_cast<Row &>(*item));

  return std::vector<std::string>();
}

std::vector<std::string>
BatchJobRunner::getWorkspacesToSave(Group const &group) const {
  return std::vector<std::string>{group.postprocessedWorkspaceName()};
}

std::vector<std::string>
BatchJobRunner::getWorkspacesToSave(Row const &row) const {
  // Get the output workspaces for the given row. Note that we only save
  // workspaces for the row if the group does not have postprocessing, because
  // in that case users just want to see the postprocessed output instead.
  auto workspaces = std::vector<std::string>();
  auto const group = m_batch.runsTable().reductionJobs().getParentGroup(row);
  if (group.requiresPostprocessing())
    return workspaces;

  // We currently only save the binned workspace in Q
  workspaces.push_back(row.reducedWorkspaceNames().iVsQBinned());
  return workspaces;
}

void BatchJobRunner::notifyWorkspaceDeleted(std::string const &wsName) {
  // Reset the state for the relevant row if the workspace was one of our
  // outputs
  auto item = m_batch.getItemWithOutputWorkspaceOrNone(wsName);
  if (item)
    item->resetState();
}

void BatchJobRunner::notifyWorkspaceRenamed(std::string const &oldName,
                                            std::string const &newName) {
  // Update the workspace name in the model, if it is one of our outputs
  auto item = m_batch.getItemWithOutputWorkspaceOrNone(oldName);
  if (item)
    item->renameOutputWorkspace(oldName, newName);
}

void BatchJobRunner::notifyAllWorkspacesDeleted() {
  // All output workspaces will be deleted so reset all rows and groups
  m_batch.resetState();
}
} // namespace CustomInterfaces
} // namespace MantidQt
