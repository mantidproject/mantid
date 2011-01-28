/***************************************************************************
    File                 : Plot.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Plot window class

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
#include "qwt_compat.h"
#include "Plot.h"
#include "Graph.h"
#include "Grid.h"
#include "ScaleDraw.h"
#include "Spectrogram.h"
#include "PlotCurve.h"
#include "LegendWidget.h"
#include "plot2D/ScaleEngine.h"

#include <qwt_plot.h>
#include <qwt_painter.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_map.h>
#include <qwt_text_label.h>

#include <QPainter>
#include <QMessageBox>

Plot::Plot(int width, int height, QWidget *parent, const char *)
: QwtPlot(parent)
{
	setAutoReplot (false);

	marker_key = 0;
	curve_key = 0;

	minTickLength = 5;
	majTickLength = 9;

	setGeometry(QRect(0, 0, width, height));
	setAxisTitle(QwtPlot::yLeft, tr("Y Axis Title"));
	setAxisTitle(QwtPlot::xBottom, tr("X Axis Title"));
	//due to the plot layout updates, we must always have a non empty title
	setAxisTitle(QwtPlot::yRight, tr(" "));
	setAxisTitle(QwtPlot::xTop, tr(" "));

	// grid
	d_grid = new Grid();
	d_grid->attach(this);

	//custom scale
	for (int i= 0; i<QwtPlot::axisCnt; i++) {
		QwtScaleWidget *scale = (QwtScaleWidget *) axisWidget(i);
		if (scale) {
			scale->setMargin(0);

			//the axis title color must be initialized...
			QwtText title = scale->title();
			title.setColor(Qt::black);
			scale->setTitle(title);

            //...same for axis color
            QPalette pal = scale->palette();
            pal.setColor(QPalette::Foreground, QColor(Qt::black));
            scale->setPalette(pal);

			ScaleDraw *sd = new ScaleDraw(this);
			sd->setTickLength(QwtScaleDiv::MinorTick, minTickLength);
			sd->setTickLength(QwtScaleDiv::MediumTick, minTickLength);
			sd->setTickLength(QwtScaleDiv::MajorTick, majTickLength);

			setAxisScaleDraw (i, sd);
			setAxisScaleEngine (i, new ScaleEngine());
		}
	}

	QwtPlotLayout *pLayout = plotLayout();
	pLayout->setCanvasMargin(0);
	pLayout->setAlignCanvasToScales (true);

	QwtPlotCanvas* plCanvas = canvas();
	plCanvas->setFocusPolicy(Qt::StrongFocus);
	plCanvas->setFocusIndicator(QwtPlotCanvas::ItemFocusIndicator);
	plCanvas->setFocus();
	plCanvas->setFrameShadow(QwtPlot::Plain);
	plCanvas->setCursor(Qt::arrowCursor);
	plCanvas->setLineWidth(0);
	plCanvas->setPaintAttribute(QwtPlotCanvas::PaintCached, false);
	plCanvas->setPaintAttribute(QwtPlotCanvas::PaintPacked, false);

    QColor background = QColor(Qt::white);
    background.setAlpha(255);

	QPalette palette;
    palette.setColor(QPalette::Window, background);
    setPalette(palette);

	setCanvasBackground (background);
	setFocusPolicy(Qt::StrongFocus);
	setFocusProxy(plCanvas);
	setFrameShape(QFrame::Box);
	setLineWidth(0);
}

QColor Plot::frameColor()
{
	return palette().color(QPalette::Active, QPalette::Foreground);
}

void Plot::printFrame(QPainter *painter, const QRect &rect) const
{
	painter->save();

	int lw = lineWidth();
	if (lw){
		QColor color = palette().color(QPalette::Active, QPalette::Foreground);
		painter->setPen (QPen(color, lw, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
	} else
		painter->setPen(QPen(Qt::NoPen));

    painter->setBrush(paletteBackgroundColor());
    QwtPainter::drawRect(painter, rect);
	painter->restore();
}

void Plot::printCanvas(QPainter *painter, const QRect &canvasRect,
   			 const QwtScaleMap map[axisCnt], const QwtPlotPrintFilter &pfilter) const
{
	painter->save();

	const QwtPlotCanvas* plotCanvas = canvas();	
	//commented it was clipping the top part of the graph
	//QRect rect = canvasRect.adjusted(1, 1, -2, -2);
	
	QRect rect = canvasRect.adjusted(1, -1, -2, -1);
	

    QwtPainter::fillRect(painter, rect, canvasBackground());
	painter->setClipping(true);
	QwtPainter::setClipRect(painter, rect);

    drawItems(painter, canvasRect, map, pfilter);
    painter->restore();

    painter->save();
	int lw = plotCanvas->lineWidth();
	if(lw > 0){
		QColor color = plotCanvas->palette().color(QPalette::Active, QColorGroup::Foreground);
		painter->setPen (QPen(color, lw, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
		QwtPainter::drawRect(painter, canvasRect.adjusted(0, 0, -1, -1));
	}
    painter->restore();

	// print texts
	QObjectList lst = children();
	foreach(QObject *o, lst){
		if (o->inherits("LegendWidget") && !((QWidget *)o)->isHidden())
        	((LegendWidget *)o)->print(painter, map);
	}
}

void Plot::drawItems (QPainter *painter, const QRect &rect,
			const QwtScaleMap map[axisCnt], const QwtPlotPrintFilter &pfilter) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    for (int i=0; i<QwtPlot::axisCnt; i++){
		if (!axisEnabled(i))
			continue;
        drawBreak(painter, rect, map[i], i);
    }
    painter->restore();

    for (int i=0; i<QwtPlot::axisCnt; i++){
		if (!axisEnabled(i))
			continue;

		ScaleEngine *sc_engine = (ScaleEngine *)axisScaleEngine(i);
		/*const QwtScaleEngine *qwtsc_engine=axisScaleEngine(i);
		const ScaleEngine *sc_engine =dynamic_cast<const ScaleEngine*>(qwtsc_engine);
		if(sc_engine!=NULL)
		{	*/
		if (!sc_engine->hasBreak())
			continue;
	
		QwtScaleMap m = map[i];
		int lb = m.transform(sc_engine->axisBreakLeft());
		int rb = m.transform(sc_engine->axisBreakRight());
		int start = lb, end = rb;
		if (sc_engine->testAttribute(QwtScaleEngine::Inverted)){
			end = lb;
			start = rb;
		}
		QRegion cr(rect);
		if (i == QwtPlot::xBottom || i == QwtPlot::xTop)
			painter->setClipRegion(cr.subtracted(QRegion(start, rect.y(), abs(end - start), rect.height())), Qt::IntersectClip);
		else if (i == QwtPlot::yLeft || i == QwtPlot::yRight)
			painter->setClipRegion(cr.subtracted(QRegion(rect.x(), end, rect.width(), abs(end - start))), Qt::IntersectClip);
		//}
	}

	QwtPlot::drawItems(painter, rect, map, pfilter);

	for (int i=0; i<QwtPlot::axisCnt; i++){
		if (!axisEnabled(i))
			continue;

		ScaleDraw *sd = (ScaleDraw *) axisScaleDraw (i);
		int majorTicksType = sd->majorTicksStyle();
		int minorTicksType = sd->minorTicksStyle();

		bool min = (minorTicksType == ScaleDraw::In || minorTicksType == ScaleDraw::Both);
		bool maj = (majorTicksType == ScaleDraw::In || majorTicksType == ScaleDraw::Both);

		if (min || maj)
			drawInwardTicks(painter, rect, map[i], i, min, maj);
	}
}

void Plot::drawInwardTicks(QPainter *painter, const QRect &rect,
		const QwtScaleMap &map, int axis, bool min, bool maj) const
{
	int x1=rect.left();
	int x2=rect.right();
	int y1=rect.top();
	int y2=rect.bottom();

	QPalette pal=axisWidget(axis)->palette();
	QColor color=pal.color(QPalette::Active, QColorGroup::Foreground);

	painter->save();
	painter->setPen(QPen(color, axesLinewidth(), Qt::SolidLine));

	QwtScaleDiv *scDiv=(QwtScaleDiv *)axisScaleDiv(axis);
	const QwtValueList minTickList = scDiv->ticks(QwtScaleDiv::MinorTick);
	int minTicks = (int)minTickList.count();

	const QwtValueList medTickList = scDiv->ticks(QwtScaleDiv::MediumTick);
	int medTicks = (int)medTickList.count();

	const QwtValueList majTickList = scDiv->ticks(QwtScaleDiv::MajorTick);
	int majTicks = (int)majTickList.count();

	int j, x, y, low,high;
	switch (axis)
	{
		case QwtPlot::yLeft:
			x=x1;
			low=y1+majTickLength;
			high=y2-majTickLength;
			if (min){
				for (j = 0; j < minTicks; j++){
					y = map.transform(minTickList[j]);
					if (y>low && y< high)
						QwtPainter::drawLine(painter, x, y, x+minTickLength, y);
				}
				for (j = 0; j < medTicks; j++){
					y = map.transform(medTickList[j]);
					if (y>low && y< high)
						QwtPainter::drawLine(painter, x, y, x+minTickLength, y);
				}
			}

			if (maj){
				for (j = 0; j < majTicks; j++){
					y = map.transform(majTickList[j]);
					if (y>low && y< high)
						QwtPainter::drawLine(painter, x, y, x+majTickLength, y);
				}
			}
			break;

		case QwtPlot::yRight:
			{
				x=x2;
				low=y1+majTickLength;
				high=y2-majTickLength;
				if (min){
					for (j = 0; j < minTicks; j++){
						y = map.transform(minTickList[j]);
						if (y>low && y< high)
							QwtPainter::drawLine(painter, x+1, y, x-minTickLength, y);
					}
					for (j = 0; j < medTicks; j++){
						y = map.transform(medTickList[j]);
						if (y>low && y< high)
							QwtPainter::drawLine(painter, x+1, y, x-minTickLength, y);
					}
				}

				if (maj){
					for (j = 0; j <majTicks; j++){
						y = map.transform(majTickList[j]);
						if (y>low && y< high)
							QwtPainter::drawLine(painter, x+1, y, x-majTickLength, y);
					}
				}
			}
			break;

		case QwtPlot::xBottom:
			y=y2;
			low=x1+majTickLength;
			high=x2-majTickLength;
			if (min){
				for (j = 0; j < minTicks; j++){
					x = map.transform(minTickList[j]);
					if (x>low && x<high)
						QwtPainter::drawLine(painter, x, y+1, x, y-minTickLength);
				}
				for (j = 0; j < medTicks; j++){
					x = map.transform(medTickList[j]);
					if (x>low && x<high)
						QwtPainter::drawLine(painter, x, y+1, x, y-minTickLength);
				}
			}

			if (maj){
				for (j = 0; j < majTicks; j++){
					x = map.transform(majTickList[j]);
					if (x>low && x<high)
						QwtPainter::drawLine(painter, x, y+1, x, y-majTickLength);
				}
			}
			break;

		case QwtPlot::xTop:
			y=y1;
			low=x1+majTickLength;
			high=x2-majTickLength;

			if (min){
				for (j = 0; j < minTicks; j++){
					x = map.transform(minTickList[j]);
					if (x>low && x<high)
						QwtPainter::drawLine(painter, x, y, x, y + minTickLength);
				}
				for (j = 0; j < medTicks; j++){
					x = map.transform(medTickList[j]);
					if (x>low && x<high)
						QwtPainter::drawLine(painter, x, y, x, y + minTickLength);
				}
			}

			if (maj){
				for (j = 0; j <majTicks; j++){
					x = map.transform(majTickList[j]);
					if (x>low && x<high)
						QwtPainter::drawLine(painter, x, y, x, y + majTickLength);
				}
			}
			break;
	}
	painter->restore();
}

void Plot::drawBreak(QPainter *painter, const QRect &rect, const QwtScaleMap &map, int axis) const
{	
    ScaleEngine *sc_engine = (ScaleEngine *)axisScaleEngine(axis);
	/*const QwtScaleEngine *qwtsc_engine=axisScaleEngine(axis);
	const ScaleEngine *sc_engine =dynamic_cast<const ScaleEngine*>(qwtsc_engine);
	if(sc_engine!=NULL)
	{*/	
		if (!sc_engine->hasBreak() || !sc_engine->hasBreakDecoration())
			return;

		painter->save();

		QColor color = axisWidget(axis)->palette().color(QPalette::Active, QColorGroup::Foreground);
		painter->setPen(QPen(color, axesLinewidth(), Qt::SolidLine));

		int left = map.transform(sc_engine->axisBreakLeft());
		int right = map.transform(sc_engine->axisBreakRight());
		int x, y;
		int len = majTickLength;
		switch (axis){
		case QwtPlot::yLeft:
			x = rect.left() - 1;
			QwtPainter::drawLine(painter, x, left, x + len, left - len);
			QwtPainter::drawLine(painter, x, right, x + len, right - len);
			break;

		case QwtPlot::yRight:
			x = rect.right() + 1;
			QwtPainter::drawLine(painter, x - len, left + len, x, left);
			QwtPainter::drawLine(painter, x - len, right + len, x, right);
			break;

		case QwtPlot::xBottom:
			y = rect.bottom() + 1;
			QwtPainter::drawLine(painter, left, y, left + len, y - len);
			QwtPainter::drawLine(painter, right, y, right + len, y - len);
			break;

		case QwtPlot::xTop:
			y = rect.top() - 1;
			QwtPainter::drawLine(painter, left - len, y + len, left, y);
			QwtPainter::drawLine(painter, right - len, y + len, right, y);
			break;
		}
		painter->restore();
	//}
}

