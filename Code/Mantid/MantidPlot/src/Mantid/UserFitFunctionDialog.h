#ifndef USERFITFUNCTIONDIALOG_H
#define USERFITFUNCTIONDIALOG_H

//----------------------------
//   Includes
//----------------------------

#include <qdialog.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <boost/shared_ptr.hpp>
#include "ui_UserFitFunctionDialog.h"

//----------------------------
//   Forward declarations
//----------------------------

class MantidUI;

/** 
    This is a dialog for constructing fitting functions.

    @author Roman Tolchenov, Tessella plc
    @date 23/09/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class UserFitFunctionDialog : public QDialog
{
  Q_OBJECT

public:
  
  /// Default constructor
  UserFitFunctionDialog(QWidget *parent);

  /// The constructed expression
  QString expression()const{return ui.teExpression->toPlainText();}

  /// Peak parameters. Empty if the function is not a peak
  QString peakParams()const{return ui.lePeakParams->text();}

  /// Width formula - 
  QString widthFormula()const{return ui.leWidthFormula->text();}

private slots:

  /// Add the selected function(s) from ui.treeFunctions to the edit window
  void addFunction();

  /// Add the selected function(s) from ui.treeFunctions to the edit window
  void multiplyFunction();

  /// Add the selected function(s) from ui.treeFunctions to the edit window
  void insertFunction();

  /// React on the change of selection in ui.treeFunctions (e.g. enable or disable ui.btnAdd)
  void functionSelectionChanged();

private:

  /// Add the selected function(s) from ui.treeFunctions to the edit window
  void addFunction(const QString& op,bool brackets);

  // The form generated with Qt Designer
  Ui::UserFitFunctionDialog ui;

};



#endif /* USERFITFUNCTIONDIALOG_H */