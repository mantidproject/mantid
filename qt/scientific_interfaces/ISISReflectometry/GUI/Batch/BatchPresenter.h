// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_BATCHPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_BATCHPRESENTER_H

#include "Common/DllConfig.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Save/ISavePresenter.h"
#include "IBatchJobRunner.h"
#include "IBatchPresenter.h"
#include "IBatchView.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IBatchView;

/** @class BatchPresenter

    BatchPresenter is the concrete main window presenter implementing the
    functionality defined by the interface IBatchPresenter.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL BatchPresenter
    : public IBatchPresenter,
      public BatchViewSubscriber,
      public MantidQt::API::WorkspaceObserver {
public:
  /// Constructor
  BatchPresenter(IBatchView *view, Batch model,
                 std::unique_ptr<IRunsPresenter> runsPresenter,
                 std::unique_ptr<IEventPresenter> eventPresenter,
                 std::unique_ptr<IExperimentPresenter> experimentPresenter,
                 std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
                 std::unique_ptr<ISavePresenter> savePresenter);

  // BatchViewSubscriber overrides
  void notifyBatchComplete(bool error) override;
  void notifyBatchCancelled() override;
  void notifyAlgorithmStarted(
      MantidQt::API::IConfiguredAlgorithm_sptr algorithm) override;
  void notifyAlgorithmComplete(
      MantidQt::API::IConfiguredAlgorithm_sptr algorithm) override;
  void notifyAlgorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm,
                            std::string const &message) override;

  // IBatchPresenter overrides
  void acceptMainPresenter(IMainWindowPresenter *mainPresenter) override;
  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  void notifyAutoreductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyAutoreductionCompleted() override;
  void notifyInstrumentChanged(const std::string &instName) override;
  void notifyRestoreDefaultsRequested() override;
  void notifySettingsChanged() override;
  void anyBatchAutoreductionResumed() override;
  void anyBatchAutoreductionPaused() override;
  bool requestClose() const override;
  bool isProcessing() const override;
  bool isAutoreducing() const override;
  bool isAnyBatchAutoreducing() const override;
  Mantid::Geometry::Instrument_const_sptr instrument() const override;
  std::string instrumentName() const override;
  int percentComplete() const override;
  AlgorithmRuntimeProps rowProcessingProperties() const override;

  // WorkspaceObserver overrides
  void postDeleteHandle(const std::string &wsName) override;
  void renameHandle(const std::string &oldName,
                    const std::string &newName) override;
  void clearADSHandle() override;

private:
  bool
  startBatch(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithms);
  void resumeReduction();
  void reductionResumed();
  void pauseReduction();
  void reductionPaused();
  void resumeAutoreduction();
  void autoreductionResumed();
  void pauseAutoreduction();
  void autoreductionPaused();
  void autoreductionCompleted();
  void instrumentChanged(const std::string &instName);
  void updateInstrument(const std::string &instName);
  void settingsChanged();

  IBatchView *m_view;
  IMainWindowPresenter *m_mainPresenter;
  std::unique_ptr<IRunsPresenter> m_runsPresenter;
  std::unique_ptr<IEventPresenter> m_eventPresenter;
  std::unique_ptr<IExperimentPresenter> m_experimentPresenter;
  std::unique_ptr<IInstrumentPresenter> m_instrumentPresenter;
  std::unique_ptr<ISavePresenter> m_savePresenter;
  Mantid::Geometry::Instrument_const_sptr m_instrument;

protected:
  std::unique_ptr<IBatchJobRunner> m_jobRunner;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_BATCHPRESENTER_H */
