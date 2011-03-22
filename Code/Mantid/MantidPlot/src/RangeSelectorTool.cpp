/***************************************************************************
    File                 : RangeSelectorTool.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006,2007 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Plot tool for selecting ranges on curves.

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
#include "RangeSelectorTool.h"
#include "Graph.h"
#include "Plot.h"
#include "PlotCurve.h"
#include "cursors.h"

#include <qwt_symbol.h>
#include <QPoint>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QEvent>
#include <QLocale>
#include <QTextStream>

RangeSelectorTool::RangeSelectorTool(Graph *graph, const QObject *status_target, const char *status_slot)
	: QwtPlotPicker(graph->plotWidget()->canvas()),
	PlotToolInterface(graph)
{
	d_selected_curve = NULL;
	for (int i=d_graph->curves(); i>=0; --i) {
		d_selected_curve = d_graph->curve(i);
		if (d_selected_curve && d_selected_curve->rtti() == QwtPlotItem::Rtti_PlotCurve
				&& d_selected_curve->dataSize() > 0)
			break;
		d_selected_curve = NULL;
	}
	if (!d_selected_curve) {
		QMessageBox::critical(d_graph, tr("MantidPlot - Warning"),
				tr("All the curves on this plot are empty!"));
		return;
	}

    d_enabled = true;
	d_visible = true;
	d_active_point = 0;
	d_inactive_point = d_selected_curve->dataSize() - 1;
	int marker_size = 20;

	d_active_marker.setSymbol(QwtSymbol(QwtSymbol::Cross, QBrush(QColor(255,255,255,0)),//QBrush(QColor(255,255,0,128)),
				QPen(Qt::red,2), QSize(marker_size,marker_size)));
	d_active_marker.setLineStyle(QwtPlotMarker::VLine);
	d_active_marker.setLinePen(QPen(Qt::red, 1, Qt::DashLine));
	d_inactive_marker.setSymbol(QwtSymbol(QwtSymbol::Cross, QBrush(QColor(255,255,255,0)), //QBrush(QColor(255,255,0,128)),
				QPen(Qt::black,2), QSize(marker_size,marker_size)));
	d_inactive_marker.setLineStyle(QwtPlotMarker::VLine);
	d_inactive_marker.setLinePen(QPen(Qt::black, 1, Qt::DashLine));
	d_active_marker.setValue(d_selected_curve->x(d_active_point),
			d_selected_curve->y(d_active_point));
	d_inactive_marker.setValue(d_selected_curve->x(d_inactive_point),
			d_selected_curve->y(d_inactive_point));
	d_active_marker.attach(d_graph->plotWidget());
	d_inactive_marker.attach(d_graph->plotWidget());

	setTrackerMode(QwtPicker::AlwaysOn);
	setSelectionFlags(QwtPicker::PointSelection | QwtPicker::ClickSelection);
	d_graph->plotWidget()->canvas()->setCursor(QCursor(getQPixmap("vizor_xpm"), -1, -1));
	d_graph->plotWidget()->canvas()->setFocus();
	d_graph->plotWidget()->replot();

	if (status_target)
		connect(this, SIGNAL(statusText(const QString&)), status_target, status_slot);
	emit statusText(tr("Click or use Ctrl+arrow key to select range (arrows select active cursor)!"));
}

RangeSelectorTool::~RangeSelectorTool()
{
	d_active_marker.detach();
	d_inactive_marker.detach();
	d_graph->plotWidget()->canvas()->unsetCursor();
	d_graph->plotWidget()->replot();
}

void RangeSelectorTool::pointSelected(const QPoint &pos)
{
	int dist, point;
	const int curve_key = d_graph->plotWidget()->closestCurve(pos.x(), pos.y(), dist, point);
	if (curve_key < 0 || dist >= 5) // 5 pixels tolerance
		return;
	QwtPlotCurve *curve = (QwtPlotCurve *)d_graph->plotWidget()->curve(curve_key);
	if (!curve)
		return;

	if (curve == d_selected_curve)
		setActivePoint(point);
	else {
        d_selected_curve = curve;

        d_active_point = point;
		d_active_marker.setValue(d_selected_curve->x(d_active_point), d_selected_curve->y(d_active_point));

        d_active_point > 0 ? d_inactive_point = 0 : d_inactive_point = d_selected_curve->dataSize() - 1;
		d_inactive_marker.setValue(curve->x(d_inactive_point), curve->y(d_inactive_point));
		emitStatusText();
		emit changed();
	}
	d_graph->plotWidget()->replot();
}

void RangeSelectorTool::setSelectedCurve(QwtPlotCurve *curve)
{
	if (!curve || d_selected_curve == curve || !d_enabled)
		return;
	d_selected_curve = curve;
	d_active_point = 0;
	d_inactive_point = d_selected_curve->dataSize() - 1;
	d_active_marker.setValue(d_selected_curve->x(d_active_point), d_selected_curve->y(d_active_point));
	d_inactive_marker.setValue(d_selected_curve->x(d_inactive_point), d_selected_curve->y(d_inactive_point));
	emitStatusText();
	emit changed();
}

void RangeSelectorTool::setActivePoint(int point)
{
	if (!d_enabled || point == d_active_point)
		return;
	d_active_point = point;
	d_active_marker.setValue(d_selected_curve->x(d_active_point), d_selected_curve->y(d_active_point));
	emitStatusText();
	emit changed();
}

void RangeSelectorTool::emitStatusText()
{
    QLocale locale = d_graph->plotWidget()->locale();
    if (((PlotCurve *)d_selected_curve)->type() == Graph::Function){
         emit statusText(QString("%1 <=> %2[%3]: x=%4; y=%5")
			.arg(d_active_marker.xValue() > d_inactive_marker.xValue() ? tr("Right") : tr("Left"))
			.arg(d_selected_curve->title().text())
			.arg(d_active_point + 1)
			.arg(locale.toString(d_selected_curve->x(d_active_point), 'G', 16))
			.arg(locale.toString(d_selected_curve->y(d_active_point), 'G', 16)));
    } else {
        Table *t = ((DataCurve*)d_selected_curve)->table();
        if (!t)
            return;

        int row = ((DataCurve*)d_selected_curve)->tableRow(d_active_point);

        emit statusText(QString("%1 <=> %2[%3]: x=%4; y=%5")
			.arg(d_active_marker.xValue() > d_inactive_marker.xValue() ? tr("Right") : tr("Left"))
			.arg(d_selected_curve->title().text())
			.arg(row + 1)
			.arg(t->text(row, t->colIndex(((DataCurve*)d_selected_curve)->xColumnName())))
			.arg(t->text(row, t->colIndex(d_selected_curve->title().text()))));
    }
}

void RangeSelectorTool::switchActiveMarker()
{
	QwtDoublePoint tmp = d_active_marker.value();
	d_active_marker.setValue(d_inactive_marker.value());
	d_inactive_marker.setValue(tmp);
	int tmp2 = d_active_point;
	d_active_point = d_inactive_point;
	d_inactive_point = tmp2;
	d_graph->plotWidget()->replot();

	emitStatusText();
}

bool RangeSelectorTool::eventFilter(QObject *obj, QEvent *event)
{
	switch(event->type()) {
		case QEvent::KeyPress:
			if (keyEventFilter((QKeyEvent*)event))
				return true;
			break;
		default:
			break;
	}
	return QwtPlotPicker::eventFilter(obj, event);
}

bool RangeSelectorTool::keyEventFilter(QKeyEvent *ke)
{
	switch(ke->key()) {
		case Qt::Key_Up:
			{
				int n_curves = d_graph->curves();
				int start = d_graph->curveIndex(d_selected_curve) + 1;
				for (int i = start; i < start + n_curves; ++i)
					if (d_graph->curve(i % n_curves)->dataSize() > 0) {
						setSelectedCurve(d_graph->curve(i % n_curves));
						break;
					}
				d_graph->plotWidget()->replot();
				return true;
			}
		case Qt::Key_Down:
			{
				int n_curves = d_graph->curves();
				int start = d_graph->curveIndex(d_selected_curve) + n_curves - 1;
				for (int i = start; i > start - n_curves; --i)
					if (d_graph->curve(i % n_curves)->dataSize() > 0) {
						setSelectedCurve(d_graph->curve(i % n_curves));
						break;
					}
				d_graph->plotWidget()->replot();
				return true;
			}
		case Qt::Key_Right:
		case Qt::Key_Plus:
			{
				if (ke->modifiers() & Qt::ControlModifier) {
					int n_points = d_selected_curve->dataSize();
					setActivePoint((d_active_point + 1) % n_points);
					d_graph->plotWidget()->replot();
				} else
					switchActiveMarker();
				return true;
			}
		case Qt::Key_Left:
		case Qt::Key_Minus:
			{
				if (ke->modifiers() & Qt::ControlModifier) {
					int n_points = d_selected_curve->dataSize();
					setActivePoint((d_active_point - 1 + n_points) % n_points);
					d_graph->plotWidget()->replot();
				} else
					switchActiveMarker();
				return true;
			}
		default:
			break;
	}
	return false;
}

void RangeSelectorTool::cutSelection()
{
    copySelection();
    clearSelection();
}

void RangeSelectorTool::copySelection()
{
    if (!d_selected_curve)
        return;

    int start_point = QMIN(d_active_point, d_inactive_point);
    int end_point = QMAX(d_active_point, d_inactive_point);
    QLocale locale = d_graph->plotWidget()->locale();
    QString text;
    for (int i = start_point; i <= end_point; i++){
        text += locale.toString(d_selected_curve->x(i), 'G', 16) + "\t";
        text += locale.toString(d_selected_curve->y(i), 'G', 16) + "\n";
    }

	QApplication::clipboard()->setText(text);
}

void RangeSelectorTool::clearSelection()
{
    if (!d_selected_curve)
        return;

    if (((PlotCurve *)d_selected_curve)->type() != Graph::Function){
        Table *t = ((DataCurve*)d_selected_curve)->table();
        if (!t)
            return;

		if (t->isReadOnlyColumn(t->colIndex(((DataCurve *)d_selected_curve)->xColumnName()))){
    		QMessageBox::warning(d_graph, tr("MantidPlot - Warning"),
        	tr("The column '%1' is read-only! Operation aborted!").arg(((DataCurve *)d_selected_curve)->xColumnName()));
		return;
		} else if (t->isReadOnlyColumn(t->colIndex(d_selected_curve->title().text()))){
    		QMessageBox::warning(d_graph, tr("MantidPlot - Warning"),
			tr("The column '%1' is read-only! Operation aborted!").arg(d_selected_curve->title().text()));
		return;
   		} 
		
        int start_point = QMIN(d_active_point, d_inactive_point);
        int start_row = ((DataCurve*)d_selected_curve)->tableRow(start_point);
        int end_point = QMAX(d_active_point, d_inactive_point);
        int end_row = ((DataCurve*)d_selected_curve)->tableRow(end_point);
        int col = t->colIndex(d_selected_curve->title().text());
        bool ok_update = (end_point - start_point + 1) < d_selected_curve->dataSize() ? true : false;
        for (int i = start_row; i <= end_row; i++)
            t->setText(i, col, "");
        t->notifyChanges();

        if (ok_update){
            d_active_point = 0;
            d_inactive_point = d_selected_curve->dataSize() - 1;
            d_active_marker.setValue(d_selected_curve->x(d_active_point), d_selected_curve->y(d_active_point));
            d_inactive_marker.setValue(d_selected_curve->x(d_inactive_point), d_selected_curve->y(d_inactive_point));
            emitStatusText();
            emit changed();
            d_graph->plotWidget()->replot();
        }
    }
}

// Paste text from the clipboard
void RangeSelectorTool::pasteSelection()
{
	QString text = QApplication::clipboard()->text();
	if (text.isEmpty())
		return;

    if (((PlotCurve *)d_selected_curve)->type() == Graph::Function)
        return;

    Table *t = ((DataCurve*)d_selected_curve)->table();
    if (!t)
        return;
	
	if (t->isReadOnlyColumn(t->colIndex(((DataCurve *)d_selected_curve)->xColumnName()))){
    	QMessageBox::warning(d_graph, tr("MantidPlot - Warning"),
        tr("The column '%1' is read-only! Operation aborted!").arg(((DataCurve *)d_selected_curve)->xColumnName()));
	return;
	} else if (t->isReadOnlyColumn(t->colIndex(d_selected_curve->title().text()))){
    	QMessageBox::warning(d_graph, tr("MantidPlot - Warning"),
		tr("The column '%1' is read-only! Operation aborted!").arg(d_selected_curve->title().text()));
	return;
   	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QTextStream ts( &text, QIODevice::ReadOnly );
    int start_point = QMIN(d_active_point, d_inactive_point);
    int start_row = ((DataCurve*)d_selected_curve)->tableRow(start_point);
    int end_point = QMAX(d_active_point, d_inactive_point);
    int end_row = ((DataCurve*)d_selected_curve)->tableRow(end_point);
    int col = t->colIndex(d_selected_curve->title().text());

    int prec; char f;
    t->columnNumericFormat(col, &f, &prec);
    QLocale locale = d_graph->plotWidget()->locale();
    for (int i = start_row; i <= end_row; i++){
        QString s = ts.readLine();
        if (s.isEmpty())
            continue;

        QStringList cellTexts = s.split("\t");
        if (cellTexts.count() >= 2){
            bool numeric;
            double value = locale.toDouble(cellTexts[1], &numeric);
			if (numeric)
                t->setText(i, col, locale.toString(value, f, prec));
			else
                t->setText(i, col, cellTexts[1]);
        }

        if(ts.atEnd())
            break;
    }

    t->notifyChanges();

    d_active_marker.setValue(d_selected_curve->x(d_active_point), d_selected_curve->y(d_active_point));
    d_inactive_marker.setValue(d_selected_curve->x(d_inactive_point), d_selected_curve->y(d_inactive_point));
    emitStatusText();
    emit changed();
    d_graph->plotWidget()->replot();

	QApplication::restoreOverrideCursor();
}

void RangeSelectorTool::setCurveRange()
{
    if (!d_selected_curve)
        return;

    if (((PlotCurve *)d_selected_curve)->type() != Graph::Function){
        ((DataCurve*)d_selected_curve)->setRowRange(QMIN(d_active_point, d_inactive_point),
                                    QMAX(d_active_point, d_inactive_point));
        d_graph->updatePlot();
        d_graph->notifyChanges();
    }
}

void RangeSelectorTool::setEnabled(bool on)
{
    d_enabled = on;
    if (on)
        d_graph->plotWidget()->canvas()->setCursor(QCursor(getQPixmap("vizor_xpm"), -1, -1));
}
