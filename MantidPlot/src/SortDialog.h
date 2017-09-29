/***************************************************************************
    File                 : SortDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Sort table dialog

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
#ifndef SORTDIALOG_H
#define SORTDIALOG_H

#include <QDialog>

class QPushButton;
class QComboBox;

//! Sorting options dialog
class SortDialog : public QDialog {
  Q_OBJECT

public:
  SortDialog(QWidget *parent = nullptr, Qt::WFlags fl = nullptr);
  void insertColumnsList(const QStringList &cols);

private slots:
  void accept() override;
  void changeType(int index);

signals:
  void sort(int, int, const QString &);

private:
  QPushButton *buttonOk;
  QPushButton *buttonCancel;
  QComboBox *boxType;
  QComboBox *boxOrder;
  QComboBox *columnsList;
};

#endif
