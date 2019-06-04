// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobRunner.h"
#include "BatchJobAlgorithm.h"
#include "GroupProcessingAlgorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "RowProcessingAlgorithm.h"

#include <numeric>

namespace MantidQt {
namespace CustomInterfaces {

namespace { // unnamed

int countItemsForLocation(
    ReductionJobs const &jobs,
    MantidWidgets::Batch::RowLocation const &location,
    std::vector<MantidWidgets::Batch::RowLocation> const &locations,
    Item::ItemCountFunction countFunction) {
  if (!jobs.validItemAtPath(location))
    return 0;

  // Rows have a single processing step but we want to ignore them if their
  // parent group is also in the selection or they will be counted twice.
  if (isRowLocation(location) && containsPath(locations, {groupOf(location)}))
    return 0;

  auto const &item = jobs.getItemFromPath(location);
  return (item.*countFunction)();
}
} // unnamed namespace

using API::IConfiguredAlgorithm_sptr;

BatchJobRunner::BatchJobRunner(Batch batch)
    : m_batch(std::move(batch)), m_isProcessing(false), m_isAutoreducing(false),
      m_reprocessFailed(false), m_processAll(false) {}

bool BatchJobRunner::isProcessing() const { return m_isProcessing; }

bool BatchJobRunner::isAutoreducing() const { return m_isAutoreducing; }

int BatchJobRunner::itemsInSelection(
    Item::ItemCountFunction countFunction) const {
  auto const &jobs = m_batch.runsTable().reductionJobs();
  auto const &locations = m_rowLocationsToProcess;
  return std::accumulate(
      locations.cbegin(), locations.cend(), 0,
      [&jobs, &locations, countFunction](
          int &count, MantidWidgets::Batch::RowLocation const &location) {
        return count +
               countItemsForLocation(jobs, location, locations, countFunction);
      });
}

int BatchJobRunner::percentComplete() const {
  // If processing everything, get the percent from the whole table
  if (m_processAll)
    return MantidQt::CustomInterfaces::percentComplete(
        m_batch.runsTable().reductionJobs());

  // If processing a selection but there is nothing to process, return 100%
  auto const totalItems = itemsInSelection(&Item::totalItems);
  if (totalItems == 0)
    return 100;

  // Otherwise calculate the percentage of completed items in the selection
  auto const completedItems = itemsInSelection(&Item::completedItems);
  return completedItems * 100 / totalItems;
}

void BatchJobRunner::reductionResumed() {
  // Cache the set of rows to process when the user starts a reduction
  m_rowLocationsToProcess = m_batch.selectedRowLocations();
  m_isProcessing = true;
  // If the user has manually selected failed rows, reprocess them; otherwise
  // skip them. If we're autoreducing, or there are no selected rows, process
  // everything
  if (m_rowLocationsToProcess.empty()) {
    // Nothing selected so process everything. Skip failed rows.
    m_processAll = true;
    m_reprocessFailed = false;
  } else {
    // User has manually selected items so only process the selection (unless
    // autoreducing). Also reprocess failed items.
    m_processAll = m_isAutoreducing;
    m_reprocessFailed = !m_isAutoreducing;
  }

  m_batch.resetSkippedItems();
}

void BatchJobRunner::reductionPaused() { m_isProcessing = false; }

void BatchJobRunner::autoreductionResumed() {
  m_rowLocationsToProcess.clear();
  m_isAutoreducing = true;
  m_isProcessing = true;
  m_reprocessFailed = true;
  m_processAll = true;
  m_batch.resetSkippedItems();
}

void BatchJobRunner::autoreductionPaused() { m_isAutoreducing = false; }

void BatchJobRunner::setReprocessFailedItems(bool reprocessFailed) {
  m_reprocessFailed = reprocessFailed;
}

template <typename T> bool BatchJobRunner::isSelected(T const &item) {
  return m_processAll || m_batch.isInSelection(item, m_rowLocationsToProcess);
}

bool BatchJobRunner::hasSelectedRowsRequiringProcessing(Group const &group) {
  // If the group itself is selected, consider its rows to also be selected
  if (m_processAll || isSelected(group))
    return true;

  for (auto const &row : group.rows()) {
    if (row && isSelected(row.get()) &&
        row->requiresProcessing(m_reprocessFailed))
      return true;
  }

  return false;
}

/** Get algorithms and related properties for processing a batch of rows and
 * groups in the table
 */
std::deque<IConfiguredAlgorithm_sptr> BatchJobRunner::getAlgorithms() {
  auto &groups =
      m_batch.mutableRunsTable().mutableReductionJobs().mutableGroups();
  for (auto &group : groups) {
    // If the group is selected, process all of its rows
    if (isSelected(group) && group.requiresProcessing(m_reprocessFailed))
      return algorithmsForProcessingRowsInGroup(group, true);
    // If the group has rows that are selected, process the selected rows
    if (hasSelectedRowsRequiringProcessing(group))
      return algorithmsForProcessingRowsInGroup(group, false);
    // If the group's requires postprocessing, do it
    if (isSelected(group) && group.requiresPostprocessing(m_reprocessFailed))
      return algorithmForPostprocessingGroup(group);
  }
  return std::deque<IConfiguredAlgorithm_sptr>();
}

/** Add the algorithms and related properties for postprocessing a group
 * @param group : the group to get the row algorithms for
 * @returns : the list of configured algorithms
 */
std::deque<IConfiguredAlgorithm_sptr>
BatchJobRunner::algorithmForPostprocessingGroup(Group &group) {
  auto algorithm = createConfiguredAlgorithm(m_batch, group);
  auto algorithms = std::deque<IConfiguredAlgorithm_sptr>();
  algorithms.emplace_back(std::move(algorithm));
  return algorithms;
}

/** Add the algorithms and related properties for processing all the rows
 * in a group
 * @param group : the group to get the row algorithms for
 * @param processAll : if true, include all rows in the group;
 * otherwise just include selected rows
 * @returns : the list of configured algorithms
 */
std::deque<IConfiguredAlgorithm_sptr>
BatchJobRunner::algorithmsForProcessingRowsInGroup(Group &group,
                                                   bool processAll) {
  auto algorithms = std::deque<IConfiguredAlgorithm_sptr>();
  auto &rows = group.mutableRows();
  for (auto &row : rows) {
    if (row && row->requiresProcessing(m_reprocessFailed) &&
        (processAll || isSelected(row.get())))
      addAlgorithmForProcessingRow(row.get(), algorithms);
  }
  return algorithms;
}

/** Add the algorithm and related properties for processing a row
 * @param row : the row to get the configured algorithm for
 * @param algorithms : the list of configured algorithms to add this row to
 * @returns : true if algorithms were added, false if there was nothing to do
 */
void BatchJobRunner::addAlgorithmForProcessingRow(
    Row &row, std::deque<IConfiguredAlgorithm_sptr> &algorithms) {
  auto algorithm = createConfiguredAlgorithm(m_batch, row);
  algorithms.emplace_back(std::move(algorithm));
}

void BatchJobRunner::algorithmStarted(IConfiguredAlgorithm_sptr algorithm) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  jobAlgorithm->item()->resetOutputNames();
  jobAlgorithm->item()->setRunning();
}

void BatchJobRunner::algorithmComplete(IConfiguredAlgorithm_sptr algorithm) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);

  // The workspaces are not in the ADS by default, so add them
  for (auto &kvp : jobAlgorithm->outputWorkspaceNameToWorkspace()) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(kvp.first,
                                                              kvp.second);
  }

  jobAlgorithm->item()->setOutputNames(jobAlgorithm->outputWorkspaceNames());
  jobAlgorithm->item()->setSuccess();
}

void BatchJobRunner::algorithmError(IConfiguredAlgorithm_sptr algorithm,
                                    std::string const &message) {
  auto jobAlgorithm =
      boost::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto *item = jobAlgorithm->item();
  item->resetOutputNames();
  item->setError(message);
  // Mark the item as skipped so we don't reprocess it in the current round of
  // reductions.
  item->setSkipped(true);
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
  if (group.hasPostprocessing())
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
