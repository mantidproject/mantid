// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IBatchJobManager.h"
#include "IReflAlgorithmFactory.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/IAlgorithmRuntimeProps.h"
#include "Reduction/Batch.h"

#include <boost/optional.hpp>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class PreviewRow;

/**
 * The BatchJobManager class sets up algorithms to run based on the reduction
 * configuration, and handles updating state when algorithms complete
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL BatchJobManager : public IBatchJobManager {
public:
  BatchJobManager(IBatch &batch, std::unique_ptr<IReflAlgorithmFactory> algFactory = nullptr);

  bool isProcessing() const override;
  bool isAutoreducing() const override;
  int percentComplete() const override;

  void notifyReductionResumed() override;
  void notifyReductionPaused() override;
  void notifyAutoreductionResumed() override;
  void notifyAutoreductionPaused() override;

  void setReprocessFailedItems(bool reprocessFailed) override;

  boost::optional<Item &> getRunsTableItem(API::IConfiguredAlgorithm_sptr const &algorithm) override;

  void algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) override;
  void algorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) override;
  void algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) override;

  std::vector<std::string>
  algorithmOutputWorkspacesToSave(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) const override;

  boost::optional<Item const &> notifyWorkspaceDeleted(std::string const &wsName) override;
  boost::optional<Item const &> notifyWorkspaceRenamed(std::string const &oldName, std::string const &newName) override;
  void notifyAllWorkspacesDeleted() override;

  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> getAlgorithms() override;
  std::unique_ptr<MantidQt::API::IAlgorithmRuntimeProps> rowProcessingProperties() const override;

  bool getProcessPartial() const override;
  bool getProcessAll() const override;

protected:
  IBatch &m_batch;
  std::unique_ptr<IReflAlgorithmFactory> m_algFactory;
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