void Plot::setAxesLinewidth(int width)
{
	for (int i=0; i<QwtPlot::axisCnt; i++){
		QwtScaleWidget *scale=(QwtScaleWidget*) this->axisWidget(i);
		if (scale) {
			scale->setPenWidth(width);
			scale->repaint();
		}
	}
}

int Plot::axesLinewidth() const
{
	for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ ) {
		const QwtScaleWidget *scale = this->axisWidget(axis);
		if (scale)
			return scale->penWidth();
	}
	return 0;
}

int Plot::minorTickLength() const
{
	return minTickLength;
}

int Plot::majorTickLength() const
{
	return majTickLength;
}

void Plot::setTickLength (int minLength, int majLength)
{
	if (majTickLength == majLength &&
			minTickLength == minLength)
		return;

	majTickLength = majLength;
	minTickLength = minLength;
}

QwtPlotCurve* Plot::curve(int index)
{
    QwtPlotItem *it = d_curves.value(index);
    if (it && it->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
        return (QwtPlotCurve*)it;
    else
        return 0;
}

int Plot::closestCurve(int xpos, int ypos, int &dist, int &point)
{
	QwtScaleMap map[QwtPlot::axisCnt];
	for ( int axis = 0; axis < QwtPlot::axisCnt; axis++ )
		map[axis] = canvasMap(axis);

	double dmin = 1.0e10;
	int key = -1;
	for (QMap<int, QwtPlotItem *>::iterator iter = d_curves.begin(); iter != d_curves.end(); ++iter )
	{
		QwtPlotItem *item = (QwtPlotItem *)iter.data();
		if (!item)
			continue;

		if(item->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
		{
      PlotCurve *c = (PlotCurve *)item;
      DataCurve *dc = dynamic_cast<DataCurve *>(item);
      if (!dc) continue;
      if (c->type() != Graph::Function && dc->hasLabels() &&
        dc->selectedLabels(QPoint(xpos, ypos))){
          dist = 0;
          return iter.key();
      } else
        dc->setLabelsSelected(false);

			for (int i=0; i<c->dataSize(); i++)
			{
				double cx = map[c->xAxis()].xTransform(c->x(i)) - double(xpos);
				double cy = map[c->yAxis()].xTransform(c->y(i)) - double(ypos);
				double f = qwtSqr(cx) + qwtSqr(cy);
				if (f < dmin && c->type() != Graph::ErrorBars)
				{
					dmin = f;
					key = iter.key();
					point = i;
				}
			}
		}
	}
	dist = int(sqrt(dmin));
	return key;
}

void Plot::removeMarker(int index)
{
	QwtPlotMarker *m = d_markers[index];
	if(!m)
		return;
	m->detach();
	d_markers.remove (index);
}

int Plot::insertMarker(QwtPlotMarker *m)
{
	marker_key++;
	if (!d_markers.contains(marker_key))
		d_markers.insert (marker_key, m);
	m->setRenderHint(QwtPlotItem::RenderAntialiased, ((Graph *)parent())->antialiasing());
	m->attach(((QwtPlot *)this));
	return marker_key;
}

int Plot::insertCurve(QwtPlotItem *c)
{
	curve_key++;
	if (!d_curves.contains(curve_key))
		d_curves.insert (curve_key, c);
	if (c->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
		((QwtPlotCurve *)c)->setPaintAttribute(QwtPlotCurve::PaintFiltered);

	c->setRenderHint(QwtPlotItem::RenderAntialiased, ((Graph *)parent())->antialiasing());
	c->attach(this);
	return curve_key;
}

void Plot::removeCurve(int index)
{
	QwtPlotItem *c = d_curves[index];
  	if (!c)
  		return;

  	if (c->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
  	{
  		Spectrogram *sp = (Spectrogram *)c;
  	    QwtScaleWidget *colorAxis = axisWidget(sp->colorScaleAxis());
  	    if (colorAxis)
  	    	colorAxis->setColorBarEnabled(false);
  	}

	c->detach();
	QwtPlotItem* p = d_curves.take (index);
  // RNT: Making curve_key unique prevents clashes elsewhere
	//--curve_key;
	delete p;	
}

QList<int> Plot::getMajorTicksType()
{
	QList<int> majorTicksType;
	for (int axis=0; axis<QwtPlot::axisCnt; axis++)
	{
		if (axisEnabled(axis))
		{
			ScaleDraw *sd = (ScaleDraw *) axisScaleDraw (axis);
			majorTicksType << sd->majorTicksStyle();
		}
		else
			majorTicksType << ScaleDraw::Out;
	}
	return majorTicksType;
}

void Plot::setMajorTicksType(int axis, int type)
{
	ScaleDraw *sd = (ScaleDraw *)axisScaleDraw (axis);
	if (sd)
		sd->setMajorTicksStyle ((ScaleDraw::TicksStyle)type);
}

QList<int> Plot::getMinorTicksType()
{
	QList<int> minorTicksType;
	for (int axis=0; axis<QwtPlot::axisCnt; axis++)
	{
		if (axisEnabled(axis))
		{
			ScaleDraw *sd = (ScaleDraw *) axisScaleDraw (axis);
			minorTicksType << sd->minorTicksStyle();
		}
		else
			minorTicksType << ScaleDraw::Out;
	}
	return minorTicksType;
}

void Plot::setMinorTicksType(int axis, int type)
{
	ScaleDraw *sd = (ScaleDraw *)axisScaleDraw (axis);
	if (sd)
		sd->setMinorTicksStyle((ScaleDraw::TicksStyle)type);
}

int Plot::axisLabelFormat(int axis)
{
	if (axisValid(axis)){
		ScaleDraw *sd = (ScaleDraw *)axisScaleDraw (axis);
		return sd->labelNumericFormat();
	}
	return 0;
}

int Plot::axisLabelPrecision(int axis)
{
	if (axisValid(axis))
	{
		ScaleDraw *sd = (ScaleDraw *)axisScaleDraw (axis);
		return sd->labelNumericPrecision();
	}

	//for a bad call we return the default values
	return 4;
}

/**
  @return the number format for the major scale labels of a specified axis
  @param axis :: axis index
  @retval f format character
  @retval prec precision
  */
void Plot::axisLabelFormat(int axis, char &f, int &prec) const
{
	if (axisValid(axis)){
		ScaleDraw *sd = (ScaleDraw *)axisScaleDraw (axis);
		sd->labelFormat(f, prec);
	} else {//for a bad call we return the default values
		f = 'g';
		prec = 4;
	}
}

/**
  \brief Adjust plot content to its current size.
  Must be reimplemented because the base implementation adds a mask causing an ugly drawing artefact.
*/
void Plot::updateLayout()
{
    plotLayout()->activate(this, contentsRect());

    // resize and show the visible widgets

    if (!titleLabel()->text().isEmpty()){
        titleLabel()->setGeometry(plotLayout()->titleRect());
        if (!titleLabel()->isVisible())
            titleLabel()->show();
    } else
		titleLabel()->hide();

    for (int axisId = 0; axisId < axisCnt; axisId++ ){
        if (axisEnabled(axisId) ){
            axisWidget(axisId)->setGeometry(plotLayout()->scaleRect(axisId));
            if (!axisWidget(axisId)->isVisible())
                axisWidget(axisId)->show();
        } else
            axisWidget(axisId)->hide();
    }

    canvas()->setGeometry(plotLayout()->canvasRect());
}


const QColor & Plot::paletteBackgroundColor() const
{
	return	palette().color(QPalette::Window);
}

void Plot::updateCurveLabels()
{
    QList<QwtPlotItem *> curves = curvesList();
    foreach(QwtPlotItem *i, curves){
      DataCurve * dc = dynamic_cast<DataCurve *>(i);
      if(dc && i->rtti() != QwtPlotItem::Rtti_PlotSpectrogram &&
        dc->type() != Graph::Function &&
        dc->hasLabels())
        dc->updateLabelsPosition();
    }
}

void Plot::showEvent (QShowEvent * event)
{
    event->accept();
    updateCurveLabels();
}

/**
  \brief Paint the plot into a given rectangle.
  Paint the contents of a QwtPlot instance into a given rectangle (Qwt modified code).

  @param painter :: Painter
  @param plotRect :: Bounding rectangle
  @param pfilter :: Print filter
*/
void Plot::print(QPainter *painter, const QRect &plotRect,
        const QwtPlotPrintFilter &pfilter)
{
    int axisId;

    if ( painter == 0 || !painter->isActive() ||
            !plotRect.isValid() || size().isNull() )
       return;

    QwtText t = title();
	printFrame(painter, plotRect);

    painter->save();

    // All paint operations need to be scaled according to
    // the paint device metrics.

    QwtPainter::setMetricsMap(this, painter->device());
    const QwtMetricsMap &metricsMap = QwtPainter::metricsMap();

    // It is almost impossible to integrate into the Qt layout
    // framework, when using different fonts for printing
    // and screen. To avoid writing different and Qt unconform
    // layout engines we change the widget attributes, print and
    // reset the widget attributes again. This way we produce a lot of
    // useless layout events ...

    pfilter.apply((QwtPlot *)this);

    int baseLineDists[QwtPlot::axisCnt];
    if ( !(pfilter.options() & 16) ){
        // In case of no background we set the backbone of
        // the scale on the frame of the canvas.

        for (axisId = 0; axisId < QwtPlot::axisCnt; axisId++ ){
            QwtScaleWidget *scaleWidget = (QwtScaleWidget *)axisWidget(axisId);
            if ( scaleWidget ){
                baseLineDists[axisId] = scaleWidget->margin();
                scaleWidget->setMargin(0);
            }
        }
    }
    // Calculate the layout for the print.

    int layoutOptions = QwtPlotLayout::IgnoreScrollbars
        | QwtPlotLayout::IgnoreFrames;
    if ( !(pfilter.options() & QwtPlotPrintFilter::PrintMargin) )
        layoutOptions |= QwtPlotLayout::IgnoreMargin;
    if ( !(pfilter.options() & QwtPlotPrintFilter::PrintLegend) )
        layoutOptions |= QwtPlotLayout::IgnoreLegend;

    ((QwtPlot *)this)->plotLayout()->activate(this,
        QwtPainter::metricsMap().deviceToLayout(plotRect),
        layoutOptions);

    if ((pfilter.options() & QwtPlotPrintFilter::PrintTitle)
        && (!titleLabel()->text().isEmpty())){
        printTitle(painter, plotLayout()->titleRect());
    }

	QRect canvasRect = plotLayout()->canvasRect();;
	canvasRect = metricsMap.layoutToDevice(canvasRect);

 
    // When using QwtPainter all sizes where computed in pixel
    // coordinates and scaled by QwtPainter later. This limits
    // the precision to screen resolution. A much better solution
    // is to scale the maps and print in unlimited resolution.

    QwtScaleMap map[axisCnt];
    for (axisId = 0; axisId < axisCnt; axisId++){
        map[axisId].setTransformation(axisScaleEngine(axisId)->transformation());

        const QwtScaleDiv &scaleDiv = *axisScaleDiv(axisId);
        map[axisId].setScaleInterval(scaleDiv.lBound(), scaleDiv.hBound());

        double from, to;
        if ( axisEnabled(axisId) ){
            const int sDist = axisWidget(axisId)->startBorderDist();
            const int eDist = axisWidget(axisId)->endBorderDist();
            const QRect &scaleRect = plotLayout()->scaleRect(axisId);

            if ( axisId == xTop || axisId == xBottom ){
                from = metricsMap.layoutToDeviceX(scaleRect.left() + sDist);
                to = metricsMap.layoutToDeviceX(scaleRect.right() + 1 - eDist);
            } else {
                from = metricsMap.layoutToDeviceY(scaleRect.bottom() + 1 - eDist );
                to = metricsMap.layoutToDeviceY(scaleRect.top() + sDist);
            }
        } else {
            const int margin = plotLayout()->canvasMargin(axisId);
            if ( axisId == yLeft || axisId == yRight ){
                from = metricsMap.layoutToDeviceX(canvasRect.bottom() - margin);
                to = metricsMap.layoutToDeviceX(canvasRect.top() + margin);
            } else {
                from = metricsMap.layoutToDeviceY(canvasRect.left() + margin);
                to = metricsMap.layoutToDeviceY(canvasRect.right() - margin);
            }
        }
        map[axisId].setPaintXInterval(from, to);
    }

   // The canvas maps are already scaled.
    QwtPainter::setMetricsMap(painter->device(), painter->device());
    printCanvas(painter, canvasRect, map, pfilter);
    QwtPainter::resetMetricsMap();


  
   canvasRect = plotLayout()->canvasRect();


    for ( axisId = 0; axisId < QwtPlot::axisCnt; axisId++ ){
        QwtScaleWidget *scaleWidget = (QwtScaleWidget *)axisWidget(axisId);
        if (scaleWidget){
            int baseDist = scaleWidget->margin();

            int startDist, endDist;
            scaleWidget->getBorderDistHint(startDist, endDist);

            QRect scaleRect = plotLayout()->scaleRect(axisId);
            if (!scaleWidget->margin()){
                switch(axisId){
                    case xBottom:
                        scaleRect.translate(0, canvasRect.bottom() - scaleRect.top());
                    break;
                    case xTop:
                        scaleRect.translate(0, canvasRect.top() - scaleRect.bottom());
                    break;
                    case yLeft:
                        scaleRect.translate(canvasRect.left() - scaleRect.right(), 0);
                    break;
                    case yRight:
                        scaleRect.translate(canvasRect.right() - scaleRect.left(), 0);
                    break;
                }
            }
            printScale(painter, axisId, startDist, endDist, baseDist, scaleRect);
        }
    }

    if ( !(pfilter.options() & 16) )
    {
        QRect boundingRect(
            canvasRect.left() - 1, canvasRect.top() - 1,
            canvasRect.width() + 2, canvasRect.height() + 2);
        boundingRect = metricsMap.layoutToDevice(boundingRect);
        boundingRect.setWidth(boundingRect.width() - 1);
        boundingRect.setHeight(boundingRect.height() - 1);

        painter->setPen(QPen(Qt::black));
        painter->setBrush(QBrush(Qt::NoBrush));
        painter->drawRect(boundingRect);
    }

    

    ((QwtPlot *)this)->plotLayout()->invalidate();

    // reset all widgets with their original attributes.
    if ( !(pfilter.options() & 16) ){
        // restore the previous base line dists
        for (axisId = 0; axisId < QwtPlot::axisCnt; axisId++ ){
            QwtScaleWidget *scaleWidget = (QwtScaleWidget *)axisWidget(axisId);
            if ( scaleWidget  )
                scaleWidget->setMargin(baseLineDists[axisId]);
        }
    }

    pfilter.reset((QwtPlot *)this);
    painter->restore();
    setTitle(t);//hack used to avoid bug in Qwt::printTitle(): the title attributes are overwritten
}
