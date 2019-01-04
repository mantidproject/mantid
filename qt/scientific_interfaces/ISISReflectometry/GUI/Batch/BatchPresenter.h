// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_BATCHPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_BATCHPRESENTER_H

#include "BatchJobRunner.h"
#include "Common/DllConfig.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Save/ISavePresenter.h"
#include "IBatchPresenter.h"
#include "IBatchView.h"
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
      public BatchViewSubscriber {
public:
  /// Constructor
  BatchPresenter(IBatchView *view, Batch model,
                 std::unique_ptr<IRunsPresenter> runsPresenter,
                 std::unique_ptr<IEventPresenter> eventPresenter,
                 std::unique_ptr<IExperimentPresenter> experimentPresenter,
                 std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
                 std::unique_ptr<ISavePresenter> savePresenter);

  // BatchViewSubscriber overrides
  void notifyBatchFinished(bool error) override;
  void notifyBatchCancelled() override;
  void notifyAlgorithmFinished() override;
  void notifyAlgorithmError(std::string const &message) override;

  // IBatchPresenter overrides
  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  void notifyReductionCompletedForGroup(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;
  void notifyReductionCompletedForRow(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;
  void notifyAutoreductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyAutoreductionCompleted() override;
  void notifyInstrumentChanged(const std::string &instName) override;
  void notifySettingsChanged() override;
  bool hasPerAngleOptions() const override;
  MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle) const override;
  bool requestClose() const override;
  bool isProcessing() const override;
  bool isAutoreducing() const override;

private:
  void reductionResumed();
  void reductionPaused();
  void reductionCompletedForGroup(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName);
  void
  reductionCompletedForRow(MantidWidgets::DataProcessor::GroupData const &group,
                           std::string const &workspaceName);
  void autoreductionResumed();
  void autoreductionPaused();
  void autoreductionCompleted();
  void instrumentChanged(const std::string &instName);
  void settingsChanged();

  IBatchView *m_view;
  BatchJobRunner m_jobRunner;
  std::unique_ptr<IRunsPresenter> m_runsPresenter;
  std::unique_ptr<IEventPresenter> m_eventPresenter;
  std::unique_ptr<IExperimentPresenter> m_experimentPresenter;
  std::unique_ptr<IInstrumentPresenter> m_instrumentPresenter;
  std::unique_ptr<ISavePresenter> m_savePresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_BATCHPRESENTER_H */
