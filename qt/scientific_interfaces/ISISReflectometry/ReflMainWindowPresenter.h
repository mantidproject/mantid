// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H

#include "DllConfig.h"
#include "IReflMainWindowPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowView;
class IReflRunsTabPresenter;
class IReflEventTabPresenter;
class IReflSettingsTabPresenter;
class IReflSaveTabPresenter;

/** @class ReflMainWindowPresenter

ReflMainWindowPresenter is the concrete main window presenter implementing the
functionality defined by the interface IReflMainWindowPresenter.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflMainWindowPresenter
    : public IReflMainWindowPresenter {
public:
  /// Constructor
  ReflMainWindowPresenter(IReflMainWindowView *view,
                          IReflRunsTabPresenter *runsPresenter,
                          IReflEventTabPresenter *eventPresenter,
                          IReflSettingsTabPresenter *settingsPresenter,
                          std::unique_ptr<IReflSaveTabPresenter> savePresenter);
  /// Destructor
  ~ReflMainWindowPresenter() override;

  /// Returns values passed for 'Transmission run(s)'
  MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(int group, const double angle) const override;
  /// Whether there are per-angle transmission runs specified
  bool hasPerAngleOptions(int group) const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions(int group) const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions(int group) const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions(int group) const override;
  /// Returns time-slicing values
  std::string getTimeSlicingValues(int group) const override;
  /// Returns time-slicing type
  std::string getTimeSlicingType(int group) const override;

  /// Dialog to show error message
  void giveUserCritical(const std::string &prompt,
                        const std::string &title) override;
  /// Dialog to show information
  void giveUserInfo(const std::string &prompt,
                    const std::string &title) override;
  /// Run a python algorithm
  std::string runPythonAlgorithm(const std::string &pythonCode) override;
  /// Set the instrument name
  void setInstrumentName(const std::string &instName) const override;

  /// Returns whether the Runs Tab is currently processing any runs
  bool isProcessing() const override;
  bool isProcessing(int group) const override;
  void settingsChanged(int group) override;
  void notify(IReflMainWindowPresenter::Flag flag) override;
  void notifyReductionPaused(int group) override;
  void notifyReductionResumed(int group) override;

  void completedGroupReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;
  void completedRowReductionSuccessfully(
      MantidWidgets::DataProcessor::GroupData const &group,
      std::string const &workspaceName) override;

private:
  /// Check for Settings Tab null pointer
  void checkSettingsPtrValid(IReflSettingsTabPresenter *pointer) const;
  /// Check for Event Handling Tab null pointer
  void checkEventPtrValid(IReflEventTabPresenter *pointer) const;
  /// Pauses reduction in the Runs Tab
  void pauseReduction() const;
  /// Resumes reduction in the Runs Tab
  void resumeReduction() const;
  void showHelp();
  /// The view we are handling
  IReflMainWindowView *m_view;
  /// The presenter of tab 'Runs'
  IReflRunsTabPresenter *m_runsPresenter;
  /// The presenter of tab 'Event Handling'
  IReflEventTabPresenter *m_eventPresenter;
  /// The presenter of tab 'Settings'
  IReflSettingsTabPresenter *m_settingsPresenter;
  /// The presenter of tab 'Save ASCII'
  std::unique_ptr<IReflSaveTabPresenter> m_savePresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLMAINWINDOWPRESENTER_H */
