// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : PenStyleBox.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef PEN_STYLE_BOX_H
#define PEN_STYLE_BOX_H

#include <QComboBox>

//! A modified QComboBox allowing to choose a Qt::PenStyle.
/**
 * This is a simple hack on top of the QComboBox class.
 */
class PenStyleBox : public QComboBox {
  Q_OBJECT

public:
  //! Constructor.
  /**
   * \param parent parent widget (only affects placement of the widget)
   */
  PenStyleBox(QWidget *parent = nullptr);
  void setStyle(const Qt::PenStyle &style);
  Qt::PenStyle style() const;

  static int styleIndex(const Qt::PenStyle &style);
  static Qt::PenStyle penStyle(int index);

private:
  static size_t numberOfPatterns();
  static const Qt::PenStyle patterns[];
};

#endif
