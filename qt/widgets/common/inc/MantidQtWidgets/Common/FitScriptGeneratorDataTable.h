// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <vector>

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
class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorDataTable : public QTableWidget {
  Q_OBJECT

public:
  enum ColumnIndex { WorkspaceName = 0, WorkspaceIndex = 1, StartX = 2, EndX = 3 };

  FitScriptGeneratorDataTable(QWidget *parent = nullptr);
  ~FitScriptGeneratorDataTable() = default;

  [[nodiscard]] std::string workspaceName(FitDomainIndex row) const;
  [[nodiscard]] MantidWidgets::WorkspaceIndex workspaceIndex(FitDomainIndex row) const;
  [[nodiscard]] double startX(FitDomainIndex row) const;
  [[nodiscard]] double endX(FitDomainIndex row) const;

  [[nodiscard]] std::vector<FitDomainIndex> allRows() const;
  [[nodiscard]] std::vector<FitDomainIndex> selectedRows() const;
  [[nodiscard]] FitDomainIndex currentRow() const;

  [[nodiscard]] bool hasLoadedData() const;

  [[nodiscard]] QString selectedDomainFunctionPrefix() const;

  void renameWorkspace(QString const &workspaceName, QString const &newName);

  void removeDomain(MantidWidgets::FitDomainIndex domainIndex);
  void addDomain(QString const &workspaceName, MantidWidgets::WorkspaceIndex workspaceIndex, double startX,
                 double endX);

  void formatSelection();
  void resetSelection();

  void setFunctionPrefixVisible(bool visible);

signals:
  void itemExited(int newRowIndex);

private slots:
  void handleItemClicked(QTableWidgetItem *item);
  void handleItemSelectionChanged();

private:
  bool eventFilter(QObject *widget, QEvent *event) override;
  QPersistentModelIndex hoveredRowIndex(QEvent *event);

  void updateVerticalHeaders();

  QString getText(FitDomainIndex row, int column) const;

  void setSelectedXValue(double xValue);

  std::vector<FitDomainIndex> m_selectedRows;
  int m_selectedColumn;
  double m_selectedValue;
  QPersistentModelIndex m_lastHoveredIndex;
};

using ColumnIndex = FitScriptGeneratorDataTable::ColumnIndex;

/**
 * This class is used for formating the type of data allowed in each of the
 * tables columns. It is also used for setting various column properties, and
 * will paint a row when it is hovered over.
 */
class CustomItemDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  CustomItemDelegate(FitScriptGeneratorDataTable *parent, ColumnIndex const &index);

private slots:
  void handleItemEntered(QTableWidgetItem *item);
  void handleItemExited(int newRowIndex);

private:
  QWidget *createEditor(QWidget *parent, QStyleOptionViewItem const &option, QModelIndex const &index) const override;
  void paint(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const override;

  FitScriptGeneratorDataTable *m_tableWidget;
  ColumnIndex m_columnIndex;
  int m_hoveredIndex;
};

} // namespace MantidWidgets
} // namespace MantidQt
