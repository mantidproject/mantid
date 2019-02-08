// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_

#include "Common/DllConfig.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "Reduction/Batch.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL BatchJobRunner {
public:
  BatchJobRunner(Batch batch, API::BatchAlgorithmRunner &batchAlgoRunner);

  bool isProcessing() const;
  bool isAutoreducing() const;

  void resumeReduction();
  void reductionPaused();
  void resumeAutoreduction();
  void autoreductionPaused();

  void setReprocessFailedItems(bool reprocessFailed);

  void algorithmFinished(Mantid::API::IAlgorithm_sptr algorithm);
  void algorithmError(std::string const &message,
                      Mantid::API::IAlgorithm_sptr algorithm);

  std::vector<std::string> algorithmOutputWorkspacesToSave(
      Mantid::API::IAlgorithm_sptr algorithm,
      const API::BatchAlgorithmRunnerSubscriber *const item) const;

  void notifyWorkspaceDeleted(std::string const &wsName);
  void notifyWorkspaceRenamed(std::string const &oldName,
                              std::string const &newName);
  void notifyAllWorkspacesDeleted();

private:
  Batch m_batch;
  bool m_isProcessing;
  bool m_isAutoreducing;
  bool m_reprocessFailed;
  bool m_processAll;
  API::BatchAlgorithmRunner &m_batchAlgoRunner;

  void setUpBatchAlgorithmRunner();
  std::vector<std::string> getWorkspacesToSave(Group const &group) const;
  std::vector<std::string> getWorkspacesToSave(Row const &row) const;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_
