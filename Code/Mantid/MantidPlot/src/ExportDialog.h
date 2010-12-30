/***************************************************************************
    File                 : ExportDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Export ASCII dialog

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
#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
class QPushButton;
class QCheckBox;
class QComboBox;
class QLabel;

//! Export ASCII dialog
class ExportDialog : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param tableName active table name
	 * \param parent parent widget
	 * \param fl window flags
	 */
    ExportDialog(const QString& tableName, QWidget* parent = 0, Qt::WFlags fl = 0 );

private:
	void closeEvent(QCloseEvent*);

    QPushButton* buttonOk;
	QPushButton* buttonCancel;
	QPushButton* buttonHelp;
    QCheckBox* boxNames;
    QCheckBox* boxComments;
    QCheckBox* boxSelection;
	QCheckBox* boxAllTables;
    QComboBox* boxSeparator;
	QComboBox* boxTable;
	QLabel *sepText;

public slots:
	//! Set the column delimiter
	void setColumnSeparator(const QString& sep);

private slots:
	//! Enable/disable the tables combox box
	/**
	 * The tables combo box is disabled when
	 * the checkbox "all" is selected.
	 */
	void enableTableName(bool ok);

	//! Enable/disable export options depending if the selected window is a Table or a Matrix.
	void updateOptions(const QString & name);

protected slots:
	//! Accept changes
	void accept();
	//! Display help
	void help();
};

#endif // ExportDialog_H
