// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef USERFITFUNCTIONDIALOG_H
#define USERFITFUNCTIONDIALOG_H

//----------------------------
//   Includes
//----------------------------

#include "ui_UserFitFunctionDialog.h"
#include <boost/shared_ptr.hpp>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlineedit.h>

//----------------------------
//   Forward declarations
//----------------------------

class MantidUI;

/**
    This is a dialog for constructing fitting functions.

    @author Roman Tolchenov, Tessella plc
    @date 23/09/2009
*/
class UserFitFunctionDialog : public QDialog {
  Q_OBJECT

public:
  /// Default constructor
  explicit UserFitFunctionDialog(QWidget *parent);

  /// The constructed expression
  QString expression() const { return ui.teExpression->toPlainText(); }

  /// Peak parameters. Empty if the function is not a peak
  QString peakParams() const { return ui.lePeakParams->text(); }

  /// Width formula -
  QString widthFormula() const { return ui.leWidthFormula->text(); }

private slots:

  /// Add the selected function(s) from ui.treeFunctions to the edit window
  void addFunction();

  /// Add the selected function(s) from ui.treeFunctions to the edit window
  void multiplyFunction();

  /// Add the selected function(s) from ui.treeFunctions to the edit window
  void insertFunction();

  /// React on the change of selection in ui.treeFunctions (e.g. enable or
  /// disable ui.btnAdd)
  void functionSelectionChanged();

private:
  /// Add the selected function(s) from ui.treeFunctions to the edit window
  void addFunction(const QString &op, bool brackets);

  // The form generated with Qt Designer
  Ui::UserFitFunctionDialog ui;
};

#endif /* USERFITFUNCTIONDIALOG_H */