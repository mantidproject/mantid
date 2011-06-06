/***************************************************************************
    File                 : LineNumberDisplay.cpp
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
#include "LineNumberDisplay.h"
#include <QScrollBar>
#include <QShowEvent>
#include <QPainter>

LineNumberDisplay::LineNumberDisplay(QTextEdit *te, QWidget *parent)
		 : QTextEdit(parent), d_text_edit(te)
{
	setReadOnly(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setFrameStyle(QFrame::Panel | QFrame::Raised);
	setMaximumWidth(0);
	setLineWidth(0);
	setFocusPolicy(Qt::NoFocus);
	setCurrentFont(te->currentFont());
	viewport()->setCursor(Qt::ArrowCursor);

	QPalette palette = this->palette();
	palette.setColor(QPalette::Highlight, palette.color(QPalette::Base));
	setPalette(palette);

	if (te){
		connect(this, SIGNAL(selectionChanged()), this, SLOT(updateDocumentSelection()));

		connect(te->document(), SIGNAL(contentsChanged()), this, SLOT(updateLineNumbers()));
		connect((QObject *)te->verticalScrollBar(), SIGNAL(valueChanged(int)),
			(QObject *)verticalScrollBar(), SLOT(setValue(int)));
        connect(te, SIGNAL(currentCharFormatChanged (const QTextCharFormat &)),
                this, SLOT(changeCharFormat (const QTextCharFormat &)));
	}
}

void LineNumberDisplay::updateDocumentSelection()
{
	if (!isVisible() || !d_text_edit)
		return;

	QTextCursor c = textCursor();
#if QT_VERSION >= 0x040500
	int selectionStart = document()->findBlock(c.selectionStart()).firstLineNumber();
	int selectionEnd = document()->findBlock(c.selectionEnd()).firstLineNumber();
#else
	int selectionStart = document()->findBlock(c.selectionStart()).blockNumber();
	int selectionEnd = document()->findBlock(c.selectionEnd()).blockNumber();
#endif
	int selectedLines = abs(selectionEnd - selectionStart);

	QTextCursor cursor(d_text_edit->textCursor());
	cursor.movePosition(QTextCursor::Start);
	for (int i = 0; i < selectionStart; i++)
		cursor.movePosition(QTextCursor::Down);

	for (int i = 0; i < selectedLines; i++)
		cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);

	cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

	if (selectionEnd == d_text_edit->document()->blockCount() - 1)
		cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);

	d_text_edit->setTextCursor(cursor);
}

void LineNumberDisplay::updateLineNumbers(bool force)
{
	if (!isVisible() || !d_text_edit)
		return;

	int lines = d_text_edit->document()->blockCount();
	if (document()->blockCount() - 1 == lines && !force)
		return;

	QString aux;
	for(int i = 0; i < lines; i++)
		aux += QString::number(i + 1) + "\n";

	setPlainText(aux);

	QFontMetrics fm(d_text_edit->currentFont(), this);
	setMaximumWidth(2*fm.boundingRect(QString::number(lines)).width());
	verticalScrollBar()->setValue(d_text_edit->verticalScrollBar()->value());
}

void LineNumberDisplay::showEvent(QShowEvent *e)
{
	e->accept();
	if (isVisible())
		updateLineNumbers();
}

void LineNumberDisplay::changeCharFormat (const QTextCharFormat &f)
{
    setCurrentFont(f.font());
}
