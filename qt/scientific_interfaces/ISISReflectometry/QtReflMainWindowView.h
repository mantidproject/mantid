#ifndef MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H

#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "IReflMainWindowView.h"
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
  /// Name of the interface
  static std::string name() { return "ISIS Reflectometry"; }
  /// This interface's categories.
  static QString categoryInfo() { return "Reflectometry"; }
  /// Run a python algorithm
  std::string runPythonAlgorithm(const std::string &pythonCode) override;
  /// Close window handler
  void closeEvent(QCloseEvent *event) override;

public slots:
  void helpPressed();

private:
  /// Initializes the interface
  void initLayout() override;
  /// Interface definition with widgets for the main interface window
  Ui::ReflMainWindowWidget m_ui;
  /// The presenter handling this view
  std::unique_ptr<IReflMainWindowPresenter> m_presenter;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H */
