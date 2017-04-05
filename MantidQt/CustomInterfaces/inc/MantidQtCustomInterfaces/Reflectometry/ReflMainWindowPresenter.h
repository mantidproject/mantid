#ifndef MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTER_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"

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

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL ReflMainWindowPresenter
    : public IReflMainWindowPresenter {
public:
  /// Constructor
  ReflMainWindowPresenter(IReflMainWindowView *view,
                          IReflRunsTabPresenter *runsPresenter,
                          IReflEventTabPresenter *eventPresenter,
                          IReflSettingsTabPresenter *settingsPresenter,
                          IReflSaveTabPresenter *savePresenter);
  /// Destructor
  ~ReflMainWindowPresenter() override;
  /// Returns values passed for 'Transmission run(s)'
  std::string getTransmissionRuns(int group) const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  std::string getTransmissionOptions(int group) const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  std::string getReductionOptions(int group) const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions(int group) const override;
  /// Returns time-slicing values
  std::string getTimeSlicingValues(int group) const override;
  /// Returns time-slicing type
  std::string getTimeSlicingType(int group) const override;

  /// Dialog/Prompt methods
  std::string askUserString(const std::string &prompt, const std::string &title,
                            const std::string &defaultValue) override;
  bool askUserYesNo(const std::string &prompt,
                    const std::string &title) override;
  void giveUserWarning(const std::string &prompt,
                       const std::string &title) override;
  void giveUserCritical(const std::string &prompt,
                        const std::string &title) override;
  void giveUserInfo(const std::string &prompt,
                    const std::string &title) override;
  std::string runPythonAlgorithm(const std::string &pythonCode) override;
  void setInstrumentName(const std::string &instName) const override;

private:
  /// Check for Settings Tab null pointer
  void checkSettingsPtrValid(IReflSettingsTabPresenter *pointer) const;
  /// Check for Event Handling Tab null pointer
  void checkEventPtrValid(IReflEventTabPresenter *pointer) const;
  /// The view we are handling
  IReflMainWindowView *m_view;
  /// The presenter of tab 'Runs'
  IReflRunsTabPresenter *m_runsPresenter;
  /// The presenter of tab 'Event Handling'
  IReflEventTabPresenter *m_eventPresenter;
  /// The presenter of tab 'Settings'
  IReflSettingsTabPresenter *m_settingsPresenter;
  /// The presenter of tab 'Save ASCII'
  IReflSaveTabPresenter *m_savePresenter;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTER_H */
