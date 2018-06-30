/***************************************************************************
    File                 : TextEditor.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : A QwtText editor

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
#include "TextEditor.h"
#include "LegendWidget.h"
#include "QwtPieCurve.h"

#include <QCloseEvent>
#include <QTextCursor>

#include <qwt_scale_widget.h>
#include <qwt_text.h>
#include <qwt_text_label.h>

TextEditor::TextEditor(Graph *g) : QTextEdit(g), d_target(nullptr) {
  setAttribute(Qt::WA_DeleteOnClose);
  setFrameShadow(QFrame::Plain);
  setFrameShape(QFrame::Box);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QPalette palette = this->palette();
  palette.setColor(QPalette::Active, QPalette::WindowText, Qt::blue);
  palette.setColor(QPalette::Active, QPalette::Base, Qt::white);
  setPalette(palette);

  QString text;
  if (g->selectedText()) {
    d_target = g->selectedText();
    setGeometry(d_target->geometry());
    auto legend = dynamic_cast<LegendWidget *>(d_target);
    text = legend ? legend->text() : "";
    d_target->hide();
  } else if (g->titleSelected()) {
    d_target = g->plotWidget()->titleLabel();
    QwtText t = g->plotWidget()->title();
    text = t.text();
    setAlignment((Qt::Alignment)t.renderFlags());
    setGeometry(d_target->geometry());
  } else if (g->selectedScale()) {
    d_target = g->selectedScale();
    QwtScaleWidget *scale = (QwtScaleWidget *)d_target;
    QwtText t = scale->title();
    text = t.text();
    setAlignment((Qt::Alignment)t.renderFlags());

    QRect rect = g->axisTitleRect(scale);
    if (scale->alignment() == QwtScaleDraw::BottomScale ||
        scale->alignment() == QwtScaleDraw::TopScale) {
      resize(rect.size());
      move(QPoint(d_target->x() + rect.x(), d_target->y() + rect.y()));
    } else {
      resize(QSize(rect.height(), rect.width()));
      if (scale->alignment() == QwtScaleDraw::LeftScale)
        move(QPoint(d_target->x() + rect.x(),
                    d_target->y() + rect.y() + rect.height() / 2));
      else if (scale->alignment() == QwtScaleDraw::RightScale)
        move(QPoint(d_target->x() - rect.height(),
                    d_target->y() + rect.y() + rect.height() / 2));

      t.setText(" ");
      t.setBackgroundPen(QPen(Qt::NoPen));
      scale->setTitle(t);
    }
  }

  QTextCursor cursor = textCursor();
  cursor.insertText(text);
  d_initial_text = text;

  show();
  setFocus();
}

TextEditor::~TextEditor() { emit textEditorDeleted(); }

void TextEditor::closeEvent(QCloseEvent *e) {
  if (d_target != nullptr) {
    Graph *g = dynamic_cast<Graph *>(parent());
    if (g) {
      QString s = QString();
      if (auto legend = dynamic_cast<LegendWidget *>(d_target)) {
        s = toPlainText();
        legend->setText(s);
        d_target->show();
        g->setSelectedText(nullptr);
      } else if (auto pieLabel = dynamic_cast<PieLabel *>(d_target)) {
        s = toPlainText();
        pieLabel->setCustomText(s);
        d_target->show();
        g->setSelectedText(nullptr);
      } else if (QString(d_target->metaObject()->className()) ==
                 "QwtTextLabel") {
        QwtText title = g->plotWidget()->title();
        s = toPlainText();
        if (s.isEmpty())
          s = " ";
        title.setText(s);
        g->plotWidget()->setTitle(title);
      } else if (QString(d_target->metaObject()->className()) ==
                 "QwtScaleWidget") {
        QwtScaleWidget *scale = (QwtScaleWidget *)d_target;
        QwtText title = scale->title();
        s = toPlainText();
        if (s.isEmpty())
          s = " ";
        title.setText(s);
        scale->setTitle(title);
      }

      if (d_initial_text != s)
        g->notifyChanges();

      d_target->repaint();
    }
  }
  e->accept();
}

void TextEditor::formatText(const QString &prefix, const QString &postfix) {
  QTextCursor cursor = textCursor();
  QString markedText = cursor.selectedText();
  cursor.insertText(prefix + markedText + postfix);
  if (markedText.isEmpty()) {
    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor,
                        postfix.size());
    setTextCursor(cursor);
  }
  setFocus();
}

void TextEditor::addSymbol(const QString &letter) {
  textCursor().insertText(letter);
}
