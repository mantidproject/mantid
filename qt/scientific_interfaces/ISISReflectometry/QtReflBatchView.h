#ifndef MANTID_ISISREFLECTOMETRY_QTREFLBATCHVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTREFLBATCHVIEW_H

#include "IReflBatchView.h"
#include "IReflBatchPresenter.h"
#include "ui_ReflBatchWidget.h"
#include <memory>

#include <QCloseEvent>

namespace MantidQt {
namespace CustomInterfaces {

class IReflEventTabPresenter;
class IReflRunsTabPresenter;
class IReflSettingsTabPresenter;
class IReflSaveTabPresenter;

/**
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
class QtReflBatchView : public QWidget, public IReflBatchView {
  Q_OBJECT
public:
  /// Constructor
  explicit QtReflBatchView(QWidget *parent = nullptr);

//  QtReflRunsTabView& runsTab() const;
//  QtReflEventTabView& eventTab() const;
//  QtReflSaveTabView& saveTab() const;
//  QtReflSettingsTabView& settingsTab() const;
private:
  /// Initializes the interface
  void initLayout();
  /// Creates the 'Runs' tab
  std::unique_ptr<IReflRunsTabPresenter> createRunsTab();
  /// Creates the 'Event Handling' tab
  IReflEventTabPresenter *createEventTab();
  /// Creates the 'Settings' tab
  IReflSettingsTabPresenter *createSettingsTab();
  /// Creates the 'Save ASCII' tab
  std::unique_ptr<IReflSaveTabPresenter> createSaveTab();

  /// Interface definition with widgets for the main interface window
  Ui::ReflBatchWidget m_ui;
  /// The presenter handling this view
  std::unique_ptr<IReflBatchPresenter> m_presenter;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_QTREFLBATCHVIEW_H */
