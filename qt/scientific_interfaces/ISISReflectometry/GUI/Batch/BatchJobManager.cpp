// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobManager.h"
#include "BatchJobAlgorithm.h"
#include "GroupProcessingAlgorithm.h"
#include "IReflAlgorithmFactory.h"
#include "Reduction/PreviewRow.h"
#include "Reduction/RowExceptions.h"
#include "ReflAlgorithmFactory.h"
#include "RowPreprocessingAlgorithm.h"
#include "RowProcessingAlgorithm.h"

#include <algorithm>
#include <numeric>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace { // unnamed

int countItemsForLocation(ReductionJobs const &jobs, MantidWidgets::Batch::RowLocation const &location,
                          std::vector<MantidWidgets::Batch::RowLocation> const &locations,
                          Item::ItemCountFunction countFunction) {
  if (!jobs.validItemAtPath(location))
    return 0;

  // Rows have a single processing step but we want to ignore them if their
  // parent group is also in the selection or they will be counted twice.
  if (isRowLocation(location) && containsPath(locations, MantidWidgets::Batch::RowLocation({groupOf(location)})))
    return 0;

  auto const &item = jobs.getItemFromPath(location);
  return (item.*countFunction)();
}
} // unnamed namespace

using API::IConfiguredAlgorithm_sptr;

BatchJobManager::BatchJobManager(IBatch &batch, std::unique_ptr<IReflAlgorithmFactory> algFactory)
    : m_batch(batch), m_algFactory(std::move(algFactory)), m_isProcessing(false), m_isAutoreducing(false),
      m_reprocessFailed(false), m_processAll(false), m_processPartial(false) {
  // TODO Pass IJobRunner into this class and move job execution here instead of in the presenter
  if (!m_algFactory)
    m_algFactory = std::make_unique<ReflAlgorithmFactory>(m_batch);
}

bool BatchJobManager::isProcessing() const { return m_isProcessing; }

bool BatchJobManager::isAutoreducing() const { return m_isAutoreducing; }

int BatchJobManager::itemsInSelection(Item::ItemCountFunction countFunction) const {
  auto const &jobs = m_batch.runsTable().reductionJobs();
  auto const &locations = m_rowLocationsToProcess;
  return std::accumulate(
      locations.cbegin(), locations.cend(), 0,
      [&jobs, &locations, countFunction](int const count, MantidWidgets::Batch::RowLocation const &location) {
        return count + countItemsForLocation(jobs, location, locations, countFunction);
      });
}

int BatchJobManager::percentComplete() const {
  // If processing everything, get the percent from the whole table
  if (m_processAll)
    return MantidQt::CustomInterfaces::ISISReflectometry::percentComplete(m_batch.runsTable().reductionJobs());

  // If processing a selection but there is nothing to process, return 100%
  auto const totalItems = itemsInSelection(&Item::totalItems);
  if (totalItems == 0)
    return 100;

  // Otherwise calculate the percentage of completed items in the selection
  auto const completedItems = itemsInSelection(&Item::completedItems);
  return completedItems * 100 / totalItems;
}

void BatchJobManager::notifyReductionResumed() {
  // Cache the set of rows to process when the user starts a reduction
  m_rowLocationsToProcess = m_batch.selectedRowLocations();
  m_isProcessing = true;
  // If the user has manually selected failed rows, reprocess them; otherwise
  // skip them. If we're autoreducing, or there are no selected rows, process
  // everything
  if (m_rowLocationsToProcess.empty()) {
    // Nothing selected so process everything. Skip failed rows.
    m_processAll = true;
    m_processPartial = false;
    m_reprocessFailed = false;
  } else {
    // User has manually selected items so only process the selection (unless
    // autoreducing). Also reprocess failed items.
    m_processAll = m_isAutoreducing;
    m_processPartial = false;
    m_reprocessFailed = !m_isAutoreducing;
    if (!m_processAll) {
      // Check whether a given group is in the selection. If not then check
      // the group's rows to determine whether it will be partially processed,
      // i.e. if it has some, but not all, rows selected
      std::map<const int, size_t> selectedRowsPerGroup;
      for (auto location : m_rowLocationsToProcess) {
        auto const groupIndex = groupOf(location);
        auto const totalRowsInGroup = getNumberOfInitialisedRowsInGroup(groupIndex);
        if (isGroupLocation(location)) {
          selectedRowsPerGroup[groupIndex] = totalRowsInGroup;
        } else if (selectedRowsPerGroup[groupIndex] < totalRowsInGroup) {
          ++selectedRowsPerGroup[groupIndex];
        }
      }
      for (const auto &groupIndexCountPair : selectedRowsPerGroup) {
        auto const groupIndex = groupIndexCountPair.first;
        auto const numSelected = groupIndexCountPair.second;
        m_processPartial = numSelected < getNumberOfInitialisedRowsInGroup(groupIndex);
        if (m_processPartial) {
          break;
        }
      }
    }
  }
  m_batch.resetSkippedItems();
}

