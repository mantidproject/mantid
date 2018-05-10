/***************************************************************************
    File                 : CanvasPicker.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006,2007 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
                           knut.franke*gmx.de
    Description          : Canvas picker

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
#include "CanvasPicker.h"
#include "ApplicationWindow.h"
#include "ArrowMarker.h"
#include "ImageMarker.h"
#include "LegendWidget.h"
#include "PlotCurve.h"
#include "Spectrogram.h"

#include <QVector>
#include <qwt_plot_canvas.h>
#include <qwt_text_label.h>

CanvasPicker::CanvasPicker(Graph *graph) : QObject(graph) {
  pointSelected = false;
  d_editing_marker = nullptr;

  plotWidget = graph->plotWidget();

  QwtPlotCanvas *canvas = plotWidget->canvas();
  canvas->installEventFilter(this);
}

bool CanvasPicker::eventFilter(QObject *object, QEvent *e) {

  QVector<int> images = plot()->imageMarkerKeys();
  QVector<int> lines = plot()->lineMarkerKeys();

  if (object != (QObject *)plot()->plotWidget()->canvas())
    return false;

  switch (e->type()) {
  case QEvent::MouseButtonPress: {
    plot()->deselect();
    emit selectPlot();

    const QMouseEvent *me = (const QMouseEvent *)e;
    // bool allAxisDisabled = true;
    for (int i = 0; i < QwtPlot::axisCnt; i++) {
      if (plotWidget->axisEnabled(i)) {
        // allAxisDisabled = false;
        break;
      }
    }

    int dist, point;
    plotWidget->closestCurve(me->pos().x(), me->pos().y(), dist, point);

    if (me->button() == Qt::LeftButton && (plot()->drawLineActive())) {
      startLinePoint = me->pos();
      return true;
    }

    if (me->button() == Qt::LeftButton && plot()->drawTextActive()) {
      drawTextMarker(me->pos());
      return true;
    }

    if (!plot()->zoomOn() && selectMarker(me)) {
      if (me->button() == Qt::RightButton)
        emit showMarkerPopupMenu();
      return true;
    }

    return false;
  } break;

  case QEvent::MouseButtonDblClick: {
    if (d_editing_marker) {
      return d_editing_marker->eventFilter(plotWidget->canvas(), e);
    } else if (plot()->selectedMarkerKey() >= 0) {
      if (lines.contains(plot()->selectedMarkerKey())) {
        emit viewLineDialog();
        return true;
      } else if (images.contains(plot()->selectedMarkerKey())) {
        emit viewImageDialog();
        return true;
      }
    } else if (plot()->isPiePlot()) {
      emit showPlotDialog(plot()->curveKey(0));
      return true;
    } else {
      const QMouseEvent *me = (const QMouseEvent *)e;
      int dist, point;
      int curveKey =
          plotWidget->closestCurve(me->pos().x(), me->pos().y(), dist, point);
      if (dist < 10)
        emit showPlotDialog(curveKey);
      else
        emit showPlotDialog(-1);
      return true;
    }
  } break;

  case QEvent::MouseMove: {

    const QMouseEvent *me = (const QMouseEvent *)e;
    if (me->button() != Qt::LeftButton)
      return true;

    QPoint pos = me->pos();
    DataCurve *c = plot()->selectedCurveLabels();
    if (c) {

      c->moveLabels(pos);
      return true;
    }

    if (plot()->drawLineActive()) {
      drawLineMarker(pos, plot()->drawArrow());
      return true;
    }
    return false;
  } break;

  case QEvent::MouseButtonRelease: {
    const QMouseEvent *me = (const QMouseEvent *)e;
    Graph *g = plot();

    if (g->drawLineActive()) {
      ApplicationWindow *app = g->multiLayer()->applicationWindow();
      if (!app)
        return true;

      ArrowMarker mrk;
      mrk.attach(g->plotWidget());
      mrk.setStartPoint(startLinePoint);
      mrk.setEndPoint(QPoint(me->x(), me->y()));
      mrk.setColor(app->defaultArrowColor);
      mrk.setWidth(app->defaultArrowLineWidth);
      mrk.setStyle(app->defaultArrowLineStyle);
      mrk.setHeadLength(app->defaultArrowHeadLength);
      mrk.setHeadAngle(app->defaultArrowHeadAngle);
      mrk.fillArrowHead(app->defaultArrowHeadFill);
      mrk.drawEndArrow(g->drawArrow());
      mrk.drawStartArrow(false);

      g->addArrow(&mrk);
      g->drawLine(false);
      mrk.detach();
      plotWidget->replot();
      return true;
    }

    /*if(plot()->zoomOn())
    {
            //fix for colormap changing on releasing the mouse button after
    zooming
            Graph * gr=plot();
            if(!gr)
                    return NULL;
            Spectrogram * spectrogram=gr->getSpectrogram();
            if(spectrogram)
                    spectrogram->setupColorBarScaling();
    }*/
    return false;
  } break;

  case QEvent::KeyPress: {
    int key = ((const QKeyEvent *)e)->key();

    int selectedMarker = plot()->selectedMarkerKey();
    if (lines.contains(selectedMarker) &&
        (key == Qt::Key_Enter || key == Qt::Key_Return)) {
      emit viewLineDialog();
      return true;
    }
    if (images.contains(selectedMarker) &&
        (key == Qt::Key_Enter || key == Qt::Key_Return)) {
      emit viewImageDialog();
      return true;
    }
  } break;

  default:
    break;
  }
  return QObject::eventFilter(object, e);
}

