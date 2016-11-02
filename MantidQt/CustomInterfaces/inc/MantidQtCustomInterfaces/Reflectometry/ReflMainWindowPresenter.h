#ifndef MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTER_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowView;
class IReflRunsTabPresenter;
class IReflSettingsTabPresenter;

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
                          IReflSettingsTabPresenter *settingsPresenter);
  /// Destructor
  ~ReflMainWindowPresenter() override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  std::string getTransmissionOptions() const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  std::string getReductionOptions() const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions() const override;
  /// Returns instrument name
  std::string getInstrumentName() const override;

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

private:
  /// Check for null pointer
  void checkPtrValid(IReflSettingsTabPresenter *pointer) const;
  /// The view we are handling
  IReflMainWindowView *m_view;
  /// The presenter of tab 'Runs'
  IReflRunsTabPresenter *m_runsPresenter;
  /// The presenter of tab 'Settings'
  IReflSettingsTabPresenter *m_settingsPresenter;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_REFLMAINWINDOWPRESENTER_H */
