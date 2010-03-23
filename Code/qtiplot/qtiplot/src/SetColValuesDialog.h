/***************************************************************************
    File                 : SetColValuesDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net,
                           knut.franke*gmx.de
    Description          : Set column values dialog

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
#ifndef VALUESDIALOG_H
#define VALUESDIALOG_H

#include "ScriptingEnv.h"
#include "Scripted.h"
#include <QDialog>

class QComboBox;
class QTextEdit;
class QSpinBox;
class QPushButton;
class QLabel;
#ifdef SCRIPTING_PYTHON
class QCheckBox;
#endif
class Table;
class ScriptingEnv;
class ScriptEdit;


//! Set column values dialog
class SetColValuesDialog : public QDialog, public Scripted
{
    Q_OBJECT

public:
    SetColValuesDialog( ScriptingEnv *env, QWidget* parent = 0, Qt::WFlags fl = 0 );
	void setTable(Table* w);

private slots:
	bool apply();
	void prevColumn();
	void nextColumn();
	void insertFunction();
	void insertCol();
	void insertCell();
	void insertExplain(int index);
	void updateColumn(int sc);

private:
	Table* table;
	
	QSize sizeHint() const ;
	void customEvent( QEvent *e );

    QComboBox* functions;
    QComboBox* boxColumn;
    QPushButton* btnAddFunction;
    QPushButton* btnAddCol;
    QPushButton* btnCancel;
    QPushButton *buttonPrev;
    QPushButton *buttonNext;
    QPushButton *addCellButton;
    QPushButton *btnApply;
    ScriptEdit* commands;
    QTextEdit* explain;
	QSpinBox* start, *end;
	QLabel *colNameLabel;
#ifdef SCRIPTING_PYTHON
	QCheckBox *boxMuParser;
#endif
};

#endif //
