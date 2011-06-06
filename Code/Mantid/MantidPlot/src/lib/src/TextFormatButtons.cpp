/***************************************************************************
    File                 : TextFormatButtons.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Widget with text format buttons (connected to a QTextEdit)

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

#include "TextFormatButtons.h"
#include "SymbolDialog.h"
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QString>

TextFormatButtons::TextFormatButtons(QTextEdit * textEdit, Buttons buttons, QWidget * parent)
: QWidget(parent),
connectedTextEdit(textEdit),
d_buttons(buttons)
{
	QHBoxLayout * layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);

	init(buttons);
}

void TextFormatButtons::init(Buttons buttons)
{
	QHBoxLayout *layout = (QHBoxLayout*)this->layout();
	QLayoutItem *child;
	while ((child = layout->takeAt(0)) != 0){
		if (child->widget())
			delete child->widget();
	}

	QFont font = QFont();
	int btnSize = 32;
#ifdef Q_OS_MAC
	btnSize = 38;
#endif
	if (buttons == Legend || buttons == TexLegend){
		QPushButton *buttonCurve = new QPushButton( QPixmap(":/lineSymbol.png"), QString());
		buttonCurve->setFixedWidth(btnSize);
		buttonCurve->setFixedHeight(btnSize);
		buttonCurve->setFont(font);
		layout->addWidget(buttonCurve);
		connect( buttonCurve, SIGNAL(clicked()), this, SLOT(addCurve()) );
	}

	QPushButton *buttonSubscript = new QPushButton(QPixmap(":/index.png"), QString());
	buttonSubscript->setFixedWidth(btnSize);
	buttonSubscript->setFixedHeight(btnSize);
	buttonSubscript->setFont(font);
	layout->addWidget(buttonSubscript);
	connect( buttonSubscript, SIGNAL(clicked()), this, SLOT(addSubscript()) );

	QPushButton *buttonSuperscript = new QPushButton(QPixmap(":/exp.png"), QString());
	buttonSuperscript->setFixedWidth(btnSize);
	buttonSuperscript->setFixedHeight(btnSize);
	buttonSuperscript->setFont(font);
	layout->addWidget(buttonSuperscript);
	connect( buttonSuperscript, SIGNAL(clicked()), this, SLOT(addSuperscript()));

	if (buttons == Equation || buttons == TexLegend){
		QPushButton *buttonFraction = new QPushButton(QPixmap(":/fraction.png"), QString());
		buttonFraction->setFixedWidth(btnSize);
		buttonFraction->setFixedHeight(btnSize);
		buttonFraction->setFont(font);
		layout->addWidget(buttonFraction);
		connect(buttonFraction, SIGNAL(clicked()), this, SLOT(addFraction()));

		QPushButton *buttonSquareRoot = new QPushButton(QPixmap(":/square_root.png"), QString());
		buttonSquareRoot->setFixedWidth(btnSize);
		buttonSquareRoot->setFixedHeight(btnSize);
		buttonSquareRoot->setFont(font);
		layout->addWidget(buttonSquareRoot);
		connect(buttonSquareRoot, SIGNAL(clicked()), this, SLOT(addSquareRoot()));
	}

	QPushButton *buttonLowerGreek = new QPushButton(QString(QChar(0x3B1)));
	buttonLowerGreek->setFont(font);
	buttonLowerGreek->setFixedWidth(btnSize);
	buttonLowerGreek->setFixedHeight(btnSize);
	layout->addWidget(buttonLowerGreek);
	connect( buttonLowerGreek, SIGNAL(clicked()), this, SLOT(showLowerGreek()));

	QPushButton *buttonUpperGreek = new QPushButton(QString(QChar(0x393)));
	buttonUpperGreek->setFont(font);
	buttonUpperGreek->setFixedWidth(btnSize);
	buttonUpperGreek->setFixedHeight(btnSize);
	layout->addWidget(buttonUpperGreek);
	connect( buttonUpperGreek, SIGNAL(clicked()), this, SLOT(showUpperGreek()));

	QPushButton *buttonArrowSymbols = new QPushButton(QString(QChar(0x2192)));
	buttonArrowSymbols->setFont(font);
	buttonArrowSymbols->setFixedWidth(btnSize);
	buttonArrowSymbols->setFixedHeight(btnSize);
	layout->addWidget(buttonArrowSymbols);
	connect( buttonArrowSymbols, SIGNAL(clicked()), this, SLOT(showArrowSymbols()));

	QPushButton *buttonMathSymbols = new QPushButton(QString(QChar(0x222B)));
	buttonMathSymbols->setFont(font);
	buttonMathSymbols->setFixedWidth(btnSize);
	buttonMathSymbols->setFixedHeight(btnSize);
	layout->addWidget(buttonMathSymbols);
	connect( buttonMathSymbols, SIGNAL(clicked()), this, SLOT(showMathSymbols()));

	if (buttons != Plot3D && buttons != Equation && buttons != TexLegend){
		font = this->font();
		font.setBold(true);

		QPushButton *buttonBold = new QPushButton(tr("B","Button bold"));
		buttonBold->setFont(font);
		buttonBold->setFixedWidth(btnSize);
		buttonBold->setFixedHeight(btnSize);
		layout->addWidget(buttonBold);
		connect( buttonBold, SIGNAL(clicked()), this, SLOT(addBold()));

		font = this->font();
		font.setItalic(true);

		QPushButton *buttonItalics = new QPushButton(tr("It","Button italics"));
		buttonItalics->setFont(font);
		buttonItalics->setFixedWidth(btnSize);
		buttonItalics->setFixedHeight(btnSize);
		layout->addWidget(buttonItalics);
		connect( buttonItalics, SIGNAL(clicked()), this, SLOT(addItalics()));

		font = this->font();
		font.setUnderline(true);

		QPushButton *buttonUnderline = new QPushButton(tr("U","Button underline"));
		buttonUnderline->setFont(font);
		buttonUnderline->setFixedWidth(btnSize);
		buttonUnderline->setFixedHeight(btnSize);
		layout->addWidget(buttonUnderline);
   		layout->addStretch();
		connect( buttonUnderline, SIGNAL(clicked()), this, SLOT(addUnderline()));
	} else
		layout->addStretch();
}

void TextFormatButtons::showLowerGreek()
{
	SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::lowerGreek, this, Qt::Tool|Qt::WindowStaysOnTopHint);
	greekLetters->setAttribute(Qt::WA_DeleteOnClose);
	QFont f = connectedTextEdit->font();
	f.setPointSize(12);
	greekLetters->setFont(f);
	connect(greekLetters, SIGNAL(addLetter(const QString&)), this, SLOT(addSymbol(const QString&)));
	greekLetters->show();
	greekLetters->setFocus();
}

void TextFormatButtons::showUpperGreek()
{
	SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::upperGreek, this, Qt::Tool|Qt::WindowStaysOnTopHint);
	greekLetters->setAttribute(Qt::WA_DeleteOnClose);
	QFont f = connectedTextEdit->font();
	f.setPointSize(12);
	greekLetters->setFont(f);
	connect(greekLetters, SIGNAL(addLetter(const QString&)), this, SLOT(addSymbol(const QString&)));
	greekLetters->show();
	greekLetters->setFocus();
}

void TextFormatButtons::showMathSymbols()
{
	SymbolDialog::CharSet charSet = SymbolDialog::mathSymbols;
	if (d_buttons == Equation || d_buttons == TexLegend)
		charSet = SymbolDialog::latexMathSymbols;

	SymbolDialog *mathSymbols = new SymbolDialog(charSet, this, Qt::Tool|Qt::WindowStaysOnTopHint);
	mathSymbols->setAttribute(Qt::WA_DeleteOnClose);
	QFont f = connectedTextEdit->font();
	f.setPointSize(12);
	mathSymbols->setFont(f);
	connect(mathSymbols, SIGNAL(addLetter(const QString&)), this, SLOT(addSymbol(const QString&)));
	mathSymbols->show();
	mathSymbols->setFocus();
}

void TextFormatButtons::showArrowSymbols()
{
	SymbolDialog::CharSet charSet = SymbolDialog::arrowSymbols;
	if (d_buttons == Equation || d_buttons == TexLegend)
		charSet = SymbolDialog::latexArrowSymbols;

	SymbolDialog *arrowSymbols = new SymbolDialog(charSet, this, Qt::Tool|Qt::WindowStaysOnTopHint);
	arrowSymbols->setAttribute(Qt::WA_DeleteOnClose);
	arrowSymbols->setFont(connectedTextEdit->font());
	QFont f = connectedTextEdit->font();
	f.setPointSize(12);
	arrowSymbols->setFont(f);
	connect(arrowSymbols, SIGNAL(addLetter(const QString&)), this, SLOT(addSymbol(const QString&)));
	arrowSymbols->show();
	arrowSymbols->setFocus();
}

void TextFormatButtons::addSymbol(const QString & letter)
{
	if (d_buttons == Equation || d_buttons == TexLegend){
		int s = 0x3B1;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\alpha");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\beta");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\gamma");
		else if (letter == QString(QChar(3 + s)))
			connectedTextEdit->textCursor().insertText("\\delta");
		else if (letter == QString(QChar(4 + s)))
			connectedTextEdit->textCursor().insertText("\\epsilon");
		else if (letter == QString(QChar(5 + s)))
			connectedTextEdit->textCursor().insertText("\\zeta");
		else if (letter == QString(QChar(6 + s)))
			connectedTextEdit->textCursor().insertText("\\eta");
		else if (letter == QString(QChar(7 + s)))
			connectedTextEdit->textCursor().insertText("\\theta");
		else if (letter == QString(QChar(8 + s)))
			connectedTextEdit->textCursor().insertText("\\iota");
		else if (letter == QString(QChar(9 + s)))
			connectedTextEdit->textCursor().insertText("\\kappa");
		else if (letter == QString(QChar(10 + s)))
			connectedTextEdit->textCursor().insertText("\\lambda");
		else if (letter == QString(QChar(11 + s)))
			connectedTextEdit->textCursor().insertText("\\mu");
		else if (letter == QString(QChar(12 + s)))
			connectedTextEdit->textCursor().insertText("\\nu");
		else if (letter == QString(QChar(13 + s)))
			connectedTextEdit->textCursor().insertText("\\xi");
		else if (letter == QString(QChar(14 + s)))
			connectedTextEdit->textCursor().insertText("\\\\o");
		else if (letter == QString(QChar(15 + s)))
			connectedTextEdit->textCursor().insertText("\\pi");
		else if (letter == QString(QChar(16 + s)))
			connectedTextEdit->textCursor().insertText("\\rho");
		else if (letter == QString(QChar(17 + s)))
			connectedTextEdit->textCursor().insertText("\\varsigma");
		else if (letter == QString(QChar(18 + s)))
			connectedTextEdit->textCursor().insertText("\\sigma");
		else if (letter == QString(QChar(19 + s)))
			connectedTextEdit->textCursor().insertText("\\tau");
		else if (letter == QString(QChar(20 + s)))
			connectedTextEdit->textCursor().insertText("\\upsilon");
		else if (letter == QString(QChar(21 + s)))
			connectedTextEdit->textCursor().insertText("\\varphi");
		else if (letter == QString(QChar(22 + s)))
			connectedTextEdit->textCursor().insertText("\\chi");
		else if (letter == QString(QChar(23 + s)))
			connectedTextEdit->textCursor().insertText("\\psi");
		else if (letter == QString(QChar(24 + s)))
			connectedTextEdit->textCursor().insertText("\\omega");

		s = 0x393;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\Gamma");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\Delta");
		else if (letter == QString(QChar(5 + s)))
			connectedTextEdit->textCursor().insertText("\\Theta");
		else if (letter == QString(QChar(8 + s)))
			connectedTextEdit->textCursor().insertText("\\Lambda");
		else if (letter == QString(QChar(11 + s)))
			connectedTextEdit->textCursor().insertText("\\Xi");
		else if (letter == QString(QChar(13 + s)))
			connectedTextEdit->textCursor().insertText("\\Pi");
		else if (letter == QString(QChar(16 + s)))
			connectedTextEdit->textCursor().insertText("\\Sigma");
		else if (letter == QString(QChar(19 + s)))
			connectedTextEdit->textCursor().insertText("\\Phi");
		else if (letter == QString(QChar(21 + s)))
			connectedTextEdit->textCursor().insertText("\\Psi");
		else if (letter == QString(QChar(22 + s)))
			connectedTextEdit->textCursor().insertText("\\Omega");

		s = 0x2190;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\leftarrow");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\uparrow");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\rightarrow");
		else if (letter == QString(QChar(3 + s)))
			connectedTextEdit->textCursor().insertText("\\downarrow");
		else if (letter == QString(QChar(4 + s)))
			connectedTextEdit->textCursor().insertText("\\leftrightarrow");
		else if (letter == QString(QChar(5 + s)))
			connectedTextEdit->textCursor().insertText("\\updownarrow");
		else if (letter == QString(QChar(6 + s)))
			connectedTextEdit->textCursor().insertText("\\nwarrow");
		else if (letter == QString(QChar(7 + s)))
			connectedTextEdit->textCursor().insertText("\\nearrow");
		else if (letter == QString(QChar(8 + s)))
			connectedTextEdit->textCursor().insertText("\\searrow");
		else if (letter == QString(QChar(9 + s)))
			connectedTextEdit->textCursor().insertText("\\swarrow");

		s = 0x21D0;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\Leftarrow");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\Uparrow");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\Rightarrow");
		else if (letter == QString(QChar(3 + s)))
			connectedTextEdit->textCursor().insertText("\\Downarrow");
		else if (letter == QString(QChar(4 + s)))
			connectedTextEdit->textCursor().insertText("\\Leftrightarrow");
		else if (letter == QString(QChar(5 + s)))
			connectedTextEdit->textCursor().insertText("\\Updownarrow");

		if (letter == QString(QChar(0x21A6)))
			connectedTextEdit->textCursor().insertText("\\mapsto");
		else if (letter == QString(QChar(0x21A9)))
			connectedTextEdit->textCursor().insertText("\\hookleftarrow");
		else if (letter == QString(QChar(0x21AA)))
			connectedTextEdit->textCursor().insertText("\\hookrightarrow");
		else if (letter == QString(QChar(0x21BC)))
			connectedTextEdit->textCursor().insertText("\\leftharpoonup");
		else if (letter == QString(QChar(0x21BD)))
			connectedTextEdit->textCursor().insertText("\\leftharpoondown");
		else if (letter == QString(QChar(0x21C0)))
			connectedTextEdit->textCursor().insertText("\\rightharpoonup");
		else if (letter == QString(QChar(0x21C1)))
			connectedTextEdit->textCursor().insertText("\\rightharpoondown");
		else if (letter == QString(QChar(0x21CC)))
			connectedTextEdit->textCursor().insertText("\\rightleftharpoons");

		s = 0x2200;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\forall");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\partial");
		else if (letter == QString(QChar(3 + s)))
			connectedTextEdit->textCursor().insertText("\\exists");
		else if (letter == QString(QChar(4 + s)))
			connectedTextEdit->textCursor().insertText("\\not\\exists");
		else if (letter == QString(QChar(5 + s)))
			connectedTextEdit->textCursor().insertText("\\oslash");
		else if (letter == QString(QChar(6 + s)))
			connectedTextEdit->textCursor().insertText("\\Delta");
		else if (letter == QString(QChar(7 + s)))
			connectedTextEdit->textCursor().insertText("\\nabla");
		else if (letter == QString(QChar(8 + s)))
			connectedTextEdit->textCursor().insertText("\\in");
		else if (letter == QString(QChar(9 + s)))
			connectedTextEdit->textCursor().insertText("\\notin");
		else if (letter == QString(QChar(11 + s)))
			connectedTextEdit->textCursor().insertText("\\ni");
		else if (letter == QString(QChar(12 + s)))
			connectedTextEdit->textCursor().insertText("\\not\\ni");

		s = 0x220F;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\prod");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\coprod");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\sum");

		if (letter == QString(QChar(0x00B1)))
			connectedTextEdit->textCursor().insertText("\\pm");
		else if (letter == QString(QChar(0x2213)))
			connectedTextEdit->textCursor().insertText("\\mp");
		else if (letter == QString(QChar(0x00D7)))
			connectedTextEdit->textCursor().insertText("\\times");

		s = 0x2217;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\ast");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\circ");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\bullet");
		else if (letter == QString(QChar(3 + s)))
			connectedTextEdit->textCursor().insertText("\\surd");
		else if (letter == QString(QChar(4 + s)))
			connectedTextEdit->textCursor().insertText("\\sqrt[3]{}");
		else if (letter == QString(QChar(5 + s)))
			connectedTextEdit->textCursor().insertText("\\sqrt[4]{}");
		else if (letter == QString(QChar(6 + s)))
			connectedTextEdit->textCursor().insertText("\\propto");
		else if (letter == QString(QChar(7 + s)))
			connectedTextEdit->textCursor().insertText("\\infty");

		s = 0x2227;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\wedge");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\vee");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\cap");
		else if (letter == QString(QChar(3 + s)))
			connectedTextEdit->textCursor().insertText("\\cup");
		else if (letter == QString(QChar(4 + s)))
			connectedTextEdit->textCursor().insertText("\\int");
		else if (letter == QString(QChar(5 + s)))
			connectedTextEdit->textCursor().insertText("\\int \\!\\!\\! \\int");
		else if (letter == QString(QChar(6 + s)))
			connectedTextEdit->textCursor().insertText("\\int \\!\\!\\! \\int \\!\\!\\! \\int");
		else if (letter == QString(QChar(7 + s)))
			connectedTextEdit->textCursor().insertText("\\oint");

		if (letter == QString(QChar(0x223F)))
			connectedTextEdit->textCursor().insertText("\\sim");
		else if (letter == QString(QChar(0x2245)))
			connectedTextEdit->textCursor().insertText("\\cong");
		else if (letter == QString(QChar(0x2248)))
			connectedTextEdit->textCursor().insertText("\\approx");

		s = 0x2260;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\not=");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\equiv");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\not\\equiv");

		s = 0x2264;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\le");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\ge");

		s = 0x226A;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\ll");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\gg");

		s = 0x2282;
		if (letter == QString(QChar(s)))
			connectedTextEdit->textCursor().insertText("\\subset");
		else if (letter == QString(QChar(1 + s)))
			connectedTextEdit->textCursor().insertText("\\supset");
		else if (letter == QString(QChar(2 + s)))
			connectedTextEdit->textCursor().insertText("\\not\\subset");
		else if (letter == QString(QChar(3 + s)))
			connectedTextEdit->textCursor().insertText("\\not\\supset");
		else if (letter == QString(QChar(4 + s)))
			connectedTextEdit->textCursor().insertText("\\subseteq");
		else if (letter == QString(QChar(5 + s)))
			connectedTextEdit->textCursor().insertText("\\supseteq");
		else if (letter == QString(QChar(6 + s)))
			connectedTextEdit->textCursor().insertText("\\not\\subseteq");
		else if (letter == QString(QChar(7 + s)))
			connectedTextEdit->textCursor().insertText("\\not\\supseteq");

		if (letter == QString(QChar(0x210F)))
			connectedTextEdit->textCursor().insertText("\\hbar");
		else if (letter == QString(QChar(0x212B)))
			connectedTextEdit->textCursor().insertText("\\AA");
	} else
		connectedTextEdit->textCursor().insertText(letter);
}

void TextFormatButtons::addCurve()
{
	formatText("\\l(",")");
}

void TextFormatButtons::addUnderline()
{
	formatText("<u>","</u>");
}

void TextFormatButtons::addItalics()
{
	formatText("<i>","</i>");
}

void TextFormatButtons::addBold()
{
	formatText("<b>","</b>");
}

void TextFormatButtons::addSubscript()
{
	if (d_buttons == TexLegend || d_buttons == Equation || d_buttons == Plot3D)
		formatText("_{","}");
	else
		formatText("<sub>","</sub>");
}

void TextFormatButtons::addSuperscript()
{
	if (d_buttons == TexLegend || d_buttons == Equation  || d_buttons == Plot3D)
		formatText("^{","}");
	else
		formatText("<sup>","</sup>");
}

void TextFormatButtons::addFraction()
{
	if (d_buttons == TexLegend || d_buttons == Equation)
		formatText("\\frac{","}{}");
}

void TextFormatButtons::addSquareRoot()
{
	if (d_buttons == TexLegend || d_buttons == Equation)
		formatText("\\sqrt{","}");
}

void TextFormatButtons::formatText(const QString & prefix, const QString & postfix)
{
	QTextCursor cursor = connectedTextEdit->textCursor();
	QString markedText = cursor.selectedText();
	cursor.insertText(prefix+markedText+postfix);
	if(markedText.isEmpty())
	{
		// if no text is marked, place cursor inside the <..></..> statement
		// instead of after it
		cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::MoveAnchor,postfix.size());
		// the next line makes the selection visible to the user
		// (the line above only changes the selection in the
		// underlying QTextDocument)
		connectedTextEdit->setTextCursor(cursor);
	}
	// give focus back to text edit
	connectedTextEdit->setFocus();
}

void TextFormatButtons::setButtons(Buttons btns)
{
	if (btns == d_buttons)
		return;

	d_buttons = btns;
	init(d_buttons);
}
