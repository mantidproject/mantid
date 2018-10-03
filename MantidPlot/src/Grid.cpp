// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : Grid.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "Grid.h"
#include "ColorBox.h"
#include "Graph.h"
#include "Plot.h"

#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include <qwt_painter.h>
#include <qwt_plot_canvas.h>

#include <QPainter>

Grid::Grid()
    : QObject(), QwtPlotGrid(), d_maj_pen_y(QPen(Qt::blue, 0.5, Qt::SolidLine)),
      d_min_pen_y(QPen(Qt::gray, 0.4, Qt::DotLine)), mrkX(-1), mrkY(-1) {
  setMajPen(QPen(Qt::blue, 0.5, Qt::SolidLine));
  setMinPen(QPen(Qt::gray, 0.4, Qt::DotLine));
  enableX(false);
  enableY(false);
  setRenderHint(QwtPlotItem::RenderAntialiased, false);
}

/**
  \brief Draw the grid

  The grid is drawn into the bounding rectangle such that
  gridlines begin and end at the rectangle's borders. The X and Y
  maps are used to map the scale divisions into the drawing region
  screen.
  @param painter ::  Painter
  @param mx :: X axis map
  @param my :: Y axis
  @param r :: Contents rect of the plot canvas
  */
void Grid::draw(QPainter *painter, const QwtScaleMap &mx, const QwtScaleMap &my,
                const QRect &r) const {
  //  draw minor X gridlines
  painter->setPen(minPen());

  if (xMinEnabled()) {
    drawLines(painter, r, Qt::Vertical, mx,
              xScaleDiv().ticks(QwtScaleDiv::MinorTick));
    drawLines(painter, r, Qt::Vertical, mx,
              xScaleDiv().ticks(QwtScaleDiv::MediumTick));
  }

  //  draw minor Y gridlines
  painter->setPen(d_min_pen_y);

  if (yMinEnabled()) {
    drawLines(painter, r, Qt::Horizontal, my,
              yScaleDiv().ticks(QwtScaleDiv::MinorTick));
    drawLines(painter, r, Qt::Horizontal, my,
              yScaleDiv().ticks(QwtScaleDiv::MediumTick));
  }

  //  draw major X gridlines
  painter->setPen(majPen());

  if (xEnabled()) {
    drawLines(painter, r, Qt::Vertical, mx,
              xScaleDiv().ticks(QwtScaleDiv::MajorTick));
  }

  //  draw major Y gridlines
  painter->setPen(d_maj_pen_y);

  if (yEnabled()) {
    drawLines(painter, r, Qt::Horizontal, my,
              yScaleDiv().ticks(QwtScaleDiv::MajorTick));
  }
}

void Grid::drawLines(QPainter *painter, const QRect &rect,
                     Qt::Orientation orientation, const QwtScaleMap &map,
                     const QwtValueList &values) const {
  const int x1 = rect.left();
  const int x2 = rect.right();
  const int y1 = rect.top();
  const int y2 = rect.bottom();

  for (uint i = 0; i < (uint)values.count(); i++) {
    const int value = map.transform(values[i]);
    if (orientation == Qt::Horizontal) {
      if ((value > y1) && (value < y2))
        QwtPainter::drawLine(painter, x1, value, x2, value);
    } else {
      if ((value > x1) && (value < x2))
        QwtPainter::drawLine(painter, value, y1, value, y2);
    }
  }
}

void Grid::load(const QStringList &grid) {
  Plot *d_plot = dynamic_cast<Plot *>(plot());
  if (!d_plot)
    return;

  bool majorOnX = grid[1].toInt();
  bool minorOnX = grid[2].toInt();
  bool majorOnY = grid[3].toInt();
  bool minorOnY = grid[4].toInt();
  bool xZeroOn = false;
  bool yZeroOn = false;
  int xAxis = QwtPlot::xBottom;
  int yAxis = QwtPlot::yLeft;

  QPen majPenX, minPenX, majPenY, minPenY;
  if (grid.count() >= 21) { // since 0.9 final
    majPenX = QPen(QColor(grid[5]), grid[7].toDouble(),
                   Graph::getPenStyle(grid[6].toInt()));
    minPenX = QPen(QColor(grid[8]), grid[10].toDouble(),
                   Graph::getPenStyle(grid[9].toInt()));
    majPenY = QPen(QColor(grid[11]), grid[13].toDouble(),
                   Graph::getPenStyle(grid[12].toInt()));
    minPenY = QPen(QColor(grid[14]), grid[16].toDouble(),
                   Graph::getPenStyle(grid[15].toInt()));

    xZeroOn = grid[17].toInt();
    yZeroOn = grid[18].toInt();
    xAxis = grid[19].toInt();
    yAxis = grid[20].toInt();
    if (grid.count() >= 22)
      setRenderHint(QwtPlotItem::RenderAntialiased, grid[21].toInt());

  } else { // older versions of QtiPlot (<= 0.9rc3)
    majPenX = QPen(ColorBox::color(grid[5].toInt()), grid[7].toDouble(),
                   Graph::getPenStyle(grid[6].toInt()));
    minPenX = QPen(ColorBox::color(grid[8].toInt()), grid[10].toDouble(),
                   Graph::getPenStyle(grid[9].toInt()));
    majPenY = majPenX;
    minPenY = minPenX;

    xZeroOn = grid[11].toInt();
    yZeroOn = grid[12].toInt();

    if (grid.count() == 15) {
      xAxis = grid[13].toInt();
      yAxis = grid[14].toInt();
    }
  }

  setMajPenX(majPenX);
  setMinPenX(minPenX);
  setMajPenY(majPenY);
  setMinPenY(minPenY);

  enableX(majorOnX);
  enableXMin(minorOnX);
  enableY(majorOnY);
  enableYMin(minorOnY);

  setAxis(xAxis, yAxis);

  enableZeroLineX(xZeroOn);
  enableZeroLineY(yZeroOn);
}

