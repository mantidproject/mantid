// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : TitlePicker.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include <QObject>

class QwtPlot;
class QwtTextLabel;

class TitlePicker : public QObject {
  Q_OBJECT

public:
  explicit TitlePicker(QwtPlot *plot);
  void setSelected(bool select = true);
  bool selected() { return d_selected; };

signals:
  void clicked();
  void doubleClicked();
  void removeTitle();
  void showTitleMenu();

private:
  bool eventFilter(QObject *, QEvent *) override;
  QwtTextLabel *title;
  bool d_selected;
};
