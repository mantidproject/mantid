// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IBatchJobRunner.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Batch.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/**
 * The BatchJobRunner class sets up algorithms to run based on the reduction
 * configuration, and handles updating state when algorithms complete
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL BatchJobRunner : public IBatchJobRunner {
public:
  explicit BatchJobRunner(Batch batch);

  bool isProcessing() const override;
  bool isAutoreducing() const override;
  int percentComplete() const override;

  void notifyReductionResumed() override;
  void notifyReductionPaused() override;
  void notifyAutoreductionResumed() override;
  void notifyAutoreductionPaused() override;

  void setReprocessFailedItems(bool reprocessFailed) override;

  Item const &algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) override;
  Item const &algorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) override;
  Item const &algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) override;

  std::vector<std::string>
  algorithmOutputWorkspacesToSave(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) const override;

  boost::optional<Item const &> notifyWorkspaceDeleted(std::string const &wsName) override;
  boost::optional<Item const &> notifyWorkspaceRenamed(std::string const &oldName, std::string const &newName) override;
  void notifyAllWorkspacesDeleted() override;

  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> getAlgorithms() override;
  AlgorithmRuntimeProps rowProcessingProperties() const override;

  bool getProcessPartial() const override;
  bool getProcessAll() const override;

protected:
  Batch m_batch;
  bool m_isProcessing;
  bool m_isAutoreducing;
  bool m_reprocessFailed;
  bool m_processAll;
  bool m_processPartial;
  std::vector<MantidWidgets::Batch::RowLocation> m_rowLocationsToProcess;

private:
  int itemsInSelection(Item::ItemCountFunction countFunction) const;

  std::vector<std::string> getWorkspacesToSave(Group const &group) const;
  std::vector<std::string> getWorkspacesToSave(Row const &row) const;
  size_t getNumberOfInitialisedRowsInGroup(const int groupIndex) const;
  template <typename T> bool isSelected(T const &item);
  bool hasSelectedRowsRequiringProcessing(Group const &group);
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithmForPostprocessingGroup(Group &group);
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithmsForProcessingRowsInGroup(Group &group,
                                                                                          bool processAll);
  void addAlgorithmForProcessingRow(Row &row, std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> &algorithms);
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