void Grid::enableZeroLineX(bool enable) {
  Plot *d_plot = dynamic_cast<Plot *>(plot());
  if (!d_plot)
    return;

  if (mrkX < 0 && enable) {
    QwtPlotMarker *m = new QwtPlotMarker();
    mrkX = d_plot->insertMarker(m);
    m->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    m->setAxis(xAxis(), yAxis());
    m->setLineStyle(QwtPlotMarker::VLine);
    m->setValue(0.0, 0.0);

    double width = 1;
    if (d_plot->canvas()->lineWidth())
      width = d_plot->canvas()->lineWidth();
    else if (d_plot->axisEnabled(QwtPlot::yLeft) ||
             d_plot->axisEnabled(QwtPlot::yRight))
      width = d_plot->axesLinewidth();

    m->setLinePen(QPen(Qt::black, width, Qt::SolidLine));
  } else if (mrkX >= 0 && !enable) {
    d_plot->removeMarker(mrkX);
    mrkX = -1;
  }
}

void Grid::enableZeroLineY(bool enable) {
  Plot *d_plot = dynamic_cast<Plot *>(plot());
  if (!d_plot)
    return;

  if (mrkY < 0 && enable) {
    QwtPlotMarker *m = new QwtPlotMarker();
    mrkY = d_plot->insertMarker(m);
    m->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    m->setAxis(xAxis(), yAxis());
    m->setLineStyle(QwtPlotMarker::HLine);
    m->setValue(0.0, 0.0);

    double width = 1;
    if (d_plot->canvas()->lineWidth())
      width = d_plot->canvas()->lineWidth();
    else if (d_plot->axisEnabled(QwtPlot::xBottom) ||
             d_plot->axisEnabled(QwtPlot::xTop))
      width = d_plot->axesLinewidth();

    m->setLinePen(QPen(Qt::black, width, Qt::SolidLine));
  } else if (mrkY >= 0 && !enable) {
    d_plot->removeMarker(mrkY);
    mrkY = -1;
  }
}

void Grid::copy(Grid *grid) {
  if (!grid)
    return;

  setMajPenX(grid->majPenX());
  setMinPenX(grid->minPenX());
  setMajPenY(grid->majPenY());
  setMinPenY(grid->minPenY());

  enableX(grid->xEnabled());
  enableXMin(grid->xMinEnabled());
  enableY(grid->yEnabled());
  enableYMin(grid->yMinEnabled());

  setAxis(grid->xAxis(), grid->yAxis());

  enableZeroLineX(grid->xZeroLineEnabled());
  enableZeroLineY(grid->yZeroLineEnabled());
  setRenderHint(QwtPlotItem::RenderAntialiased,
                grid->testRenderHint(QwtPlotItem::RenderAntialiased));
}

std::string Grid::saveToString() {
  MantidQt::API::TSVSerialiser tsv;
  tsv.writeLine("grid");

  tsv << xEnabled() << xMinEnabled();
  tsv << yEnabled() << yMinEnabled();

  tsv << majPenX().color().name();
  tsv << majPenX().style() - 1;
  tsv << majPenX().widthF();

  tsv << minPenX().color().name();
  tsv << minPenX().style() - 1;
  tsv << minPenX().widthF();

  tsv << majPenY().color().name();
  tsv << majPenY().style() - 1;
  tsv << majPenY().widthF();

  tsv << minPenY().color().name();
  tsv << minPenY().style() - 1;
  tsv << minPenY().widthF();

  tsv << xZeroLineEnabled() << yZeroLineEnabled();
  tsv << xAxis() << yAxis();
  tsv << testRenderHint(QwtPlotItem::RenderAntialiased);
  return tsv.outputLines();
}
