/***************************************************************************
    File                 : FunctionDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Function dialog

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef FUNCTIONDIALOG_H
#define FUNCTIONDIALOG_H

#include "Graph.h"

class QStackedWidget;
class QWidget;
class QLineEdit;
class QComboBox;
class QPushButton;
class QSpinBox;
class QLabel;
class QTextEdit;
class ApplicationWindow;

//! Function dialog
class FunctionDialog : public QDialog {
  Q_OBJECT

public:
  FunctionDialog(ApplicationWindow *app, Graph *g = nullptr,
                 Qt::WFlags fl = nullptr);

protected:
  QComboBox *boxXFunction;
  QComboBox *boxYFunction;
  QComboBox *boxPolarRadius;
  QComboBox *boxPolarTheta;
  QComboBox *boxType;
  QLineEdit *boxFrom;
  QLineEdit *boxTo;
  QLineEdit *boxParameter;
  QLineEdit *boxParFrom;
  QLineEdit *boxParTo;
  QLineEdit *boxPolarParameter;
  QLineEdit *boxPolarFrom;
  QLineEdit *boxPolarTo;
  QPushButton *buttonClear;
  QPushButton *buttonCancel;
  QPushButton *buttonOk;
  QSpinBox *boxPoints;
  QSpinBox *boxParPoints;
  QSpinBox *boxPolarPoints;
  QStackedWidget *optionStack;
  QTextEdit *boxFunction;
  QWidget *functionPage;
  QWidget *polarPage;
  QWidget *parametricPage;

protected slots:
  void raiseWidget(int index);

public slots:
  void accept() override;
  void acceptFunction();
  void acceptParametric();
  void acceptPolar();
  void setCurveToModify(Graph *g, int curve);
  void insertParamFunctionsList(const QStringList &xList,
                                const QStringList &yList);
  void insertPolarFunctionsList(const QStringList &rList,
                                const QStringList &thetaList);
  void clearList();

signals:
  void clearParamFunctionsList();
  void clearPolarFunctionsList();

private:
  ApplicationWindow *d_app;
  Graph *graph;
  int curveID;
};

#endif // FUNCTIONDIALOG_H
