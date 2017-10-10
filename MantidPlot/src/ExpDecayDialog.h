/***************************************************************************
    File                 : ExpDecayDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Fit exponential decay dialog

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
#ifndef EXPDECAYDIALOG_H
#define EXPDECAYDIALOG_H

#include <QDialog>

class QPushButton;
class QLineEdit;
class QComboBox;
class QLabel;
class Graph;
class ColorBox;
class Fit;

//! Fit exponential decay dialog
class ExpDecayDialog : public QDialog {
  Q_OBJECT

public:
  ExpDecayDialog(int type, QWidget *parent = nullptr, Qt::WFlags fl = nullptr);

public slots:
  void fit();
  void setGraph(Graph *g);

private slots:
  void activateCurve(const QString &curveName);
  void changeDataRange();

signals:
  void options(const QString &, double, double, double, double, int);
  void options(const QString &, double, double, double, int);
  void options3(const QString &, double, double, double, double, double, int);

private:
  void closeEvent(QCloseEvent *) override;

  Fit *fitter;
  Graph *graph;
  int slopes;

  QPushButton *buttonFit;
  QPushButton *buttonCancel;
  QComboBox *boxName;
  QLineEdit *boxAmplitude;
  QLineEdit *boxFirst;
  QLineEdit *boxSecond;
  QLineEdit *boxThird;
  QLineEdit *boxStart;
  QLineEdit *boxYOffset;
  QLabel *thirdLabel, *dampingLabel;
  ColorBox *boxColor;
};

#endif
