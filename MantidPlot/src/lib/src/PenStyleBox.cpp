// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : PenStyleBox.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "PenStyleBox.h"

#include <algorithm>

const Qt::PenStyle PenStyleBox::patterns[] = {Qt::SolidLine, Qt::DashLine,
                                              Qt::DotLine, Qt::DashDotLine,
                                              Qt::DashDotDotLine};

PenStyleBox::PenStyleBox(QWidget *parent) : QComboBox(parent) {
  setEditable(false);
  addItem("_____");
  addItem("_ _ _");
  addItem(".....");
  addItem("_._._");
  addItem("_.._..");
}

void PenStyleBox::setStyle(const Qt::PenStyle &style) {
  // Avoid compiler warnings relating to patterns + sizeof(patterns) being out
  // of range
  size_t n = numberOfPatterns();
  for (size_t i = 0; i < n; ++i) {
    if (patterns[i] == style) {
      setCurrentIndex(int(i));
      return;
    }
  }
  setCurrentIndex(0); // default style is solid.
  //  const Qt::PenStyle*ite = std::find(patterns, patterns + sizeof(patterns),
  //  style);
  //  if (ite == patterns + sizeof(patterns))
  //    this->setCurrentIndex(0); // default style is solid.
  //  else
  //    this->setCurrentIndex(int(ite - patterns));
}

Qt::PenStyle PenStyleBox::penStyle(int index) {
  if (index < (int)numberOfPatterns())
    return patterns[index];
  else
    return Qt::SolidLine; // default style is solid.
}

Qt::PenStyle PenStyleBox::style() const {
  size_t i = this->currentIndex();
  if (i < numberOfPatterns())
    return patterns[i];
  else
    return Qt::SolidLine; // default style is solid.
}

int PenStyleBox::styleIndex(const Qt::PenStyle &style) {
  // Avoid compiler warnings relating to patterns + sizeof(patterns) being out
  // of range
  size_t n = numberOfPatterns();
  for (size_t i = 0; i < n; ++i) {
    if (patterns[i] == style)
      return int(i);
  }
  return 0; // default style is solid.
  //  const Qt::PenStyle*ite = std::find(patterns, patterns + sizeof(patterns),
  //  style);
  //  if (ite == patterns + sizeof(patterns))
  //    return 0; // default style is solid.
  //  else
  //    return (int(ite - patterns));
}

size_t PenStyleBox::numberOfPatterns() {
  return sizeof(patterns) / sizeof(Qt::PenStyle);
}