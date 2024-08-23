// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------
//   Includes
//----------------------------

#include "DllOption.h"
#include <QDialog>
#include <QListWidget>
#include <QStringList>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

/**
    This is a dialog for selecting workspaces.

    @author Roman Tolchenov, Tessella plc
    @date 22/06/2010
*/
class EXPORT_OPT_MANTIDQT_COMMON SelectWorkspacesDialog : public QDialog {
  Q_OBJECT

public:
  /// return value of the Custom button
  static const int CustomButton = 45654; // do not use this number direct, just
                                         // refer to this static constant

  /// Constructor
  SelectWorkspacesDialog(QWidget *parent = nullptr, const std::string &typeFilter = "",
                         const std::string &customButtonLabel = "",
                         QAbstractItemView::SelectionMode mode = QAbstractItemView::MultiSelection);

  /// Return the selected names
  QStringList getSelectedNames() const;

private slots:

  /// Slot to monitor the workspace selection status
  void selectionChanged();

  /// slot to handle the custom button press
  void customButtonPress();

private:
  /// Displays available workspace names
  QListWidget *m_wsList;
  /// The OK button
  QPushButton *m_okButton;
  /// The OK button
  QPushButton *m_customButton;
};
} // namespace MantidWidgets
} // namespace MantidQt
