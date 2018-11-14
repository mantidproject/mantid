// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : Grid.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef GRID_H
#define GRID_H

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>

class Grid : public QObject,
             public QwtPlotGrid // Made a QObject just for our python proxies
{
  Q_OBJECT
public:
  Grid();

  bool xZeroLineEnabled() { return (mrkX >= 0) ? true : false; };
  void enableZeroLineX(bool enable = true);
  bool yZeroLineEnabled() { return (mrkY >= 0) ? true : false; };
  void enableZeroLineY(bool enable = true);

  void setMajPenX(const QPen &p) { setMajPen(p); };
  const QPen &majPenX() const { return majPen(); };

  void setMinPenX(const QPen &p) { setMinPen(p); };
  const QPen &minPenX() const { return minPen(); };

  void setMajPenY(const QPen &p) {
    if (d_maj_pen_y != p)
      d_maj_pen_y = p;
  };
  const QPen &majPenY() const { return d_maj_pen_y; };

  void setMinPenY(const QPen &p) {
    if (d_min_pen_y != p)
      d_min_pen_y = p;
  };
  const QPen &minPenY() const { return d_min_pen_y; };

  void load(const QStringList &);
  void copy(Grid *);
  std::string saveToString();

private:
  void draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QRect &rect) const override;
  void drawLines(QPainter *painter, const QRect &rect,
                 Qt::Orientation orientation, const QwtScaleMap &map,
                 const QwtValueList &values) const;

  QPen d_maj_pen_y;
  QPen d_min_pen_y;

  int mrkX, mrkY; // x=0 et y=0 line markers keys
};

#endif
