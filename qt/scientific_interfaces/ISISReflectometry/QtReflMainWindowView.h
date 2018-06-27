#ifndef MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H

#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "IReflMainWindowView.h"
#include "ui_ReflMainWindowWidget.h"
#include "IReflMainWindowPresenter.h"
#include "ReflMainWindowPresenter.h"

#include <QCloseEvent>

namespace MantidQt {
namespace CustomInterfaces {

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
  explicit QtReflMainWindowView(QWidget *parent = nullptr);
  void subscribe(ReflMainWindowSubscriber* notifyee) override;

  static std::string name() { return "ISIS Reflectometry"; }
  static QString categoryInfo() { return "Reflectometry"; }
  std::string runPythonAlgorithm(const std::string &pythonCode) override;

  virtual std::vector<IReflBatchView*> batches() const override;

  void closeEvent(QCloseEvent *event) override;

  IReflBatchView* newBatch() override;
  void removeBatch(int batchIndex) override;

public slots:
  void helpPressed();
  void onTabCloseRequested(int tabIndex);
  void onNewBatchRequested(bool);

private:
  /// Initializes the interface
  void initLayout() override;
  /// Interface definition with widgets for the main interface window
  Ui::ReflMainWindowWidget m_ui;
  /// The presenter handling this view
  ReflMainWindowSubscriber* m_notifyee;
  boost::optional<ReflMainWindowPresenter> m_presenter;
  std::vector<IReflBatchView*> m_batchViews;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_QTREFLMAINWINDOWVIEW_H */
