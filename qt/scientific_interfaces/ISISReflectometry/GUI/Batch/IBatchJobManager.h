// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Batch/RowProcessingAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/IAlgorithmRuntimeProps.h"
#include "Reduction/Batch.h"

#include <boost/optional.hpp>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class PreviewRow;

class MANTIDQT_ISISREFLECTOMETRY_DLL IBatchJobManager {
public:
  virtual ~IBatchJobManager() = default;

  virtual bool isProcessing() const = 0;
  virtual bool isAutoreducing() const = 0;
  virtual int percentComplete() const = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyReductionPaused() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void setReprocessFailedItems(bool reprocessFailed) = 0;
  virtual boost::optional<Item &> getRunsTableItem(API::IConfiguredAlgorithm_sptr const &algorithm) = 0;
  virtual void algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) = 0;
  virtual void algorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) = 0;
  virtual void algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) = 0;
  virtual std::vector<std::string>
  algorithmOutputWorkspacesToSave(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) const = 0;
  virtual boost::optional<Item const &> notifyWorkspaceDeleted(std::string const &wsName) = 0;
  virtual boost::optional<Item const &> notifyWorkspaceRenamed(std::string const &oldName,
                                                               std::string const &newName) = 0;
  virtual void notifyAllWorkspacesDeleted() = 0;
  virtual std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> getAlgorithms() = 0;
  virtual std::unique_ptr<MantidQt::API::IAlgorithmRuntimeProps> rowProcessingProperties() const = 0;
  virtual bool getProcessPartial() const = 0;
  virtual bool getProcessAll() const = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
