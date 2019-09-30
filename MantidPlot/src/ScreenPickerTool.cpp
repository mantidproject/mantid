/***************************************************************************
    File                 : ScreenPickerTool.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006,2007 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Tool for selecting arbitrary points on a plot.

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
#include "ScreenPickerTool.h"
#include "ApplicationWindow.h"
#include "Graph.h"
#include "MantidQtWidgets/Common/pixmaps.h"
#include "Plot.h"
#include "PlotCurve.h"
#include "SymbolBox.h"
#include "Table.h"
#include <QKeyEvent>
#include <qwt_symbol.h>

using namespace MantidQt::API;

ScreenPickerTool::ScreenPickerTool(Graph *graph, const QObject *status_target,
                                   const char *status_slot)
    : QwtPlotPicker(graph->plotWidget()->canvas()), PlotToolInterface(graph) {
  d_selection_marker.setLineStyle(QwtPlotMarker::Cross);
  d_selection_marker.setLinePen(QPen(Qt::red, 1));
  setTrackerMode(QwtPicker::AlwaysOn);
  setSelectionFlags(QwtPicker::PointSelection | QwtPicker::ClickSelection);
  d_graph->plotWidget()->canvas()->setCursor(
      QCursor(getQPixmap("cursor_xpm"), -1, -1));

  if (status_target)
    connect(this, SIGNAL(statusText(const QString &)), status_target,
            status_slot);
  emit statusText(tr("Click on plot or move cursor to display coordinates!"));
}

ScreenPickerTool::~ScreenPickerTool() {
  d_selection_marker.detach();
  d_graph->plotWidget()->canvas()->unsetCursor();
  d_graph->plotWidget()->replot();
}

void ScreenPickerTool::append(const QPoint &point) {
  QwtDoublePoint pos = invTransform(point);
  QString info;
  info.sprintf("x=%g; y=%g", pos.x(), pos.y());
  emit statusText(info);

  d_selection_marker.setValue(pos);
  if (d_selection_marker.plot() == nullptr)
    d_selection_marker.attach(d_graph->plotWidget());
  d_graph->plotWidget()->replot();
}

bool ScreenPickerTool::eventFilter(QObject *obj, QEvent *event) {
  switch (event->type()) {
  case QEvent::MouseButtonDblClick:
    emit selected(d_selection_marker.value());
    return true;
  case QEvent::KeyPress: {
    QKeyEvent *ke = (QKeyEvent *)event;
    switch (ke->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return: {
      QwtDoublePoint pos =
          invTransform(canvas()->mapFromGlobal(QCursor::pos()));
      d_selection_marker.setValue(pos);
      if (d_selection_marker.plot() == nullptr)
        d_selection_marker.attach(d_graph->plotWidget());
      d_graph->plotWidget()->replot();
      emit selected(d_selection_marker.value());
      QString info;
      emit statusText(info.sprintf("x=%g; y=%g", pos.x(), pos.y()));
      return true;
    }
    default:
      break;
    }
  }
  default:
    break;
  }
  return QwtPlotPicker::eventFilter(obj, event);
}
/**
 *  This overrides the base class trackerText() function so that we can
 *  adjust the text returned.
 *
 *  @param pos  The current mouse location.
 */
QwtText ScreenPickerTool::trackerText(const QPoint &pos) const {
  return QwtPlotPicker::trackerText(pos);
}

/**
 *  This overrides the base class trackerText() function so that we can
 *  adjust the text returned.
 *
 *  @param pos  The current mouse location.
 */
QwtText ScreenPickerTool::trackerText(const QwtDoublePoint &pos) const {
  QString info;
  info.sprintf("%g; %g", pos.x(), pos.y());
  return QwtText(info);
}

DrawPointTool::DrawPointTool(ApplicationWindow *app, Graph *graph,
                             const QObject *status_target,
                             const char *status_slot)
    : ScreenPickerTool(graph, status_target, status_slot), d_app(app) {
  d_curve = nullptr;
  d_table = nullptr;
}

void DrawPointTool::appendPoint(const QwtDoublePoint &pos) {
  if (!d_app)
    return;

  QString info;
  emit statusText(info.sprintf("x=%g; y=%g", pos.x(), pos.y()));

  if (!d_table) {
    d_table = d_app->newHiddenTable(d_app->generateUniqueName(tr("Draw")), "",
                                    30, 2, "");
    d_app->modifiedProject();
  }

  int rows = 0;
  if (d_curve)
    rows = d_curve->dataSize();

  if (d_table->numRows() <= rows)
    d_table->setNumRows(rows + 10);

  d_table->setCell(rows, 0, pos.x());
  d_table->setCell(rows, 1, pos.y());

  if (!d_curve) {
    d_curve = new DataCurve(d_table, d_table->colName(0), d_table->colName(1));
    d_curve->setAxis(QwtPlot::xBottom, QwtPlot::yLeft);
    d_curve->setPen(QPen(Qt::black, d_app->defaultCurveLineWidth));
    d_curve->setSymbol(
        QwtSymbol(QwtSymbol::Ellipse, QBrush(Qt::black),
                  QPen(Qt::black, d_app->defaultCurveLineWidth),
                  QSize(d_app->defaultSymbolSize, d_app->defaultSymbolSize)));
    d_graph->insertPlotItem(d_curve, GraphOptions::LineSymbols);
  }

  d_curve->setFullRange();
  d_graph->updatePlot();
}

bool DrawPointTool::eventFilter(QObject *obj, QEvent *event) {
  switch (event->type()) {
  case QEvent::MouseButtonDblClick:
    appendPoint(d_selection_marker.value());
    return true;
  case QEvent::KeyPress: {
    QKeyEvent *ke = (QKeyEvent *)event;
    switch (ke->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return: {
      QwtDoublePoint pos =
          invTransform(canvas()->mapFromGlobal(QCursor::pos()));
      d_selection_marker.setValue(pos);
      if (d_selection_marker.plot() == nullptr)
        d_selection_marker.attach(d_graph->plotWidget());
      d_graph->plotWidget()->replot();
      emit selected(d_selection_marker.value());

      appendPoint(pos);
      return true;
    }
    default:
      break;
    }
  }
  default:
    break;
  }
  return QwtPlotPicker::eventFilter(obj, event);
}
