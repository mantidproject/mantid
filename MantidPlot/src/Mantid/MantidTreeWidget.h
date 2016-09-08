#ifndef MANTIDTREEWIDGET_H
#define MANTIDTREEWIDGET_H

#include "Mantid/MantidSurfacePlotDialog.h"
#include "Mantid/MantidWSIndexDialog.h"

#include <QTreeWidget>
#include <boost/shared_ptr.hpp>

class MantidDockWidget;
class MantidUI;

enum class MantidItemSortScheme { ByName, ByLastModified };

class MantidTreeWidget : public QTreeWidget {
  Q_OBJECT

public:
  MantidTreeWidget(MantidDockWidget *w, MantidUI *mui);
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;

  QStringList getSelectedWorkspaceNames() const;
  MantidWSIndexWidget::UserInput
  chooseSpectrumFromSelected(bool showWaterfallOpt = true,
                             bool showPlotAll = true) const;
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
  MantidSurfacePlotDialog::UserInputSurface
  chooseSurfacePlotOptions(int nWorkspaces) const;
  MantidSurfacePlotDialog::UserInputSurface
  chooseContourPlotOptions(int nWorkspaces) const;

protected:
  void dragMoveEvent(QDragMoveEvent *de) override;
  void dragEnterEvent(QDragEnterEvent *de) override;
  MantidSurfacePlotDialog::UserInputSurface
  choosePlotOptions(const QString &type, int nWorkspaces) const;

private:
  QPoint m_dragStartPosition;
  MantidDockWidget *m_dockWidget;
  MantidUI *m_mantidUI;
  Mantid::API::AnalysisDataServiceImpl &m_ads;
  MantidItemSortScheme m_sortScheme;
  Qt::SortOrder m_sortOrder;
};

#endif // MANTIDTREEWIDGET_H