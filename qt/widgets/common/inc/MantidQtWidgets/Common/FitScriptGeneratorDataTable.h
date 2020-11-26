// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QEvent>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QString>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace MantidQt {
namespace MantidWidgets {

/**
 * This class represents the table widget which holds domain data for the
 * FitScriptGenerator interface. This table has four columns:
 * Workspace Name, Workspace Index, Start X, End X.
 *
 * This table has been manually created and derived from QTableWidget to allow
 * the table rows to be highlighted when a hover event occurs.
 */
class FitScriptGeneratorDataTable : public QTableWidget {
  Q_OBJECT

  enum ColumnIndex {
    WorkspaceName = 0,
    WorkspaceIndex = 1,
    StartX = 2,
    EndX = 3
  } const;

public:
  FitScriptGeneratorDataTable(QWidget *parent = nullptr);
  ~FitScriptGeneratorDataTable() = default;

  void addWorkspaceDomain(QString const &workspaceName, int workspaceIndex,
                          double startX, double endX);

signals:
  void itemExited(int newRowIndex);

private:
  bool eventFilter(QObject *widget, QEvent *event) override;
  QPersistentModelIndex hoveredRowIndex(QEvent *event);

  QPersistentModelIndex m_lastIndex;
};

/**
 * This class is used for formating the type of data allowed in each of the
 * tables columns. It is also used for setting various column properties, and
 * will paint a row when it is hovered over.
 */
class CustomItemDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  enum class DelegateType { Double, Int, String };

  CustomItemDelegate(FitScriptGeneratorDataTable *parent = nullptr,
                     DelegateType const &type = DelegateType::Double);

private slots:
  void handleItemEntered(QTableWidgetItem *item);
  void handleItemExited(int newRowIndex);

private:
  QWidget *createEditor(QWidget *parent, QStyleOptionViewItem const &option,
                        QModelIndex const &index) const;
  void paint(QPainter *painter, QStyleOptionViewItem const &option,
             QModelIndex const &index) const override;

  FitScriptGeneratorDataTable *m_tableWidget;
  int m_hoveredIndex;
  DelegateType m_type;
};

} // namespace MantidWidgets
} // namespace MantidQt
