// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_IBATCHJOBRUNNER_H_
#define MANTID_CUSTOMINTERFACES_IBATCHJOBRUNNER_H_

#include "Common/DllConfig.h"
#include "GUI/Batch/RowProcessingAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Batch.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL IBatchJobRunner {
public:
  virtual ~IBatchJobRunner() = default;

  virtual bool isProcessing() const = 0;
  virtual bool isAutoreducing() const = 0;
  virtual int percentComplete() const = 0;
  virtual void reductionResumed() = 0;
  virtual void reductionPaused() = 0;
  virtual void autoreductionResumed() = 0;
  virtual void autoreductionPaused() = 0;
  virtual void setReprocessFailedItems(bool reprocessFailed) = 0;
  virtual Item const &
  algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) = 0;
  virtual Item const &
  algorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr algorithm) = 0;
  virtual Item const &
  algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm,
                 std::string const &message) = 0;
  virtual std::vector<std::string> algorithmOutputWorkspacesToSave(
      MantidQt::API::IConfiguredAlgorithm_sptr algorithm) const = 0;
  virtual boost::optional<Item const &>
  notifyWorkspaceDeleted(std::string const &wsName) = 0;
  virtual boost::optional<Item const &>
  notifyWorkspaceRenamed(std::string const &oldName,
                         std::string const &newName) = 0;
  virtual void notifyAllWorkspacesDeleted() = 0;
  virtual std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>
  getAlgorithms() = 0;
  virtual AlgorithmRuntimeProps rowProcessingProperties() const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_IBATCHJOBRUNNER_H_
