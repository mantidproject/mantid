#ifndef MANTIDQT_MANTIDWIDGETS_MANTIDTREEWIDGET_H
#define MANTIDQT_MANTIDWIDGETS_MANTIDTREEWIDGET_H

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <MantidAPI/AnalysisDataService.h>
#include <MantidAPI/MatrixWorkspace_fwd.h>
#include <MantidQtMantidWidgets/MantidWSIndexDialog.h>

#include <QTreeWidget>
#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {
class MantidDisplayBase;
class QWorkspaceDockView;

enum class MantidItemSortScheme { ByName, ByLastModified };

class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MantidTreeWidget : public QTreeWidget {
  Q_OBJECT

public:
  MantidTreeWidget(QWorkspaceDockView *w, MantidDisplayBase *mui);
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;

  QStringList getSelectedWorkspaceNames() const;
  MantidWSIndexWidget::UserInput
  chooseSpectrumFromSelected(bool showWaterfallOpt = true,
                             bool showPlotAll = true, bool showTiledOpt = true,
                             bool isAdvanced = false) const;
  void setSortScheme(MantidItemSortScheme);
  void setSortOrder(Qt::SortOrder);
  MantidItemSortScheme getSortScheme() const;
  Qt::SortOrder getSortOrder() const;
  void logWarningMessage(const std::string &);
  void disableNodes(bool);
  void sort();
  void dropEvent(QDropEvent *de) override;
  QList<boost::shared_ptr<const Mantid::API::MatrixWorkspace>>
  getSelectedMatrixWorkspaces() const;

protected:
  void dragMoveEvent(QDragMoveEvent *de) override;
  void dragEnterEvent(QDragEnterEvent *de) override;

private:
  QPoint m_dragStartPosition;
  QWorkspaceDockView *m_dockWidget;
  MantidDisplayBase *m_mantidUI;
  Mantid::API::AnalysisDataServiceImpl &m_ads;
  MantidItemSortScheme m_sortScheme;
  Qt::SortOrder m_sortOrder;
};
}
}
#endif // MANTIDQT_MANTIDWIDGETS_MANTIDTREEWIDGET_H