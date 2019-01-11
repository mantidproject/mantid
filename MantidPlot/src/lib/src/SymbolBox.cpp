// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2006 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : SymbolBox.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "SymbolBox.h"
#include <algorithm>

#include <qpainter.h>
#include <qpixmap.h>

const QwtSymbol::Style SymbolBox::symbols[] = {
    QwtSymbol::NoSymbol,  QwtSymbol::Ellipse,   QwtSymbol::Rect,
    QwtSymbol::Diamond,   QwtSymbol::Triangle,  QwtSymbol::DTriangle,
    QwtSymbol::UTriangle, QwtSymbol::LTriangle, QwtSymbol::RTriangle,
    QwtSymbol::Cross,     QwtSymbol::XCross,    QwtSymbol::HLine,
    QwtSymbol::VLine,     QwtSymbol::Star1,     QwtSymbol::Star2,
    QwtSymbol::Hexagon};

SymbolBox::SymbolBox(bool showNoSymbol, QWidget *parent) : QComboBox(parent) {
  init(showNoSymbol);
}

void SymbolBox::init(bool showNoSymbol) {
  QPixmap icon = QPixmap(15, 15);
  QColor c = QColor(Qt::gray);
  icon.fill(c);
  const QRect r = QRect(1, 1, 14, 14);
  QPainter p(&icon);
  p.setRenderHint(QPainter::Antialiasing);
  QwtSymbol symb;
  p.setBrush(QBrush(QColor(Qt::white)));

  if (showNoSymbol)
    this->addItem(tr("No Symbol"));

  symb.setStyle(QwtSymbol::Ellipse);
  symb.draw(&p, r);
  this->addItem(icon, tr("Ellipse"));

  symb.setStyle(QwtSymbol::Rect);
  icon.fill(c);
  symb.draw(&p, r.adjusted(0, 0, -1, -1));
  this->addItem(icon, tr("Rectangle"));

  symb.setStyle(QwtSymbol::Diamond);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Diamond"));

  symb.setStyle(QwtSymbol::Triangle);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Triangle"));

  symb.setStyle(QwtSymbol::DTriangle);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Down Triangle"));

  symb.setStyle(QwtSymbol::UTriangle);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Up Triangle"));

  symb.setStyle(QwtSymbol::LTriangle);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Left Triangle"));

  symb.setStyle(QwtSymbol::RTriangle);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Right Triangle"));

  symb.setStyle(QwtSymbol::Cross);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Cross"));

  symb.setStyle(QwtSymbol::XCross);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Diagonal Cross"));

  symb.setStyle(QwtSymbol::HLine);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Horizontal Line"));

  symb.setStyle(QwtSymbol::VLine);
  p.eraseRect(r);
  symb.draw(&p, r);
  this->addItem(icon, tr("Vertical Line"));

  symb.setStyle(QwtSymbol::Star1);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Star 1"));

  symb.setStyle(QwtSymbol::Star2);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Star 2"));

  symb.setStyle(QwtSymbol::Hexagon);
  icon.fill(c);
  symb.draw(&p, r);
  this->addItem(icon, tr("Hexagon"));

  p.end();
}

void SymbolBox::setStyle(const QwtSymbol::Style &style) {
  // Avoid compiler warnings relating to symbols + sizeof(symbols) being out of
  // range
  size_t n = numberOfSymbols();
  for (size_t i = 0; i < n; ++i) {
    if (symbols[i] == style) {
      setCurrentIndex(int(i));
      return;
    }
  }
  setCurrentIndex(0);
  //  const QwtSymbol::Style*ite = std::find(symbols, symbols + sizeof(symbols),
  //  style);
  //  if (ite == symbols + sizeof(symbols))
  //    this->setCurrentIndex(0);
  //  else
  //    this->setCurrentIndex(int(ite - symbols));
}

QwtSymbol::Style SymbolBox::selectedSymbol() const {
  size_t i = this->currentIndex();
  if (i < sizeof(symbols))
    return symbols[this->currentIndex()];

  return QwtSymbol::NoSymbol;
}

int SymbolBox::symbolIndex(const QwtSymbol::Style &style) {
  // Avoid compiler warnings relating to symbols + sizeof(symbols) being out of
  // range
  size_t n = numberOfSymbols();
  for (size_t i = 0; i < n; ++i) {
    if (symbols[i] == style) {
      return int(i);
    }
  }
  return 0;
  //  const QwtSymbol::Style*ite = std::find(symbols, symbols + sizeof(symbols),
  //  style);
  //  if (ite == symbols + sizeof(symbols))
  //    return 0;
  //
  //  return (int(ite - symbols));
}

QwtSymbol::Style SymbolBox::style(int index) {
  if (index >= 0 && index < (int)numberOfSymbols())
    return symbols[index];

  return QwtSymbol::NoSymbol;
}

QList<int> SymbolBox::defaultSymbols() {
  QList<int> lst;
  for (int i = 0; i < QwtSymbol::StyleCnt; i++)
    lst << i;

  return lst;
}

void SymbolBox::focusInEvent(QFocusEvent *e) {
  emit activated(this);
  return QComboBox::focusInEvent(e);
}

size_t SymbolBox::numberOfSymbols() {
  return sizeof(symbols) / sizeof(QwtSymbol::Style);
}
