#ifndef MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGETSIMPLE_H
#define MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGETSIMPLE_H

#include "MantidQtWidgets/Common/DllOption.h"
#include <MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h>

#include <MantidAPI/MatrixWorkspace_fwd.h>

#include <QWidget>
#include <QMenu>

class QTreeWidgetItem;
class QSignalMapper;

namespace MantidQt {
namespace MantidWidgets {
class MantidDisplayBase;
class MantidTreeWidget;

/**
\class  WorkspaceTreeWidgetSimple
\brief  WorkspaceTreeWidget implementation for the Workbench - required for some
function overides
\author Elliot Oram
\date   16-01-2018
\version 1.0


Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class EXPORT_OPT_MANTIDQT_COMMON WorkspaceTreeWidgetSimple
    : public WorkspaceTreeWidget {
  Q_OBJECT
public:
  explicit WorkspaceTreeWidgetSimple(QWidget *parent = nullptr);
  ~WorkspaceTreeWidgetSimple();

  // Context Menu Handlers
  void popupContextMenu() override;

signals:
  void plotSpectrumClicked(const QStringList &workspaceName);
  void plotSpectrumWithErrorsClicked(const QStringList &workspaceName);
  void plotColorfillClicked(const QStringList &workspaceName);

private slots:
  void onPlotSpectrumClicked();
  void onPlotSpectrumWithErrorsClicked();
  void onPlotColorfillClicked();

private:
  QAction *m_plotSpectrum, *m_plotSpectrumWithErrs, *m_plotColorfill;
};
}
}
#endif // MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGETSIMPLE_H
