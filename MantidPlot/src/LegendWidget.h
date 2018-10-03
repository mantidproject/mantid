// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : LegendWidget.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef LEGENDWIDGET_H
#define LEGENDWIDGET_H

#include <QWidget>
#include <qwt_plot.h>
#include <qwt_text.h>

#include "Graph.h"
#include "Plot.h"

class LegendWidget : public QWidget {
  Q_OBJECT

public:
  explicit LegendWidget(Plot *);
  ~LegendWidget() override;

  void clone(LegendWidget *t);
  QwtPlot *plot() { return d_plot; };

  //! The kinds of frame a LegendWidget can draw around the Text.
  enum FrameStyle { None = 0, Line = 1, Shadow = 2 };

  QString text() { return d_text->text(); };
  void setText(const QString &s);

  //! Sets the position of the top left corner in axis coordinates
  void setOriginCoord(double x, double y);
  //! Sets the position of the top left corner in paint coordinates
  void setOrigin(int x, int y) { move(x, y); };

  QColor textColor() { return d_text->color(); };
  void setTextColor(const QColor &c);

  QColor backgroundColor() { return d_text->backgroundBrush().color(); };
  void setBackgroundColor(const QColor &c);

  int frameStyle() { return d_frame; };
  void setFrameStyle(int style);

  QFont font() { return d_text->font(); };
  void setFont(const QFont &font);

  int angle() { return d_angle; };
  void setAngle(int ang) { d_angle = ang; };

  double xValue();
  double yValue();

  void setSelected(bool on = true);

  void showTextEditor();
  void showTextDialog() { emit showDialog(); };
  void showContextMenu() { emit showMenu(); };

  void print(QPainter *p, const QwtScaleMap map[QwtPlot::axisCnt]);
  void setFixedCoordinatesMode(bool on = true);

private:
  PlotCurve *getCurve(const QString &s, int &point);
  void drawFrame(QPainter *p, const QRect &rect);
  void drawVector(PlotCurve *c, QPainter *p, int x, int y, int l);
  void drawSymbol(PlotCurve *c, int point, QPainter *p, int x, int y, int l);
  void drawText(QPainter *, const QRect &, QwtArray<int>, int);

  QwtArray<int> itemsHeight(int y, int symbolLineLength, int &width,
                            int &height);
  int symbolsMaxWidth();
  QString parse(const QString &str);

  void paintEvent(QPaintEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void contextMenuEvent(QContextMenuEvent *) override { emit showMenu(); };

  //! Parent plot
  Plot *d_plot;

  //! Frame type
  int d_frame;

  //! Rotation angle: not implemented yet
  int d_angle;

  //! Pointer to the QwtText object
  QwtText *d_text;

  //! Distance between symbols and legend text
  int h_space;

  //! Distance between frame and content
  int left_margin, top_margin;

  //! Length of the symbol line
  int line_length;

  SelectionMoveResizer *d_selector;

  double d_x, d_y;
  bool d_fixed_coordinates;

signals:
  void showDialog();
  void showMenu();
  void enableEditor();
};

#endif
