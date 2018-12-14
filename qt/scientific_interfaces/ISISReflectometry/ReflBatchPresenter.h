// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTER_H

#include "DllConfig.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "IReflBatchPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IReflBatchView;
class IReflRunsTabPresenter;
class IEventPresenter;
class ISavePresenter;

/** @class ReflBatchPresenter

ReflBatchPresenter is the concrete main window presenter implementing the
functionality defined by the interface IReflBatchPresenter.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflBatchPresenter
    : public IReflBatchPresenter {
public:
  /// Constructor
  ReflBatchPresenter(IReflBatchView *view,
                     std::unique_ptr<IReflRunsTabPresenter> runsPresenter,
                     std::unique_ptr<IEventPresenter> eventPresenter,
                     std::unique_ptr<IExperimentPresenter> experimentPresenter,
                     std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
                     std::unique_ptr<ISavePresenter> savePresenter);

  /// Returns values passed for 'Transmission run(s)'
  MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle) const override;
  /// Whether there are per-angle transmission runs specified
  bool hasPerAngleOptions() const override;
  /// Set the instrument name
  void setInstrumentName(const std::string &instName) const override;

  /// Returns whether the Runs Tab is currently processing any runs
  bool isProcessing() const override;
  void settingsChanged() override;
  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  bool requestClose() const override;

  void completedGroupReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;
  void completedRowReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;

private:
  /// Pauses reduction in the Runs Tab
  void pauseReduction() const;
  /// Resumes reduction in the Runs Tab
  void resumeReduction() const;
  /// The view we are handling
  IReflBatchView *m_view;
  /// The presenter of tab 'Runs'
  std::unique_ptr<IReflRunsTabPresenter> m_runsPresenter;
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
