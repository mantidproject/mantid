/***************************************************************************
    File                 : LegendWidget.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : A 2D Plot Legend Widget

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
#include "LegendWidget.h"
#include "QwtPieCurve.h"
#include "VectorCurve.h"
#include "SelectionMoveResizer.h"
#include "ApplicationWindow.h"

#include <QPainter>
#include <QPolygon>
#include <QMessageBox>

#include <qwt_plot.h>
#include <qwt_scale_widget.h>
#include <qwt_painter.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <qwt_layout_metrics.h>
#include <qwt_symbol.h>

#include <iostream>

LegendWidget::LegendWidget(Plot *plot):QWidget(plot),
	d_plot(plot),
	d_frame (0),
	d_angle(0),
	d_fixed_coordinates(false)
{
	setAttribute(Qt::WA_DeleteOnClose);

	d_text = new QwtText(QString::null, QwtText::RichText);
	d_text->setFont(QFont("Arial", 12, QFont::Normal, false));
	d_text->setRenderFlags(Qt::AlignTop|Qt::AlignLeft);
	d_text->setBackgroundBrush(QBrush(Qt::NoBrush));
	d_text->setColor(Qt::black);
	d_text->setBackgroundPen (QPen(Qt::NoPen));
	d_text->setPaintAttribute(QwtText::PaintBackground);

	h_space = 5;
	left_margin = 10;
	top_margin = 5;
	line_length = 20;

	QPoint pos = plot->canvas()->pos();
	pos = QPoint(pos.x() + 10, pos.y() + 10);
	move(pos);

    d_selector = NULL;

	connect (this, SIGNAL(showDialog()), plot->parent(), SIGNAL(viewTextDialog()));
	connect (this, SIGNAL(showMenu()), plot->parent(), SIGNAL(showMarkerPopupMenu()));
	connect (this, SIGNAL(enableEditor()), plot->parent(), SLOT(enableTextEditor()));

	setMouseTracking(true);
	show();
	setFocus();
}

void LegendWidget::paintEvent(QPaintEvent *e)
{
	if (d_fixed_coordinates){
		setOriginCoord(d_x, d_y);
		d_fixed_coordinates = false;
	}

	const int symbolLineLength = line_length + symbolsMaxWidth();
	int width, height;
	QwtArray<int> heights = itemsHeight(0, symbolLineLength, width, height);
	if (d_frame == Shadow)
		resize(width + 5, height + 5);
	else
    	resize(width, height);

	QRect rect = QRect(0, 0, width - 1, height - 1);
    QPainter p(this);
	drawFrame(&p, rect);
	drawText(&p, rect, heights, symbolLineLength);
	e->accept();
}

void LegendWidget::print(QPainter *painter, const QwtScaleMap map[QwtPlot::axisCnt])
{
	int x = map[QwtPlot::xBottom].transform(xValue());
	int y = map[QwtPlot::yLeft].transform(yValue());

    const int symbolLineLength = line_length + symbolsMaxWidth();
	int width, height;
	QwtArray<int> heights = itemsHeight(y, symbolLineLength, width, height);

	QRect rect = QRect(x, y, width, height);
	drawFrame(painter, rect);
	drawText(painter, rect, heights, symbolLineLength);
}

void LegendWidget::setText(const QString& s)
{
	d_text->setText(s);
}

void LegendWidget::setFrameStyle(int style)
{
	if (d_frame == style)
		return;

	d_frame = style;
}

void LegendWidget::setBackgroundColor(const QColor& c)
{
	if (d_text->backgroundBrush().color() == c)
		return;

	d_text->setBackgroundBrush(QBrush(c));
}

void LegendWidget::setTextColor(const QColor& c)
{
	if ( c == d_text->color() )
		return;

	d_text->setColor(c);
}

void LegendWidget::setOriginCoord(double x, double y)
{
	QPoint pos(d_plot->transform(QwtPlot::xBottom, x), d_plot->transform(QwtPlot::yLeft, y));
	pos = d_plot->canvas()->mapToParent(pos);
	move(pos);
}

double LegendWidget::xValue()
{
	QPoint d_pos = d_plot->canvas()->mapFromParent(geometry().topLeft());
	return d_plot->invTransform(QwtPlot::xBottom, d_pos.x());
}

double LegendWidget::yValue()
{
	QPoint d_pos = d_plot->canvas()->mapFromParent(geometry().topLeft());
	return d_plot->invTransform(QwtPlot::yLeft, d_pos.y());
}

void LegendWidget::setFont(const QFont& font)
{
	if ( font == d_text->font() )
		return;

	d_text->setFont(font);
}

void LegendWidget::drawFrame(QPainter *p, const QRect& rect)
{
	p->save();
	p->setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
	if (d_frame == None)
		p->fillRect (rect, d_text->backgroundBrush());

	if (d_frame == Line){
		p->setBrush(d_text->backgroundBrush());
		QwtPainter::drawRect(p, rect);
	} else if (d_frame == Shadow) {
		QRect shadow_right = QRect(rect.right() + 1, rect.y() + 5, 5, rect.height());
		QRect shadow_bottom = QRect(rect.x() + 5, rect.bottom() + 1, rect.width(), 5);
		p->setBrush(QBrush(Qt::black));
		p->drawRect(shadow_right);
		p->drawRect(shadow_bottom);

		p->setBrush(d_text->backgroundBrush());
		QwtPainter::drawRect(p,rect);
	}
	p->restore();
}

void LegendWidget::drawVector(PlotCurve *c, QPainter *p, int x, int y, int l)
{
	if (!c)
		return;

	VectorCurve *v = (VectorCurve*)c;
	p->save();

	if (((Graph *)d_plot->parent())->antialiasing())
		p->setRenderHints(QPainter::Antialiasing);

	QPen pen(v->color(), v->width(), Qt::SolidLine);
	p->setPen(pen);
	p->drawLine(x, y, x + l, y);

	p->translate(x+l, y);

	double pi=4*atan(-1.0);
	int headLength = v->headLength();
	int d=qRound(headLength*tan(pi*(double)v->headAngle()/180.0));

	QPolygon endArray(3);
	endArray[0] = QPoint(0, 0);
	endArray[1] = QPoint(-headLength, d);
	endArray[2] = QPoint(-headLength, -d);

	if (v->filledArrowHead())
		p->setBrush(QBrush(pen.color(), Qt::SolidPattern));

	p->drawPolygon(endArray);
	p->restore();
}

void LegendWidget::drawSymbol(PlotCurve *c, int point, QPainter *p, int x, int y, int l)
{
    if (!c || c->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
        return;

    if (c->type() == Graph::VectXYXY || c->type() == Graph::VectXYAM){
        drawVector(c, p, x, y, l);
        return;
    }

	if (c->type() == Graph::Pie){
		QwtPieCurve *pie = (QwtPieCurve *)c;
		const QBrush br = QBrush(pie->color(point), pie->pattern());
		QPen pen = pie->pen();
		p->save();
		p->setPen (QPen(pen.color(), pen.widthF(), Qt::SolidLine));
		QRect lr = QRect(x, y - 4, l, 10);
		p->setBrush(br);
		QwtPainter::drawRect(p, lr);
		p->restore();
		return;
	}

    QwtSymbol symb = c->symbol();
    const QBrush br = c->brush();
    QPen pen = c->pen();
    p->save();
    if (c->style()!=0){
        p->setPen (pen);
        if (br.style() != Qt::NoBrush || c->type() == Graph::Box){
            QRect lr = QRect(x, y-4, l, 10);
            p->setBrush(br);
            QwtPainter::drawRect(p, lr);
        } else
            QwtPainter::drawLine(p, x, y, x + l, y);
    }
    int symb_size = symb.size().width();
    if (symb_size > 15)
        symb_size = 15;
    else if (symb_size < 3)
        symb_size = 3;
    symb.setSize(symb_size);
    symb.draw(p, x + l/2, y);
    p->restore();
}

void LegendWidget::drawText(QPainter *p, const QRect& rect,
		QwtArray<int> height, int symbolLineLength)
{
  p->save();
  if (((Graph *)d_plot->parent())->antialiasing())
    p->setRenderHints(QPainter::Antialiasing);

  // RJT (22/09/09): For remainder of method, copied in code from current 
  // QtiPlot (rev. 1373) to fix infinite loop if closing bracket missing
  int l = symbolLineLength;
  QString text = d_text->text();
  QStringList titles = text.split("\n", QString::KeepEmptyParts);

  for (int i=0; i<(int)titles.count(); i++){
    int w = left_margin + rect.x(); // QtiPlot Rev 1373 has this as 2nd arg: d_frame_pen.width();
    bool  curveSymbol = false;
    QString s = titles[i];
    while (s.contains("\\l(",Qt::CaseInsensitive) || s.contains("\\p{",Qt::CaseInsensitive)){
      curveSymbol = true;
      int pos = s.indexOf("\\l(", 0,Qt::CaseInsensitive);
      int pos2 = s.indexOf(",",pos); // two arguments in case if pie chart
      int pos3 = s.indexOf(")",pos);
      if (pos >= 0 && (pos2 == -1 || pos2 > pos3)){
        QwtText aux(parse(s.left(pos))); //not a pie chart
        aux.setFont(d_text->font());
        aux.setColor(d_text->color());
        aux.setRenderFlags (Qt::AlignLeft | Qt::AlignVCenter);

        QSize size = aux.textSize(); // In QtiPlot rev 1373: textSize(p, aux);
        QRect tr = QRect(QPoint(w, height[i] - size.height()/2), size);
        aux.draw(p, tr);
        w += size.width();

        int pos1 = s.indexOf("(", pos);
        int pos2 = s.indexOf(")", pos1);
        if (pos2 == -1){
          s = s.right(s.length() - pos1 - 1);
          continue;
        }
        int point = -1;
        PlotCurve *curve = getCurve(s.mid(pos1+1, pos2-pos1-1), point);
        if (!curve){
          s = s.right(s.length() - pos2 - 1);
          continue;
        }

        drawSymbol(curve, point, p, w, height[i], l);
        w += l + h_space;
        s = s.right(s.length() - pos2 - 1);
      } else { // pie chart?
        pos = s.indexOf("\\p{", 0);
        if (pos >= 0){  // old syntax
          QwtText aux(parse(s.left(pos)));
          aux.setFont(d_text->font());
          aux.setColor(d_text->color());
          aux.setRenderFlags (Qt::AlignLeft | Qt::AlignVCenter);

          QSize size = aux.textSize(); // In QtiPlot rev 1373: textSize(p, aux);
          QRect tr = QRect(QPoint(w, height[i] - size.height()/2), size);
          aux.draw(p, tr);
          w += size.width();

          int pos1 = s.indexOf("{", pos);
          int pos2 = s.indexOf("}", pos1);
          if (pos2 == -1){
          s = s.right(s.length() - pos1 - 1);
          continue;
          }
          int point = s.mid(pos1 + 1, pos2 - pos1 - 1).toInt() - 1;
          drawSymbol((PlotCurve*)d_plot->curve(0), point, p, w, height[i], l);
          w += l;
          s = s.right(s.length() - pos2 - 1);
        } else {
          pos = s.indexOf("\\l(", 0,Qt::CaseInsensitive);
          if (pos >= 0 && pos2 != -1){ //new syntax
            QwtText aux(parse(s.left(pos)));
            aux.setFont(d_text->font());
            aux.setColor(d_text->color());
            aux.setRenderFlags (Qt::AlignLeft | Qt::AlignVCenter);

            QSize size = aux.textSize(); // In QtiPlot rev 1373: textSize(p, aux);
            QRect tr = QRect(QPoint(w, height[i] - size.height()/2), size);
            aux.draw(p, tr);
            w += size.width();

            int pos1 = s.indexOf(",", pos);
            int pos3 = s.indexOf(")", pos1);
            if (pos3 == -1){
              s = s.right(s.length() - pos1 - 1);
              continue;
            }
            int point = s.mid(pos1 + 1, pos3 - pos1 - 1).toInt() - 1;
            drawSymbol((PlotCurve*)d_plot->curve(0), point, p, w, height[i], l);
            w += l;
            s = s.right(s.length() - pos3 - 1);
          }
        }
      }
    }

    if (!s.isEmpty()){
      if (curveSymbol)
        w += h_space;
      QwtText aux(parse(s));
      aux.setFont(d_text->font());
      aux.setColor(d_text->color());
      aux.setRenderFlags (Qt::AlignLeft | Qt::AlignVCenter);

      QSize size = aux.textSize(); // In QtiPlot rev 1373: textSize(p, aux);
      QRect tr = QRect(QPoint(w, height[i] - size.height()/2), size);
      aux.draw(p, tr);
    }
  }
  p->restore();
}

QwtArray<int> LegendWidget::itemsHeight(int y, int symbolLineLength, int &width, int &height)
{
  // RJT (22/09/09): For most of method, copied in code from current 
  // QtiPlot (rev. 1373) to fix infinite loop if closing bracket missing
  QString text = d_text->text();
  QStringList titles = text.split("\n", QString::KeepEmptyParts);
  int n = (int)titles.count();
  QwtArray<int> heights(n);

  width = 0;
  height = 0;
  int maxL = 0;
  int h = top_margin; // In QtiPlot rev 1373: + d_frame_pen.width();
  for (int i=0; i<n; i++){
    QString s = titles[i];
    int textL = 0;
    bool curveSymbol = false;
    while (s.contains("\\l(",Qt::CaseInsensitive) || s.contains("\\p{",Qt::CaseInsensitive)){
      int pos = s.indexOf("\\l(", 0,Qt::CaseInsensitive);
      int pos2 = s.indexOf(",",pos); // two arguments in case if pie chart
      int pos3 = s.indexOf(")",pos);
      curveSymbol = true;
      if (pos >= 0 && (pos2==-1 || pos2>pos3)){
        QwtText aux(parse(s.left(pos))); //not a pie chart
        aux.setFont(d_text->font());
        QSize size = aux.textSize(); // In QtiPlot rev 1373: textSize(p, aux);
        textL += size.width();

        int pos1 = s.indexOf("(", pos);
        int pos2 = s.indexOf(")", pos1);
        if (pos2 == -1){
          s = s.right(s.length() - pos1 - 1);
          continue;
        }
        int point = -1;
        PlotCurve *curve = getCurve(s.mid(pos1+1, pos2-pos1-1), point);
        if (!curve){
          s = s.right(s.length() - pos2 - 1);
          continue;
        }

        textL += symbolLineLength + h_space;
        s = s.right(s.length() - s.indexOf(")", pos) - 1);
      } else { //Pie chart?
        pos = s.indexOf("\\p{", 0,Qt::CaseInsensitive); //look for old syntax
        if (pos >= 0){
          QwtText aux(parse(s.left(pos)));
          aux.setFont(d_text->font());
          QSize size = aux.textSize(); // In QtiPlot rev 1373: textSize(p, aux);
          textL += size.width();
          textL += symbolLineLength;
          int pos2=s.indexOf("}", pos);
          if (pos2==-1) pos2=pos+3;
          s = s.right(s.length() - pos2 - 1);
        } else {
          pos = s.indexOf("\\l(", 0,Qt::CaseInsensitive); // new syntax
          if (pos >= 0){
            QwtText aux(parse(s.left(pos)));
            aux.setFont(d_text->font());
            QSize size = aux.textSize(); // In QtiPlot rev 1373: textSize(p, aux);
            textL += size.width();
            textL += symbolLineLength;
            int pos2=s.indexOf(")", pos);
            if (pos2==-1) pos2=pos+3;
            s = s.right(s.length() - pos2 - 1);
          }
        }
      }
    }
    // RJT (22/09/09): End copied in code from rev. 1373

    QwtText aux(parse(s));
    aux.setFont(d_text->font());
    QSize size = aux.textSize();
    textL += size.width();

    if (textL > maxL)
      maxL = textL;

    int textH = size.height();
    height += textH;

    heights[i] = y + h + textH/2;
    h += textH;
  }

  height += 2*top_margin;
  width = 2*left_margin + maxL + h_space;

  return heights;
}

int LegendWidget::symbolsMaxWidth()
{
  QList<int> cvs = d_plot->curveKeys();
  int curves = cvs.count();
  if (!curves)
    return 0;

  // RJT (22/09/09): For rest of method, copied in code from current 
  // QtiPlot (rev. 1373) to fix infinite loop if closing bracket missing
  int maxL = 0;
  QString text = d_text->text();
  QStringList titles = text.split("\n", QString::KeepEmptyParts);
  for (int i=0; i<(int)titles.count(); i++){
    QString s = titles[i];
    while (s.contains("\\l(",Qt::CaseInsensitive)){
      int pos = s.indexOf("\\l(", 0,Qt::CaseInsensitive);
      int pos1 = s.indexOf("(", pos);
      int pos2 = s.indexOf(")", pos1);
      int pos3 = s.indexOf(",",pos1);
      if (pos3 != -1 && pos3 < pos2 ) pos2=pos3; // for pi charts get dataset number
      if (pos2 == -1){
        s = s.right(s.length() - pos1 - 1);
        continue;
      }

      int point = 0;
      PlotCurve* c = getCurve(s.mid(pos1 + 1, pos2 - pos1 - 1), point);
      if (c && c->type() == Graph::Pie){
        maxL = 2*d_text->font().pointSize();//10;
        line_length = 0;
        s = s.right(s.length() - pos2 - 1);
        continue;
      }

      if (c && c->rtti() != QwtPlotItem::Rtti_PlotSpectrogram) {
        if (c->type() == Graph::Pie ||
          c->type() == Graph::VerticalBars ||
          c->type() == Graph::HorizontalBars ||
          c->type() == Graph::Histogram ||
          c->type() == Graph::Box){
          maxL = 2*d_text->font().pointSize();//10;
          line_length = 0;
        } else {
          int l = c->symbol().size().width();
          if (l < 3)
            l = 3;
          else if (l > 15)
            l = 15;
          if (l>maxL && c->symbol().style() != QwtSymbol::NoSymbol)
            maxL = l;
        }
      }
      s = s.right(s.length() - pos2 - 1);
    }

    if (titles[i].contains("\\p{")){ // old syntax for pie charts
      maxL = 2*d_text->font().pointSize();//10;
      line_length = 0;
    }
  }
  return maxL;
}

QString LegendWidget::parse(const QString& str)
{
    QString s = str;
    s.remove(QRegExp("\\l(*)", Qt::CaseSensitive, QRegExp::Wildcard));
    s.remove(QRegExp("\\p{*}", Qt::CaseSensitive, QRegExp::Wildcard));

	QString aux = str;
    while (aux.contains(QRegExp("%(*)", Qt::CaseInsensitive, QRegExp::Wildcard))){//curve name specification
		int pos = str.indexOf("%(", 0, Qt::CaseInsensitive);
        int pos2 = str.indexOf(")", pos, Qt::CaseInsensitive);
		QString spec = str.mid(pos + 2, pos2 - pos - 2);
		QStringList lst = spec.split(",");
		if (!lst.isEmpty()){
        	int cv = lst[0].toInt() - 1;
			Graph *g = (Graph *)d_plot->parent();
        	if (g && cv >= 0 && cv < g->curves()){
				PlotCurve *c = (PlotCurve *)g->curve(cv);
            	if (c){
					if (lst.count() == 1)
						s = s.replace(pos, pos2-pos+1, c->title().text());
					else if (lst.count() == 3 && c->type() == Graph::Pie){
						Table *t = ((DataCurve *)c)->table();
						int col = t->colIndex(c->title().text());
						int row = lst[2].toInt() - 1;
						s = s.replace(pos, pos2-pos+1, t->text(row, col));
					}
				}
        	}
			aux = aux.right(aux.length() - pos2 - 1);
		}
    }
    return s;
}

PlotCurve* LegendWidget::getCurve(const QString& s, int &point)
{
	point = -1;
	PlotCurve *curve = 0;
	Graph *g = (Graph *)d_plot->parent();

	QStringList l = s.split(",");
    if (l.count() == 2)
		point = l[1].toInt() - 1;

	if (!l.isEmpty()){
		l = l[0].split(".");
    	if (l.count() == 2){
    		int cv = l[1].toInt() - 1;
			Graph *layer = g->multiLayer()->layer(l[0].toInt());
			if (layer && cv >= 0 && cv < layer->curves())
				return (PlotCurve*)layer->curve(cv);
		} else if (l.count() == 1){
			int cv = l[0].toInt() - 1;
			if (cv >= 0 || cv < g->curves())
				return (PlotCurve*)g->curve(cv);
		}
	}
	return curve;
}

void LegendWidget::mousePressEvent (QMouseEvent *)
{
    if (d_selector){
        delete d_selector;
		d_selector = NULL;
	}

	((Graph *)d_plot->parent())->activateGraph();
	((Graph *)d_plot->parent())->deselectMarker();

    d_selector = new SelectionMoveResizer(this);
	connect(d_selector, SIGNAL(targetsChanged()), (Graph*)d_plot->parent(), SIGNAL(modifiedGraph()));
	((Graph *)d_plot->parent())->setSelectedText(this);
}

void LegendWidget::setSelected(bool on)
{
	if (on){
		if (d_selector)
			return;
		else {
			d_selector = new SelectionMoveResizer(this);
			connect(d_selector, SIGNAL(targetsChanged()), (Graph*)d_plot->parent(), SIGNAL(modifiedGraph()));
			((Graph *)d_plot->parent())->setSelectedText(this);
		}
	} else if (d_selector){
		d_selector->close();
		d_selector = NULL;
		((Graph *)d_plot->parent())->setSelectedText(NULL);
	}
}

void LegendWidget::showTextEditor()
{
  // RJT (22/09/09): The code below caused a warning from the QObject destructor, which can't be good
  // The d_selector member is completely gone from the current version of this code
  //if (d_selector){
  //  delete d_selector;
  //  d_selector = NULL;
  //}

    ApplicationWindow *app = ((Graph *)d_plot->parent())->multiLayer()->applicationWindow();
    if (!app)
        return;

	if (app->d_in_place_editing)
        enableEditor();
    else
        showDialog();
}

void LegendWidget::clone(LegendWidget* t)
{
	d_frame = t->frameStyle();
	d_angle = t->angle();

	setTextColor(t->textColor());
	setBackgroundColor(t->backgroundColor());
	setFont(t->font());
	setText(t->text());
	move(t->pos());
}

void LegendWidget::setFixedCoordinatesMode(bool on)
{
	if (d_fixed_coordinates == on)
		return;

	d_fixed_coordinates = on;

	if(on){
		d_x = xValue();
		d_y = yValue();
	}
}

LegendWidget::~LegendWidget()
{
	delete d_text;
	if (d_selector)
        delete d_selector;
}
