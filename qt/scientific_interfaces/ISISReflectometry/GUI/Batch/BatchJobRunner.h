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
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Batch.h"

namespace MantidQt {
namespace CustomInterfaces {

// Override the configured algorithm to pass to BatchAlgorithmRunner
// so that we can associate our own data with it
class BatchJobAlgorithm : public MantidQt::API::ConfiguredAlgorithm {
public:
  BatchJobAlgorithm(
      Mantid::API::IAlgorithm_sptr algorithm,
      MantidQt::API::ConfiguredAlgorithm::AlgorithmRuntimeProps properties,
      Item *item);

  Item *item();

private:
  // The data is an item in the table (i.e. a row or group)
  Item *m_item;
};

using BatchJobAlgorithm_sptr = boost::shared_ptr<BatchJobAlgorithm>;

/**
 * The BatchJobRunner class manages the running of algorithms based
 * on the reduction configuration and handling of state when algorithms
 * complete
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL BatchJobRunner {
public:
  explicit BatchJobRunner(Batch batch);

  bool isProcessing() const;
  bool isAutoreducing() const;

  void resumeReduction();
  void reductionPaused();
  void resumeAutoreduction();
  void autoreductionPaused();

  void setReprocessFailedItems(bool reprocessFailed);

  void algorithmStarted(MantidQt::API::ConfiguredAlgorithm_sptr algorithm);
  void algorithmComplete(MantidQt::API::ConfiguredAlgorithm_sptr algorithm);
  void algorithmError(MantidQt::API::ConfiguredAlgorithm_sptr algorithm,
                      std::string const &message);

  std::vector<std::string> algorithmOutputWorkspacesToSave(
      MantidQt::API::ConfiguredAlgorithm_sptr algorithm) const;

  void notifyWorkspaceDeleted(std::string const &wsName);
  void notifyWorkspaceRenamed(std::string const &oldName,
                              std::string const &newName);
  void notifyAllWorkspacesDeleted();

  std::deque<MantidQt::API::ConfiguredAlgorithm_sptr> getAlgorithms();

private:
  Batch m_batch;
  bool m_isProcessing;
  bool m_isAutoreducing;
  bool m_reprocessFailed;
  bool m_processAll;

  std::vector<std::string> getWorkspacesToSave(Group const &group) const;
  std::vector<std::string> getWorkspacesToSave(Row const &row) const;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_
