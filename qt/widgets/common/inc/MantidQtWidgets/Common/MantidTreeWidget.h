// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_MANTIDTREEWIDGET_H
#define MANTIDQT_MANTIDWIDGETS_MANTIDTREEWIDGET_H

#include "MantidQtWidgets/Common/DllOption.h"
#include <MantidAPI/AnalysisDataService.h>
#include <MantidAPI/MatrixWorkspace_fwd.h>
#include <MantidQtWidgets/Common/MantidWSIndexDialog.h>

#include <QTreeWidget>
#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {
class MantidDisplayBase;
class WorkspaceTreeWidget;

enum class MantidItemSortScheme { ByName, ByLastModified, ByMemorySize };

class EXPORT_OPT_MANTIDQT_COMMON MantidTreeWidget : public QTreeWidget {
  Q_OBJECT

public:
  MantidTreeWidget(MantidDisplayBase *mui, QWidget *parent = nullptr);
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
  MantidDisplayBase *m_mantidUI;
  Mantid::API::AnalysisDataServiceImpl &m_ads;
  MantidItemSortScheme m_sortScheme;
  Qt::SortOrder m_sortOrder;
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQT_MANTIDWIDGETS_MANTIDTREEWIDGET_H