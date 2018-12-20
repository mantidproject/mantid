// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTER_H

#include "DllConfig.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Save/ISavePresenter.h"
#include "IReflBatchPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IReflBatchView;

/** @class ReflBatchPresenter

ReflBatchPresenter is the concrete main window presenter implementing the
functionality defined by the interface IReflBatchPresenter.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflBatchPresenter
    : public IReflBatchPresenter {
public:
  /// Constructor
  ReflBatchPresenter(IReflBatchView *view,
                     std::unique_ptr<IRunsPresenter> runsPresenter,
                     std::unique_ptr<IEventPresenter> eventPresenter,
                     std::unique_ptr<IExperimentPresenter> experimentPresenter,
                     std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
                     std::unique_ptr<ISavePresenter> savePresenter);

  // IReflBatchPresenter overrides
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
  bool hasPerAngleOptions() const override;
  MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle) const override;
  void notifyInstrumentChanged(const std::string &instName) const override;
  bool isProcessing() const override;
  bool requestClose() const override;
  void settingsChanged() override;

private:
  void setInstrumentName(const std::string &instName) const;
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
  /// The view we are handling
  IReflBatchView *m_view;
  /// The presenter of tab 'Runs'
  std::unique_ptr<IRunsPresenter> m_runsPresenter;
  /// The presenter of tab 'Event Handling'
  std::unique_ptr<IEventPresenter> m_eventPresenter;
  /// The presenter of tab 'Settings'
  std::unique_ptr<IExperimentPresenter> m_experimentPresenter;
  std::unique_ptr<IInstrumentPresenter> m_instrumentPresenter;
  /// The presenter of tab 'Save ASCII'
  std::unique_ptr<ISavePresenter> m_savePresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTER_H */
