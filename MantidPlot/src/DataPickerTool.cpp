/***************************************************************************
    File                 : DataPickerTool.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006,2007 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Plot tool for selecting points on curves.

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
#include "DataPickerTool.h"
#include "ApplicationWindow.h"
#include "FunctionCurve.h"
#include "Graph.h"
#include "Plot.h"
#include "PlotCurve.h"
#include "QwtErrorPlotCurve.h"
#include "MantidQtWidgets/Common/pixmaps.h"

#include <QApplication>
#include <QClipboard>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTextStream>

#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_symbol.h>

using namespace MantidQt::API;

DataPickerTool::DataPickerTool(Graph *graph, ApplicationWindow *app, Mode mode,
                               const QObject *status_target,
                               const char *status_slot)
    : QwtPlotPicker(graph->plotWidget()->canvas()), PlotToolInterface(graph),
      d_app(app), d_mode(mode), d_move_mode(Free) {
  d_selected_curve = nullptr;

  d_selection_marker.setLineStyle(QwtPlotMarker::Cross);
  d_selection_marker.setLinePen(QPen(Qt::red, 1));

  setTrackerMode(QwtPicker::AlwaysOn);
  if (d_mode == Move) {
    setSelectionFlags(QwtPicker::PointSelection | QwtPicker::DragSelection);
    d_graph->plotWidget()->canvas()->setCursor(Qt::PointingHandCursor);
  } else {
    setSelectionFlags(QwtPicker::PointSelection | QwtPicker::ClickSelection);
    d_graph->plotWidget()->canvas()->setCursor(
        QCursor(getQPixmap("vizor_xpm"), -1, -1));
  }

  if (status_target)
    connect(this, SIGNAL(statusText(const QString &)), status_target,
            status_slot);
  switch (d_mode) {
  case Display:
    emit statusText(tr("Click on plot or move cursor to display coordinates!"));
    break;
  case Move:
    emit statusText(tr("Please, click on plot and move cursor!"));
    break;
  case Remove:
    emit statusText(tr("Select point and double click to remove it!"));
    break;
  }

  if (status_target)
    connect(this, SIGNAL(statusText(const QString &)), status_target,
            status_slot);
  switch (d_mode) {
  case Display:
    emit statusText(tr("Click on plot or move cursor to display coordinates!"));
    break;
  case Move:
    emit statusText(tr("Please, click on plot and move cursor!"));
    break;
  case Remove:
    emit statusText(tr("Select point and double click to remove it!"));
    break;
  }
}

DataPickerTool::~DataPickerTool() { d_selection_marker.detach(); }

void DataPickerTool::append(const QPoint &pos) {
  int dist, point_index;
  const int curve =
      d_graph->plotWidget()->closestCurve(pos.x(), pos.y(), dist, point_index);
  if (curve <= 0 || dist >= 5) { // 5 pixels tolerance
    setSelection(nullptr, 0);
    return;
  }
  setSelection(
      dynamic_cast<QwtPlotCurve *>(d_graph->plotWidget()->curve(curve)),
      point_index);
  if (!d_selected_curve)
    return;

  QwtPlotPicker::append(
      transform(QwtDoublePoint(d_selected_curve->x(d_selected_point),
                               d_selected_curve->y(d_selected_point))));
}

void DataPickerTool::setSelection(QwtPlotCurve *curve, int point_index) {
  if (curve == d_selected_curve && point_index == d_selected_point)
    return;

  d_selected_curve = curve;
  d_selected_point = point_index;

  if (!d_selected_curve) {
    d_selection_marker.detach();
    d_graph->plotWidget()->replot();
    return;
  }

  setAxis(d_selected_curve->xAxis(), d_selected_curve->yAxis());
  auto plotCurve = dynamic_cast<PlotCurve *>(d_selected_curve);
  auto dataCurve = dynamic_cast<DataCurve *>(d_selected_curve);

  d_restricted_move_pos =
      QPoint(plot()->transform(xAxis(), d_selected_curve->x(d_selected_point)),
             plot()->transform(yAxis(), d_selected_curve->y(d_selected_point)));

  if (plotCurve && plotCurve->type() == GraphOptions::Function) {
    QLocale locale = d_app->locale();
    emit statusText(
        QString("%1[%2]: x=%3; y=%4")
            .arg(d_selected_curve->title().text())
            .arg(d_selected_point + 1)
            .arg(locale.toString(d_selected_curve->x(d_selected_point), 'G',
                                 d_app->d_decimal_digits))
            .arg(locale.toString(d_selected_curve->y(d_selected_point), 'G',
                                 d_app->d_decimal_digits)));
  } else if (dataCurve) {
    int row = dataCurve->tableRow(d_selected_point);

    Table *t = dataCurve->table();
    int xCol = t->colIndex(dataCurve->xColumnName());
    int yCol = t->colIndex(d_selected_curve->title().text());

    emit statusText(QString("%1[%2]: x=%3; y=%4")
                        .arg(d_selected_curve->title().text())
                        .arg(row + 1)
                        .arg(t->text(row, xCol))
                        .arg(t->text(row, yCol)));
  }

  QwtDoublePoint selected_point_value(d_selected_curve->x(d_selected_point),
                                      d_selected_curve->y(d_selected_point));
  d_selection_marker.setValue(selected_point_value);
  if (d_selection_marker.plot() == nullptr)
    d_selection_marker.attach(d_graph->plotWidget());
  d_graph->plotWidget()->replot();
}

bool DataPickerTool::eventFilter(QObject *obj, QEvent *event) {
  if (!d_selected_curve) {
    return QwtPlotPicker::eventFilter(obj, event);
  }
  switch (event->type()) {
  case QEvent::MouseButtonDblClick:
    if (d_selected_curve)
      emit selected(d_selected_curve, d_selected_point);
    event->accept();
    return true;

  case QEvent::MouseMove:
    if (auto mouseEvent = dynamic_cast<QMouseEvent *>(event)) {
      if (mouseEvent->modifiers() == Qt::ControlModifier)
        d_move_mode = Vertical;
      else if (mouseEvent->modifiers() == Qt::AltModifier)
        d_move_mode = Horizontal;
      else
        d_move_mode = Free;
      break;
    }

  default:
    break;
  }
  return QwtPlotPicker::eventFilter(obj, event);
}
