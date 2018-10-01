/***************************************************************************
    File                 : TextFormatButtons.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Widget with text format buttons (connected to a
 QTextEdit)

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

#ifndef TEXTFORMATBUTTONS_H
#define TEXTFORMATBUTTONS_H

#include <QWidget>
class QTextEdit;
class QPushButton;

//! Widget with text format buttons (connected to a QTextEdit)
class TextFormatButtons : public QWidget {
  Q_OBJECT

signals:
  void formattingModified();

public:
  enum Buttons { Plot3D, AxisLabel, Legend, Equation, TexLegend };

  //! Constructor
  /**
   * \param textEdit the QTextEdit that the buttons shall affect
   * \param parent parent widget
   */
  TextFormatButtons(QTextEdit *textEdit, Buttons buttons = Plot3D,
                    QWidget *parent = nullptr);
  void setButtons(Buttons btns);

private:
  QTextEdit *connectedTextEdit;
  Buttons d_buttons;
  //! Internal function: format selected text with prefix and postfix
  void formatText(const QString &prefix, const QString &postfix);
  void init(Buttons btns);

private slots:
  //! Format selected text to fraction
  void addFraction();
  //! Format selected text to square root
  void addSquareRoot();
  //! Format selected text to subscript
  void addSubscript();
  //! Format selected text to superscript
  void addSuperscript();
  //! Format selected text to underlined
  void addUnderline();
  //! Format selected text to italics
  void addItalics();
  //! Format selected text to bold
  void addBold();
  //! Insert curve marker into the text
  void addCurve();

  //! Let the user insert lower case greek letters
  void showLowerGreek();
  //! Let the user insert capital greek letters
  void showUpperGreek();
  //! Let the user insert mathematical symbols
  void showMathSymbols();
  //! Let the user insert arrow symbols
  void showArrowSymbols();
  //! Insert 'letter' into the text
  void addSymbol(const QString &letter);
};

#endif // TEXTFORMATBUTTONS_H