void CanvasPicker::disableEditing() {
  if (d_editing_marker) {
    d_editing_marker->setEditable(false);
    d_editing_marker = nullptr;
  }
}

void CanvasPicker::drawTextMarker(const QPoint &point) {
  LegendWidget t(plotWidget);
  t.move(point);
  t.setText(tr("enter your text here"));

  ApplicationWindow *app = plot()->multiLayer()->applicationWindow();
  if (app) {
    t.setFrameStyle(app->legendFrameStyle);
    t.setFont(app->plotLegendFont);
    t.setTextColor(app->legendTextColor);
    t.setBackgroundColor(app->legendBackground);
  }

  LegendWidget *l = plot()->insertText(&t);
  l->setSelected();
  l->showTextDialog();

  plot()->drawText(FALSE);
  emit drawTextOff();
}

void CanvasPicker::drawLineMarker(const QPoint &point, bool endArrow) {
  plot()->plotWidget()->canvas()->repaint();
  ArrowMarker mrk;
  mrk.attach(plotWidget);

  int clw = plotWidget->canvas()->lineWidth();
  mrk.setStartPoint(QPoint(startLinePoint.x() + clw, startLinePoint.y() + clw));
  mrk.setEndPoint(QPoint(point.x() + clw, point.y() + clw));
  mrk.setWidth(1);
  mrk.setStyle(Qt::SolidLine);
  mrk.drawEndArrow(endArrow);
  mrk.drawStartArrow(false);

  if (plot()->drawLineActive())
    mrk.setColor(Qt::black);
  else
    mrk.setColor(Qt::red);

  plotWidget->replot();
  mrk.detach();
}

bool CanvasPicker::selectMarker(const QMouseEvent *e) {
  const QPoint point = e->pos();
  foreach (int i, plot()->imageMarkerKeys()) {
    ImageMarker *m = dynamic_cast<ImageMarker *>(plotWidget->marker(i));
    if (!m)
      return false;
    if (m->rect().contains(point)) {
      disableEditing();
      plot()->setSelectedMarker(i, e->modifiers() & Qt::ShiftModifier);
      return true;
    }
  }
  foreach (int i, plot()->lineMarkerKeys()) {
    ArrowMarker *mrkL = dynamic_cast<ArrowMarker *>(plotWidget->marker(i));
    if (!mrkL)
      return false;
    int d =
        qRound(mrkL->width() + floor(mrkL->headLength() *
                                         tan(M_PI * mrkL->headAngle() / 180.0) +
                                     0.5));
    double dist = mrkL->dist(point.x(), point.y());
    if (dist <= d) {
      disableEditing();
      if (e->modifiers() & Qt::ShiftModifier) {
        plot()->setSelectedMarker(i, true);
        return true;
      } else if (e->button() == Qt::RightButton) {
        mrkL->setEditable(false);
        plot()->setSelectedMarker(i, false);
        return true;
      }
      plot()->deselectMarker();
      mrkL->setEditable(true);
      d_editing_marker = mrkL;
      return true;
    }
  }
  return false;
}

Graph *CanvasPicker::plot() { return dynamic_cast<Graph *>(parent()); }