void BatchJobManager::notifyReductionPaused() {
  m_isProcessing = false;
  m_rowLocationsToProcess.clear();
}

void BatchJobManager::notifyAutoreductionResumed() {
  m_isAutoreducing = true;
  m_reprocessFailed = true;
  m_processAll = true;
  m_processPartial = false;
  m_batch.resetSkippedItems();
}

void BatchJobManager::notifyAutoreductionPaused() {
  m_isAutoreducing = false;
  m_rowLocationsToProcess.clear();
}

void BatchJobManager::setReprocessFailedItems(bool reprocessFailed) { m_reprocessFailed = reprocessFailed; }

template <typename T> bool BatchJobManager::isSelected(T const &item) {
  return m_processAll || m_batch.isInSelection(item, m_rowLocationsToProcess);
}

bool BatchJobManager::hasSelectedRowsRequiringProcessing(Group const &group) {
  // If the group itself is selected, consider its rows to also be selected
  auto processAllRowsInGroup = (m_processAll || isSelected(group));

  return std::any_of((group.rows()).cbegin(), (group.rows()).cend(), [this, &processAllRowsInGroup](const auto &row) {
    return (row && (processAllRowsInGroup || isSelected(row.get())) && row->requiresProcessing(m_reprocessFailed));
  });
}

/** Get algorithms and related properties for processing a batch of rows and
 * groups in the table
 */
std::deque<IConfiguredAlgorithm_sptr> BatchJobManager::getAlgorithms() {
  auto &groups = m_batch.mutableRunsTable().mutableReductionJobs().mutableGroups();
  for (auto &group : groups) {
    auto algorithms = std::deque<IConfiguredAlgorithm_sptr>{};
    if (isSelected(group) && group.requiresProcessing(m_reprocessFailed)) {
      // If the group is selected, process all of its rows
      algorithms = algorithmsForProcessingRowsInGroup(group, true);
    } else if (hasSelectedRowsRequiringProcessing(group)) {
      // If the group has rows that are selected, process the selected rows
      algorithms = algorithmsForProcessingRowsInGroup(group, false);
    } else if (isSelected(group) && group.requiresPostprocessing(m_reprocessFailed)) {
      // If the group's requires postprocessing, do it
      algorithms = algorithmForPostprocessingGroup(group);
    }
    // If we have valid algorithms, return now; otherwise continue to the next group
    if (algorithms.size() > 0) {
      return algorithms;
    }
  }
  return std::deque<IConfiguredAlgorithm_sptr>();
}

/** Add the algorithms and related properties for postprocessing a group
 * @param group : the group to get the row algorithms for
 * @returns : the list of configured algorithms
 */
