#ifndef MANTID_CUSTOMINTERFACES_QTREFLMAINWINDOWVIEW_H
#define MANTID_CUSTOMINTERFACES_QTREFLMAINWINDOWVIEW_H

#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowView.h"
#include "ui_ReflMainWindowWidget.h"

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;
class IReflRunsTabPresenter;
class IReflSettingsTabPresenter;

/** @class ReflMainWindowView

ReflMainWindowView is the concrete main window view implementing the
functionality defined by the interface IReflMainWindowView

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
class QtReflMainWindowView : public MantidQt::API::UserSubWindow,
                             public IReflMainWindowView {
public:
  /// Constructor
  QtReflMainWindowView(QWidget *parent = 0);
  /// Destructor
  ~QtReflMainWindowView() override;
  /// Name of the interface
  static std::string name() { return "ISIS Reflectometry (Polref)"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Reflectometry"; }

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
  /// Initializes the interface
  void initLayout() override;
  /// Creates the 'Runs' tab
  IReflRunsTabPresenter *createRunsTab();
  /// Creates the 'Settings' tab
  IReflSettingsTabPresenter *createSettingsTab();

  /// Interface definition with widgets for the main interface window
  Ui::RelMainWindowWidget m_ui;
  /// The presenter handling this view
  std::unique_ptr<IReflMainWindowPresenter> m_presenter;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_QTREFLMAINWINDOWVIEW_H */
