/***************************************************************************
    File                 : FilterDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Filter options dialog

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
#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QDialog>

class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class Graph;
class ColorBox;

//! Filter options dialog
class FilterDialog : public QDialog {
  Q_OBJECT

public:
  FilterDialog(int type, QWidget *parent = nullptr, Qt::WFlags fl = nullptr);

public slots:
  void setGraph(Graph *g);
  void filter();

private:
  Graph *graph;
  int filter_type;

  QPushButton *buttonFilter;
  QPushButton *buttonCancel;
  QComboBox *boxName;
  QCheckBox *boxOffset;
  QLineEdit *boxStart;
  QLineEdit *boxEnd;
  ColorBox *boxColor;
};

#endif
