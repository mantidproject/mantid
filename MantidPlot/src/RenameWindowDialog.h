/***************************************************************************
    File                 : RenameWindowDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Rename window dialog

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
#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include <qdialog.h>
#include <qvariant.h>

#include "MdiSubWindow.h"

class QGroupBox;
class QPushButton;
class QLineEdit;
class QRadioButton;
class QTextEdit;
class MdiSubWindow;
class QButtonGroup;

//! Rename window dialog
class RenameWindowDialog : public QDialog {
  Q_OBJECT

public:
  RenameWindowDialog(QWidget *parent = nullptr, Qt::WFlags fl = nullptr);

private:
  QPushButton *buttonOk;
  QPushButton *buttonCancel;
  QGroupBox *groupBox1;
  QButtonGroup *buttons;
  QRadioButton *boxName;
  QRadioButton *boxLabel;
  QRadioButton *boxBoth;
  QLineEdit *boxNameLine;
  QTextEdit *boxLabelEdit;

public slots:
  void setWidget(MdiSubWindow *w);
  MdiSubWindow::CaptionPolicy getCaptionPolicy();
  void accept() override;

signals:

private:
  MdiSubWindow *window;
};

#endif // EXPORTDIALOG_H
