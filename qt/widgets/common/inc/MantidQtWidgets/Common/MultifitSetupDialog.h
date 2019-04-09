// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MULTIFITSETUPTDIALOG_H
#define MULTIFITSETUPTDIALOG_H

#include "ui_MultifitSetupDialog.h"
#include <QDialog>

namespace MantidQt {
namespace MantidWidgets {
class FitPropertyBrowser;
/**
    This is a dialog for doing setting up the MultiBG function.


    @author Roman Tolchenov, Tessella plc
    @date 7/09/2011
*/
class MultifitSetupDialog : public QDialog {
  Q_OBJECT

public:
  /// Default constructor
  MultifitSetupDialog(FitPropertyBrowser *fitBrowser);

  /// Returns a list of parameter ties. Empty string means no ties and parameter
  /// is local
  QStringList getParameterTies() const { return m_ties; }

private slots:

  /// Setup the function and close dialog
  void accept() override;
  void cellChanged(int /*row*/, int /*col*/);

private:
  /// The form generated with Qt Designer
  Ui::MultifitSetupDialog ui;

  /// Pointer to the calling fit browser
  FitPropertyBrowser *m_fitBrowser;

  /// A list with parameter ties
  QStringList m_ties;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MULTIFITSETUPTDIALOG_H */