std::deque<IConfiguredAlgorithm_sptr> BatchJobManager::algorithmForPostprocessingGroup(Group &group) {
  auto algorithm = GroupProcessing::createConfiguredAlgorithm(m_batch, group);
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
std::deque<IConfiguredAlgorithm_sptr> BatchJobManager::algorithmsForProcessingRowsInGroup(Group &group,
                                                                                          bool processAll) {
  auto algorithms = std::deque<IConfiguredAlgorithm_sptr>();
  auto &rows = group.mutableRows();
  for (auto &row : rows) {
    if (row && row->requiresProcessing(m_reprocessFailed) && (processAll || isSelected(row.get())))
      addAlgorithmForProcessingRow(row.get(), algorithms);
  }
  return algorithms;
}

/** Add the algorithm and related properties for processing a row
 * @param row : the row to get the configured algorithm for
 * @param algorithms : the list of configured algorithms to add this row to
 * @returns : true if algorithms were added, false if there was nothing to do
 */
void BatchJobManager::addAlgorithmForProcessingRow(Row &row, std::deque<IConfiguredAlgorithm_sptr> &algorithms) {
  IConfiguredAlgorithm_sptr algorithm;
  try {
    algorithm = m_algFactory->makeRowProcessingAlgorithm(row);
  } catch (MultipleRowsFoundException const &) {
    row.setError("The title and angle specified matches multiple rows in the Experiment Settings tab");
    // Mark the item as skipped so we don't reprocess it in the current round of
    // reductions.
    row.setSkipped(true);
    return;
  } catch (std::invalid_argument const &e) {
    row.setError("Error while setting algorithm properties: " + std::string(e.what()));
    row.setSkipped(true);
    return;
  }
  algorithms.emplace_back(std::move(algorithm));
}

std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> BatchJobManager::rowProcessingProperties() const {
  return RowProcessing::createAlgorithmRuntimeProps(m_batch);
}

void BatchJobManager::algorithmStarted(IConfiguredAlgorithm_sptr algorithm) {
  auto item = getRunsTableItem(algorithm);
  assert(item);
  item->resetOutputs();
  item->setRunning();
}

void BatchJobManager::algorithmComplete(IConfiguredAlgorithm_sptr algorithm) {
  auto item = getRunsTableItem(algorithm);
  assert(item);
  auto jobAlgorithm = std::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  jobAlgorithm->updateItem();
  item->setSuccess();
}

void BatchJobManager::algorithmError(IConfiguredAlgorithm_sptr algorithm, std::string const &message) {
  auto item = getRunsTableItem(algorithm);
  assert(item);
  item->resetOutputs();
  item->setError(message);
  // Mark the item as skipped so we don't reprocess it in the current round of
  // reductions.
  item->setSkipped(true);
}

boost::optional<Item &> BatchJobManager::getRunsTableItem(IConfiguredAlgorithm_sptr const &algorithm) {
  auto jobAlgorithm = std::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto *item = jobAlgorithm->item();
  if (!item || item->isPreview()) {
    return boost::none;
  }
  return *item;
}

std::vector<std::string> BatchJobManager::algorithmOutputWorkspacesToSave(IConfiguredAlgorithm_sptr algorithm,
                                                                          bool includeGrpRows) const {
  auto jobAlgorithm = std::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto item = jobAlgorithm->item();

  if (item->isGroup())
    return getWorkspacesToSave(dynamic_cast<Group &>(*item), includeGrpRows);
  else
    return getWorkspacesToSave(dynamic_cast<Row &>(*item));

  return std::vector<std::string>();
}

std::vector<std::string> BatchJobManager::getWorkspacesToSave(Group const &group, bool includeRows) const {
  auto workspaces = std::vector<std::string>{group.postprocessedWorkspaceName()};

  if (includeRows) {
    auto const &rows = group.rows();
    for (auto &row : rows) {
      if (row) {
        workspaces.emplace_back(row.get().reducedWorkspaceNames().iVsQBinned());
      }
    }
  }

  return workspaces;
}

std::vector<std::string> BatchJobManager::getWorkspacesToSave(Row const &row) const {
  // Get the output workspaces for the given row. Note that we only save
  // workspaces for the row if the group does not have postprocessing, because
  // in that case users just want to see the postprocessed output instead.
  auto workspaces = std::vector<std::string>();
  auto const *group = row.getParent();
  if (group && group->hasPostprocessing())
    return workspaces;

  // We currently only save the binned workspace in Q
  workspaces.emplace_back(row.reducedWorkspaceNames().iVsQBinned());
  return workspaces;
}

size_t BatchJobManager::getNumberOfInitialisedRowsInGroup(const int groupIndex) const {
  auto const &group = m_batch.runsTable().reductionJobs().groups()[groupIndex];
  return static_cast<int>(std::count_if(group.rows().cbegin(), group.rows().cend(),
                                        [](const boost::optional<Row> &row) { return row.is_initialized(); }));
}

boost::optional<Item const &> BatchJobManager::notifyWorkspaceDeleted(std::string const &wsName) {
  // Reset the state for the relevant row if the workspace was one of our
  // outputs
  auto item = m_batch.getItemWithOutputWorkspaceOrNone(wsName);
  if (item.is_initialized()) {
    item->resetState(false);
    return boost::optional<Item const &>(item.get());
  }
  return boost::none;
}

boost::optional<Item const &> BatchJobManager::notifyWorkspaceRenamed(std::string const &oldName,
                                                                      std::string const &newName) {
  // Update the workspace name in the model, if it is one of our outputs
  auto item = m_batch.getItemWithOutputWorkspaceOrNone(oldName);
  if (item.is_initialized()) {
    item->renameOutputWorkspace(oldName, newName);
    return boost::optional<Item const &>(item.get());
  }
  auto newItem = m_batch.getItemWithOutputWorkspaceOrNone(newName);
  if (newItem.is_initialized()) {
    newItem->resetState();
    return boost::optional<Item const &>(newItem.get());
  }

  return boost::none;
}

void BatchJobManager::notifyAllWorkspacesDeleted() {
  // All output workspaces will be deleted so reset all rows and groups
  m_batch.resetState();
}

bool BatchJobManager::getProcessPartial() const { return m_processPartial; }
bool BatchJobManager::getProcessAll() const { return m_processAll && !m_isAutoreducing; }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
