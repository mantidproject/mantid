#ifndef MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H

#include "IReflMainWindowView.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ui_ReflMainWindowWidget.h"

#include <QCloseEvent>

namespace MantidQt {
namespace CustomInterfaces {

class IReflEventTabPresenter;
class IReflMainWindowPresenter;
class IReflRunsTabPresenter;
class IReflSettingsTabPresenter;
class IReflSaveTabPresenter;

/** @class ReflMainWindowView

ReflMainWindowView is the concrete main window view implementing the
functionality defined by the interface IReflMainWindowView

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  Q_OBJECT
public:
  /// Constructor
  explicit QtReflMainWindowView(QWidget *parent = nullptr);
  /// Destructor
  ~QtReflMainWindowView() override;
  /// Name of the interface
  static std::string name() { return "ISIS Reflectometry"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Reflectometry"; }

  /// Dialog to show an error message
  void giveUserCritical(const std::string &prompt,
                        const std::string &title) override;
  /// Dialog to show information
  void giveUserInfo(const std::string &prompt,
                    const std::string &title) override;
  /// Run a python algorithm
  std::string runPythonAlgorithm(const std::string &pythonCode) override;

  /// Close window handler
  void closeEvent(QCloseEvent *event) override;

public slots:
  void helpPressed();

private:
  /// Initializes the interface
  void initLayout() override;
  /// Creates the 'Runs' tab
  IReflRunsTabPresenter *createRunsTab();
  /// Creates the 'Event Handling' tab
  IReflEventTabPresenter *createEventTab();
  /// Creates the 'Settings' tab
  IReflSettingsTabPresenter *createSettingsTab();
  /// Creates the 'Save ASCII' tab
  std::unique_ptr<IReflSaveTabPresenter> createSaveTab();

  /// Interface definition with widgets for the main interface window
  Ui::RelMainWindowWidget m_ui;
  /// The presenter handling this view
  std::unique_ptr<IReflMainWindowPresenter> m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H */
