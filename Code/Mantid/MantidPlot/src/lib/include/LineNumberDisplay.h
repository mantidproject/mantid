/***************************************************************************
    File                 : LineNumberDisplay.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2008 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : A widget displaying line numbers for a QTextEdit

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
#ifndef LineNumberDisplay_H
#define LineNumberDisplay_H

#include <QTextEdit>

//! A QTextEdit displaying line numbers.
/**
 * It must be used in connection with another "source" QTextEdit.
 */
class LineNumberDisplay: public QTextEdit
{
    Q_OBJECT

public:
	//! Constructor
	/**
	* \param te the "source" QTextEdit for which we want to display the line numbers
	* \param parent parent widget (only affects placement of the dialog)
	*/
	LineNumberDisplay(QTextEdit *te, QWidget *parent = 0);

public slots:
	void updateLineNumbers(bool force = false);
	void updateDocumentSelection();

private slots:
	void changeCharFormat (const QTextCharFormat &);

private:
	void showEvent(QShowEvent *);
	QTextEdit *d_text_edit;
};
#endif
