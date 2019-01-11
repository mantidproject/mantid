// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2006 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : SymbolBox.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef SYMBOLBOX_H
#define SYMBOLBOX_H

#include <QComboBox>
#include <qwt_symbol.h>

//! A modified QComboBox allowing to choose a QwtSmbol style.
/**
 * This is a simple hack on top of the QComboBox class.
 \image html images/symbol_box.png
 */
class SymbolBox : public QComboBox {
  Q_OBJECT
public:
  //! Constructor.
  /**
   * \param parent parent widget (only affects placement of the widget)
   */
  SymbolBox(bool showNoSymbol = true, QWidget *parent = nullptr);

  void setStyle(const QwtSymbol::Style &c);
  QwtSymbol::Style selectedSymbol() const;

  static QwtSymbol::Style style(int index);
  static int symbolIndex(const QwtSymbol::Style &style);
  static QList<int> defaultSymbols();

signals:
  //! Signal emitted when the box gains focus
  void activated(SymbolBox *);

protected:
  void init(bool showNoSymbol);
  void focusInEvent(QFocusEvent *) override;

private:
  static size_t numberOfSymbols();

  static const QwtSymbol::Style symbols[];
};

#endif
