/***************************************************************************
    File                 : DataCurve.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : AbstractPlotCurve and DataCurve classes

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
#include "PlotCurve.h"
#include "ScaleDraw.h"
#include "SymbolBox.h"
#include "PatternBox.h"
#include "plot2D/ScaleEngine.h"
#include <QDateTime>
#include <QMessageBox>
#include <QPainter>
#include <qwt_symbol.h>

QString PlotCurve::saveCurveLayout()
{		
	Plot *plot = (Plot *)this->plot();
	Graph *g = (Graph *)plot->parent();
	
	int index = g->curveIndex((QwtPlotCurve *)this);
	int style = g->curveType(index);
	QString s = "<Style>" + QString::number(style) + "</Style>\n";
	
	if (style == Graph::Spline)
		s += "<LineStyle>5</LineStyle>\n";
	else if (style == Graph::VerticalSteps)
		s += "<LineStyle>6</LineStyle>\n";
	else
		s += "<LineStyle>" + QString::number(this->style()) + "</LineStyle>\n";
			
	QPen pen = this->pen();
	if (pen.style() != Qt::NoPen){
		s += "<Pen>\n";
		s += "\t<Color>" + pen.color().name() + "</Color>\n";
		s += "\t<Style>" + QString::number(pen.style()-1) + "</Style>\n";
		s += "\t<Width>" + QString::number(pen.widthF()) + "</Width>\n";
		s += "</Pen>\n";
	}
	
	QBrush brush = this->brush();
	if (brush.style() != Qt::NoBrush){
		s += "<Brush>\n";
		s += "\t<Color>" + brush.color().name() + "</Color>\n";
		s += "\t<Style>" + QString::number(PatternBox::patternIndex(brush.style())) + "</Style>\n";
		s += "</Brush>\n";
	}
	
	const QwtSymbol symbol = this->symbol();
	if (symbol.style() != QwtSymbol::NoSymbol){
		s += "<Symbol>\n";
		s += "\t<Style>" + QString::number(SymbolBox::symbolIndex(symbol.style())) + "</Style>\n";
		s += "\t<Size>" + QString::number(symbol.size().width()) + "</Size>\n";
	
		s += "\t<SymbolPen>\n";
		s += "\t\t<Color>" + symbol.pen().color().name() + "</Color>\n";
		s += "\t\t<Width>" + QString::number(symbol.pen().widthF()) + "</Width>\n";
		s += "\t</SymbolPen>\n";
	
		brush = this->brush();
		if (brush.style() != Qt::NoBrush){
			s += "\t<SymbolBrush>\n";
			s += "\t\t<Color>" + symbol.brush().color().name() + "</Color>\n";
			s += "\t\t<Style>" + QString::number(PatternBox::patternIndex(symbol.brush().style())) + "</Style>\n";
			s += "\t</SymbolBrush>\n";
		}
		s += "</Symbol>\n";
	}
	s += "<xAxis>" + QString::number(xAxis()) + "</xAxis>\n";
	s += "<yAxis>" + QString::number(yAxis()) + "</yAxis>\n";
	s += "<Visible>" + QString::number(isVisible()) + "</Visible>\n";
	return s;
}

void PlotCurve::restoreCurveLayout(const QStringList& lst)
{
	QStringList::const_iterator line = lst.begin();
	for (line++; line != lst.end(); line++){
        QString s = *line;
        if (s == "<Pen>"){
			QPen pen;
			while(s != "</Pen>"){
				s = (*(++line)).stripWhiteSpace();
				if (s.contains("<Color>"))
					pen.setColor(QColor(s.remove("<Color>").remove("</Color>")));
				else if (s.contains("<Style>"))
					pen.setStyle(Graph::getPenStyle(s.remove("<Style>").remove("</Style>").toInt()));
				else if (s.contains("<Width>"))
					pen.setWidthF(s.remove("<Width>").remove("</Width>").toDouble());	
			}
			setPen(pen);
		} else if (s == "<Brush>"){
			QBrush brush;
			while(s != "</Brush>"){
				s = (*(++line)).stripWhiteSpace();
				if (s.contains("<Color>"))
					brush.setColor(QColor(s.remove("<Color>").remove("</Color>")));
				else if (s.contains("<Style>"))
					brush.setStyle(PatternBox::brushStyle(s.remove("<Style>").remove("</Style>").toInt()));
			}
			setBrush(brush);
		} else if (s == "<Symbol>"){
			QwtSymbol symbol;
			while(s != "</Symbol>"){
				s = (*(++line)).stripWhiteSpace();
				if (s.contains("<Style>"))
					symbol.setStyle(SymbolBox::style(s.remove("<Style>").remove("</Style>").toInt()));
				else if (s.contains("<Size>"))
					symbol.setSize((QwtSymbol::Style)s.remove("<Size>").remove("</Size>").toInt());
				else if (s == "<SymbolPen>"){
					QPen pen;
					while(s != "</SymbolPen>"){
						s = (*(++line)).stripWhiteSpace();
						if (s.contains("<Color>"))
							pen.setColor(QColor(s.remove("<Color>").remove("</Color>")));
						else if (s.contains("<Style>"))
							pen.setStyle(Graph::getPenStyle(s.remove("<Style>").remove("</Style>").toInt()));
						else if (s.contains("<Width>"))
							pen.setWidthF(s.remove("<Width>").remove("</Width>").toDouble());	
					}
				symbol.setPen(pen);
				} else if (s == "<SymbolBrush>"){
					QBrush brush;
					while(s != "</SymbolBrush>"){
						s = (*(++line)).stripWhiteSpace();
						if (s.contains("<Color>"))
							brush.setColor(QColor(s.remove("<Color>").remove("</Color>")));
						else if (s.contains("<Style>"))
							brush.setStyle(PatternBox::brushStyle(s.remove("<Style>").remove("</Style>").toInt()));
					}
					symbol.setBrush(brush);
				}
				setSymbol(symbol);
			}		
		} else if (s.contains("<xAxis>"))
			setXAxis(s.remove("<xAxis>").remove("</xAxis>").toInt());
		else if (s.contains("<yAxis>"))
			setYAxis(s.remove("<yAxis>").remove("</yAxis>").toInt());
		else if (s.contains("<Visible>"))
			setVisible(s.remove("<Visible>").remove("</Visible>").toInt());
	}
}

void PlotCurve::aboutToBeDeleted()
{
  emit forgetMe(this);
}

DataCurve::DataCurve(Table *t, const QString& xColName, const QString& name, int startRow, int endRow):
    PlotCurve(name),
	d_table(t),
	d_x_column(xColName),
	d_start_row(startRow),
	d_end_row(endRow),
	d_labels_column(QString()),
	d_labels_color(Qt::black),
	d_labels_font(QFont()),
	d_labels_angle(0.0),
	d_white_out_labels(false),
	d_labels_align(Qt::AlignHCenter),
	d_labels_x_offset(0),
	d_labels_y_offset(50),
	d_selected_label(NULL)
{
	if (t && d_end_row < 0)
		d_end_row = t->numRows() - 1;
}

DataCurve::DataCurve(const DataCurve& c):
    PlotCurve(c.title().text()),
      d_table(c.d_table),
      d_x_column(c.d_x_column),
      d_start_row(c.d_start_row),
      d_end_row(c.d_end_row),
      d_labels_column(c.d_labels_column),
      d_labels_color(c.d_labels_color),
      d_labels_font(c.d_labels_font),
      d_labels_angle(c.d_labels_angle),
      d_white_out_labels(c.d_white_out_labels),
      d_labels_align(c.d_labels_align),
      d_labels_x_offset(c.d_labels_x_offset),
      d_labels_y_offset(c.d_labels_y_offset),
      d_selected_label(c.d_selected_label)
{
}

void DataCurve::setRowRange(int startRow, int endRow)
{
	if (d_start_row == startRow && d_end_row == endRow)
		return;

	d_start_row = startRow;
	d_end_row = endRow;

	loadData();

	foreach(DataCurve *c, d_error_bars)
		c->loadData();
}

void DataCurve::setFullRange()
{
	d_start_row = 0;
	d_end_row = d_table->numRows() - 1;

	loadData();

	foreach(DataCurve *c, d_error_bars)
		c->loadData();
}

bool DataCurve::isFullRange()const
{
	if (d_start_row != 0 || d_end_row != d_table->numRows() - 1)
		return false;
	else
		return true;
}

QString DataCurve::plotAssociation()const
{
	QString s = title().text();
    if (!d_x_column.isEmpty())
        s = d_x_column + "(X)," + title().text() + "(Y)";

    if (!d_labels_column.isEmpty())
        s += "," + d_labels_column + "(L)";

	return s;
}

void DataCurve::updateColumnNames(const QString& oldName, const QString& newName, bool updateTableName)
{
    if (updateTableName){
        QString s = title().text();
        QStringList lst = s.split("_", QString::SkipEmptyParts);
        if (lst[0] == oldName)
            setTitle(newName + "_" + lst[1]);

        if (!d_x_column.isEmpty()){
            lst = d_x_column.split("_", QString::SkipEmptyParts);
            if (lst[0] == oldName)
                d_x_column = newName + "_" + lst[1];
        }
    } else {
        if (title().text() == oldName)
            setTitle(newName);
        if (d_x_column == oldName)
            d_x_column = newName;
    }
}

bool DataCurve::updateData(Table *t, const QString& colName)
{
	if (d_table != t ||
       (colName != title().text() &&
       colName != d_x_column &&
       colName != d_labels_column))
        return false;

	loadData();
	return true;
}

void DataCurve::loadData()
{
    Plot *plot = (Plot *)this->plot();
	Graph *g = (Graph *)plot->parent();
	if (!g)
		return;

	int xcol = d_table->colIndex(d_x_column);
	int ycol = d_table->colIndex(title().text());

	if (xcol < 0 || ycol < 0){
		remove();
		return;
	}

	int r = abs(d_end_row - d_start_row) + 1;
    QVarLengthArray<double> X(r), Y(r);
	int xColType = d_table->columnType(xcol);
	int yColType = d_table->columnType(ycol);

	QStringList xLabels, yLabels;// store text labels

	QTime time0;
	QDateTime date0;
	QString date_time_fmt = d_table->columnFormat(xcol);
	if (xColType == Table::Time){
		for (int i = d_start_row; i <= d_end_row; i++ ){
			QString xval=d_table->text(i,xcol);
			if (!xval.isEmpty()){
				time0 = QTime::fromString (xval, date_time_fmt);
				if (time0.isValid())
					break;
			}
		}
	} else if (xColType == Table::Date){
		for (int i = d_start_row; i <= d_end_row; i++ ){
			QString xval=d_table->text(i,xcol);
			if (!xval.isEmpty()){
				date0 = QDateTime::fromString (xval, date_time_fmt);
				if (date0.isValid())
					break;
			}
		}
	}

	int size = 0;
	for (int i = d_start_row; i <= d_end_row; i++ ){
		QString xval = d_table->text(i,xcol);
		QString yval = d_table->text(i,ycol);
		if (!xval.isEmpty() && !yval.isEmpty()){
		    bool valid_data = true;
			if (xColType == Table::Text){
				xLabels << xval;
				X[size] = (double)(size + 1);
			} else if (xColType == Table::Time){
				QTime time = QTime::fromString (xval, date_time_fmt);
				if (time.isValid())
					X[size]= time0.msecsTo (time);
			} else if (xColType == Table::Date){
				QDateTime d = QDateTime::fromString (xval, date_time_fmt);
				if (d.isValid())
					X[size] = (double) date0.secsTo(d);
			} else
				X[size] = plot->locale().toDouble(xval, &valid_data);

			if (yColType == Table::Text){
				yLabels << yval;
				Y[size] = (double)(size + 1);
			} else
				Y[size] = plot->locale().toDouble(yval, &valid_data);

            if (valid_data)
                size++;
		}
	}

    X.resize(size);
    Y.resize(size);

	if (!size){
		remove();
		return;
	} else {
		if (d_type == Graph::HorizontalBars){
			setData(Y.data(), X.data(), size);
			foreach(DataCurve *c, d_error_bars)
                c->setData(Y.data(), X.data(), size);
		} else {
			setData(X.data(), Y.data(), size);
			foreach(DataCurve *c, d_error_bars)
                c->setData(X.data(), Y.data(), size);
		}

		if (xColType == Table::Text){
			if (d_type == Graph::HorizontalBars)
				g->setLabelsTextFormat(QwtPlot::yLeft, ScaleDraw::Text, d_x_column, xLabels);
			else
                g->setLabelsTextFormat(QwtPlot::xBottom, ScaleDraw::Text, d_x_column, xLabels);
		} else if (xColType == Table::Time || xColType == Table::Date){
			int axis = QwtPlot::xBottom;
			if (d_type == Graph::HorizontalBars)
                axis = QwtPlot::yLeft;
            ScaleDraw *old_sd = (ScaleDraw *)plot->axisScaleDraw(axis);
            ScaleDraw *sd = new ScaleDraw(plot, old_sd);
            if (xColType == Table::Date)
                sd->setDateTimeOrigin(date0);
            else
                sd->setDateTimeOrigin(QDateTime(QDate::currentDate(), time0));
            plot->setAxisScaleDraw(axis, sd);
		}

		if (yColType == Table::Text)
            g->setLabelsTextFormat(QwtPlot::yLeft, ScaleDraw::Text, title().text(), yLabels);
	}

    if (!d_labels_list.isEmpty()){
        ((Graph*)plot->parent())->updatePlot();
        loadLabels();
    }
}

void DataCurve::removeErrorBars(DataCurve *c)
{
	if (!c || d_error_bars.isEmpty())
		return;

	int index = d_error_bars.indexOf(c);
	if (index >= 0 && index < d_error_bars.size())
		d_error_bars.removeAt(index);
}

void DataCurve::clearErrorBars()
{
	if (d_error_bars.isEmpty())
		return;

	foreach(DataCurve *c, d_error_bars)
		c->remove();
}

void DataCurve::remove()
{
	Graph *g = (Graph *)plot()->parent();
	if (!g)
		return;

	g->removeCurve(title().text());
}

void DataCurve::setVisible(bool on)
{
	QwtPlotCurve::setVisible(on);
	foreach(DataCurve *c, d_error_bars)
		c->setVisible(on);
}

int DataCurve::tableRow(int point)const
{
    if (!d_table)
        return -1;

	int xcol = d_table->colIndex(d_x_column);
	int ycol = d_table->colIndex(title().text());

	if (xcol < 0 || ycol < 0)
		return -1;

    int xColType = d_table->columnType(xcol);
    if (xColType == Table::Date){
        QString format = d_table->columnFormat(xcol);
        QDate date0 = QDate::fromString (d_table->text(d_start_row, xcol), format);
        for (int i = d_start_row; i <= d_end_row; i++ ){
            QDate d = QDate::fromString (d_table->text(i, xcol), format);
            if (d.isValid()){
                if (d_type == Graph::HorizontalBars && date0.daysTo(d) == y(point) && d_table->cell(i, ycol) == x(point))
                    return i;
                else if (date0.daysTo(d) == x(point) && d_table->cell(i, ycol) == y(point))
                    return i;
            }
        }
    } else if (xColType == Table::Time){
        QString format = d_table->columnFormat(xcol);
        QTime t0 = QTime::fromString (d_table->text(d_start_row, xcol), format);
        for (int i = d_start_row; i <= d_end_row; i++ ){
            QTime t = QTime::fromString (d_table->text(i, xcol), format);
            if (t.isValid()){
                if (d_type == Graph::HorizontalBars && t0.msecsTo(t) == y(point) && d_table->cell(i, ycol) == x(point))
                    return i;
                if (t0.msecsTo(t) == x(point) && d_table->cell(i, ycol) == y(point))
                    return i;
            }
        }
    }

	double x_val = x(point);
	double y_val = y(point);
	for (int i = d_start_row; i <= d_end_row; i++ ){
		if (d_table->cell(i, xcol) == x_val && d_table->cell(i, ycol) == y_val)
			return i;
	}
	return -1;
}

void DataCurve::setLabelsColumnName(const QString& name)
{
	if (!validCurveType())
		return;

	if (d_labels_column == name && !d_labels_list.isEmpty())
		return;

	d_labels_column = name;
    loadLabels();
}

void DataCurve::loadLabels()
{
	if (!validCurveType())
		return;

    clearLabels();

    int xcol = d_table->colIndex(d_x_column);
    int ycol = d_table->colIndex(title().text());
    int labelsCol = d_table->colIndex(d_labels_column);
    int cols = d_table->numCols();
	if (xcol < 0 || ycol < 0 || labelsCol < 0 ||
        xcol >= cols || ycol >= cols || labelsCol >= cols)
		return;

	QwtPlot *d_plot = plot();
	if (!d_plot)
		return;

    int index = 0;
	for (int i = d_start_row; i <= d_end_row; i++){
		if (d_table->text(i, xcol).isEmpty() ||
            d_table->text(i, ycol).isEmpty())
            continue;

		PlotMarker *m = new PlotMarker(index, d_labels_angle);

		QwtText t = QwtText(d_table->text(i, labelsCol));
		t.setColor(d_labels_color);
		t.setFont(d_labels_font);
		if (d_white_out_labels)
			t.setBackgroundBrush(QBrush(Qt::white));
        else
            t.setBackgroundBrush(QBrush(Qt::transparent));
		m->setLabel(t);

        int x_axis = xAxis();
        int y_axis = yAxis();
		m->setAxis(x_axis, y_axis);

		QSize size = t.textSize();
        int dx = int(d_labels_x_offset*0.01*size.height());
        int dy = -int((d_labels_y_offset*0.01 + 0.5)*size.height());
        int x2 = d_plot->transform(x_axis, x(index)) + dx;
        int y2 = d_plot->transform(y_axis, y(index)) + dy;
        switch(d_labels_align){
            case Qt::AlignLeft:
            break;
            case Qt::AlignHCenter:
                x2 -= size.width()/2;
            break;
            case Qt::AlignRight:
                x2 -= size.width();
            break;
        }
        m->setXValue(d_plot->invTransform(x_axis, x2));
        m->setYValue(d_plot->invTransform(y_axis, y2));
		m->attach(d_plot);
		d_labels_list << m;
		index++;
	}
}

void DataCurve::clearLabels()
{
	if (!validCurveType())
		return;

	foreach(PlotMarker *m, d_labels_list){
		m->detach();
		delete m;
	}
	d_labels_list.clear();
}

void DataCurve::setLabelsFont(const QFont& font)
{
	if (!validCurveType())
		return;

     if (font == d_labels_font)
        return;

    d_labels_font = font;

    foreach(PlotMarker *m, d_labels_list){
		QwtText t = m->label();
		t.setFont(font);
		m->setLabel(t);
	}
	updateLabelsPosition();
}

void DataCurve::setLabelsColor(const QColor& c)
{
	if (!validCurveType())
		return;

    if (c == d_labels_color)
        return;

    d_labels_color = c;

    foreach(PlotMarker *m, d_labels_list){
		QwtText t = m->label();
		t.setColor(c);
		m->setLabel(t);
	}
}

void DataCurve::setLabelsAlignment(int flags)
{
	if (!validCurveType())
		return;

    if (flags == d_labels_align)
        return;

    d_labels_align = flags;
    updateLabelsPosition();
}

void DataCurve::updateLabelsPosition()
{
	if (!validCurveType())
		return;

    QwtPlot *d_plot = plot();
    if (!d_plot)
        return;

    foreach(PlotMarker *m, d_labels_list){
        int index = m->index();
        QSize size = m->label().textSize();
        int x_axis = xAxis();
        int y_axis = yAxis();
        int dx = int(d_labels_x_offset*0.01*size.height());
        int dy = -int((d_labels_y_offset*0.01 + 0.5)*size.height());
        int x2 = d_plot->transform(x_axis, x(index)) + dx;
        int y2 = d_plot->transform(y_axis, y(index)) + dy;
        switch(d_labels_align){
            case Qt::AlignLeft:
            break;
            case Qt::AlignHCenter:
                x2 -= size.width()/2;
            break;
            case Qt::AlignRight:
                x2 -= size.width();
            break;
        }
        m->setXValue(d_plot->invTransform(x_axis, x2));
        m->setYValue(d_plot->invTransform(y_axis, y2));
	}
}

void DataCurve::setLabelsOffset(int x, int y)
{
	if (!validCurveType())
		return;

    if (x == d_labels_x_offset && y == d_labels_y_offset)
        return;

    d_labels_x_offset = x;
    d_labels_y_offset = y;
    updateLabelsPosition();
}

void DataCurve::setLabelsRotation(double angle)
{
	if (!validCurveType())
		return;

    if (angle == d_labels_angle)
        return;

    d_labels_angle = angle;

    foreach(PlotMarker *m, d_labels_list)
		m->setAngle(angle);
}

void DataCurve::setLabelsWhiteOut(bool whiteOut)
{
	if (!validCurveType())
		return;

    if (whiteOut == d_white_out_labels)
        return;

    d_white_out_labels = whiteOut;

    foreach(PlotMarker *m, d_labels_list){
		QwtText t = m->label();
		if (whiteOut)
			t.setBackgroundBrush(QBrush(Qt::white));
        else
            t.setBackgroundBrush(QBrush(Qt::transparent));
		m->setLabel(t);
	}
}

void DataCurve::clone(DataCurve* c)
{
	if (!validCurveType())
		return;

    d_labels_color = c->labelsColor();
    d_labels_font = c->labelsFont();
	d_labels_angle = c->labelsRotation();
	d_white_out_labels = c->labelsWhiteOut();
	d_labels_align = c->labelsAlignment();
	d_labels_x_offset = c->labelsXOffset();
	d_labels_y_offset = c->labelsYOffset();

    if (!c->labelsColumnName().isEmpty()){
        plot()->replot();
        setLabelsColumnName(c->labelsColumnName());
    }
}

QString DataCurve::saveToString()
{
	if (!validCurveType())
		return QString();

    if (d_labels_list.isEmpty() || type() == Graph::Function || type() == Graph::Box)
        return QString();

    QString s = "<CurveLabels>\n";
    s += "\t<column>" + d_labels_column + "</column>\n";
    s += "\t<color>" + d_labels_color.name() + "</color>\n";
    s += "\t<whiteOut>" + QString::number(d_white_out_labels) + "</whiteOut>\n";
    s += "\t<font>" + d_labels_font.family() + "\t";
    s += QString::number(d_labels_font.pointSize()) + "\t";
    s += QString::number(d_labels_font.bold()) + "\t";
    s += QString::number(d_labels_font.italic()) + "\t";
    s += QString::number(d_labels_font.underline()) + "</font>\n";
    s += "\t<angle>" + QString::number(d_labels_angle) + "</angle>\n";
    s += "\t<justify>" + QString::number(d_labels_align) + "</justify>\n";
    if (d_labels_x_offset != 0.0)
        s += "\t<xoffset>" + QString::number(d_labels_x_offset) + "</xoffset>\n";
    if (d_labels_y_offset != 0.0)
        s += "\t<yoffset>" + QString::number(d_labels_y_offset) + "</yoffset>\n";
    return s + "</CurveLabels>\n";
}

bool DataCurve::selectedLabels(const QPoint& pos)
{
  if (!validCurveType())
    return false;

  QwtPlot *d_plot = plot();
  if (!d_plot)
    return false;


  bool selected = false;
  d_selected_label = NULL;
  foreach(PlotMarker *m, d_labels_list){
    int x = d_plot->transform(xAxis(), m->xValue());
    int y = d_plot->transform(yAxis(), m->yValue());
    if (QRect(QPoint(x, y), m->label().textSize()).contains(pos)){
      d_selected_label = m;
      d_click_pos_x = d_plot->invTransform(xAxis(), pos.x());
      d_click_pos_y = d_plot->invTransform(yAxis(), pos.y());
      setLabelsSelected();
      return true;
    }
  }
  return selected;
}

bool DataCurve::hasSelectedLabels()const
{
	if (!validCurveType())
		return false;

    if (d_labels_list.isEmpty())
        return false;

    foreach(PlotMarker *m, d_labels_list){
        if (m->label().backgroundPen() == QPen(Qt::blue))
            return true;
        else
            return false;
    }
    return false;
}

void DataCurve::setLabelsSelected(bool on)
{
	if (!validCurveType())
		return;

    foreach(PlotMarker *m, d_labels_list){
		QwtText t = m->label();
		if(t.text().isEmpty())
            continue;

        if (on){
            t.setBackgroundPen(QPen(Qt::blue));
        } else
            t.setBackgroundPen(QPen(Qt::NoPen));
        m->setLabel(t);
    }
    if (on){
        Graph *g = (Graph *)plot()->parent();
        g->selectTitle(false);
        g->deselectMarker();
        g->notifyFontChange(d_labels_font);
    }
    plot()->replot();
}

bool DataCurve::validCurveType()const
{
	int style = type();
	if (style == Graph::Function || style == Graph::Box ||
		style == Graph::Pie || style == Graph::ErrorBars ||
		style == Graph::ColorMap ||  style == Graph::GrayScale ||
		style == Graph::Contour || style == Graph::ImagePlot)
		return false;
	return true;
}

void DataCurve::moveLabels(const QPoint& pos)
{
	if (!validCurveType())
		return;
	if (!d_selected_label || d_labels_list.isEmpty())
		return;

    QwtPlot *d_plot = plot();
    if (!d_plot)
        return;

    d_plot->replot();
    int d_x = pos.x() - d_plot->transform(xAxis(), d_click_pos_x);
	int d_y = pos.y() - d_plot->transform(yAxis(), d_click_pos_y);

	int height = d_selected_label->label().textSize().height();
	d_labels_x_offset += int(d_x*100.0/(double)height);
    d_labels_y_offset -= int(d_y*100.0/(double)height);

	updateLabelsPosition();
	d_plot->replot();

    ((Graph *)d_plot->parent())->notifyChanges();

	d_click_pos_x = d_plot->invTransform(xAxis(), pos.x());
	d_click_pos_y = d_plot->invTransform(yAxis(), pos.y());
}

PlotCurve* DataCurve::clone()const
{
  return new DataCurve(*this);
}

PlotMarker::PlotMarker(int index, double angle):QwtPlotMarker(),
	d_index(index),
	d_angle(angle),
	d_label_x_offset(0.0),
	d_label_y_offset(0.0)
{}

void PlotMarker::draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &) const
{
	p->save();
	int x = xMap.transform (xValue());
	int y = yMap.transform (yValue());

	p->translate(x, y);
	p->rotate(-d_angle);

	QwtText text = label();
	text.draw(p, QRect(QPoint(0, 0), text.textSize()));
	p->restore();
}

