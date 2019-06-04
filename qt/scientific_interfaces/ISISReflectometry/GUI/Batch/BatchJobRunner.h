// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_

#include "Common/DllConfig.h"
#include "IBatchJobRunner.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Batch.h"

namespace MantidQt {
namespace CustomInterfaces {

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

  void reductionResumed() override;
  void reductionPaused() override;
  void autoreductionResumed() override;
  void autoreductionPaused() override;

  void setReprocessFailedItems(bool reprocessFailed) override;

  void
  algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) override;
  void algorithmComplete(
      MantidQt::API::IConfiguredAlgorithm_sptr algorithm) override;
  void algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm,
                      std::string const &message) override;

  std::vector<std::string> algorithmOutputWorkspacesToSave(
      MantidQt::API::IConfiguredAlgorithm_sptr algorithm) const override;

  void notifyWorkspaceDeleted(std::string const &wsName) override;
  void notifyWorkspaceRenamed(std::string const &oldName,
                              std::string const &newName) override;
  void notifyAllWorkspacesDeleted() override;

  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> getAlgorithms() override;

protected:
  Batch m_batch;
  bool m_isProcessing;
  bool m_isAutoreducing;
  bool m_reprocessFailed;
  bool m_processAll;
  std::vector<MantidWidgets::Batch::RowLocation> m_rowLocationsToProcess;

private:
  int itemsInSelection(Item::ItemCountFunction countFunction) const;

  std::vector<std::string> getWorkspacesToSave(Group const &group) const;
  std::vector<std::string> getWorkspacesToSave(Row const &row) const;

  template <typename T> bool isSelected(T const &item);
  bool hasSelectedRowsRequiringProcessing(Group const &group);
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>
  algorithmForPostprocessingGroup(Group &group);
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>
  algorithmsForProcessingRowsInGroup(Group &group, bool processAll);
  void addAlgorithmForProcessingRow(
      Row &row,
      std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> &algorithms);
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_
