// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOM_DIALOGS_SORTTABLEWORKSPACEDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGS_SORTTABLEWORKSPACEDIALOG_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_SortTableWorkspaceDialog.h"

#include <QMap>

namespace MantidQt {

namespace CustomDialogs {

/**
  This class gives specialised dialog for the SortTableWorkspace algorithm.

  @date 1/12/2014
*/
class SortTableWorkspaceDialog : public API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Default constructor
  SortTableWorkspaceDialog(QWidget *parent = nullptr);

private:
  /// Initialize the layout
  void initLayout() override;
  /// Pass input from non-standard GUI elements to the algorithm
  void parseInput() override;
  /// Tie static widgets to their properties
  void tieStaticWidgets(const bool readHistory);
private slots:
  /// Update GUI after workspace changes
  void workspaceChanged(const QString &wsName);
  /// Add GUI elements to set a new column as a sorting key
  void addColumn();
  /// Sync the GUI after a sorting column name changes
  void changedColumnName(int /*unused*/);
  /// Remove a column to sort by.
  void removeColumn();
  /// Clear the GUI form the workspace specific data/elements
  void clearGUI();

private:
  /// Form
  Ui::SortTableWorkspaceDialog m_form;
  /// Names of the columns in the workspace
  QStringList m_columnNames;
  /// Names of columns used to sort the table
  QStringList m_sortColumns;
};

} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOM_DIALOGS_SORTTABLEWORKSPACEDIALOG_H
