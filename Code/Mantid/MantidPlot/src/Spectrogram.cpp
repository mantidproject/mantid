/***************************************************************************
	File                 : Spectrogram.cpp
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2006 by Ion Vasilief
	Email (use @ for *)  : ion_vasilief*yahoo.fr
	Description          : QtiPlot's Spectrogram Class
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
#include "Spectrogram.h"
#include <math.h>
#include <QPen>
#include <qwt_scale_widget.h>
#include <QColor>
#include <qwt_painter.h>
#include "qwt_scale_engine.h"
#include <QPainter>
#include <qwt_symbol.h>


#include <iostream>
#include <numeric>

#include "MantidAPI/MatrixWorkspace.h"
using namespace Mantid::API;

//Mantid::Kernel::Logger & Spectrogram::g_log=Mantid::Kernel::Logger::get("Spectrogram");
Spectrogram::Spectrogram():
	    QwtPlotSpectrogram(),
	    d_matrix(0),d_funct(0),//Mantid
	    color_axis(QwtPlot::yRight),
	    color_map_policy(Default),
	    color_map(QwtLinearColorMap())
{
}

Spectrogram::Spectrogram(Matrix *m):
	    QwtPlotSpectrogram(QString(m->objectName())),
	    d_matrix(m),d_funct(0),//Mantid
	    color_axis(QwtPlot::yRight),
	    color_map_policy(Default),mColorMap()
//color_map(QwtLinearColorMap()),
{
  setData(MatrixData(m));
  double step = fabs(data().range().maxValue() - data().range().minValue())/5.0;

  QwtValueList contourLevels;
  for ( double level = data().range().minValue() + step;
      level < data().range().maxValue(); level += step )
    contourLevels += level;

  setContourLevels(contourLevels);
}

Spectrogram::Spectrogram(UserHelperFunction *f,int nrows, int ncols,double left, double top, double width, double height,double minz,double maxz)
:	QwtPlotSpectrogram(),
 	d_matrix(0),d_funct(f),
 	color_axis(QwtPlot::yRight),
 	color_map_policy(Default),
 	color_map(QwtLinearColorMap())
{
  setData(FunctionData(f,nrows,ncols,left,top,width,height,minz,maxz));
  double step = fabs(data().range().maxValue() - data().range().minValue())/5.0;

  QwtValueList contourLevels;
  for ( double level = data().range().minValue() + step;
      level < data().range().maxValue(); level += step )
    contourLevels += level;

  setContourLevels(contourLevels);

}

Spectrogram::Spectrogram(UserHelperFunction *f,int nrows, int ncols,QwtDoubleRect bRect,double minz,double maxz)
:	QwtPlotSpectrogram(),
 	d_color_map_pen(false),
 	d_matrix(0),
 	d_funct(f),
 	color_axis(QwtPlot::yRight),
 	color_map_policy(Default),
 	d_show_labels(true),
 	d_white_out_labels(false),
 	d_labels_angle(0.0),
 	d_selected_label(NULL),
  d_labels_color(Qt::black),
 	d_labels_x_offset(0), 	d_labels_y_offset(0),
 	d_labels_align(Qt::AlignHCenter),
 	d_labels_font(QFont()),
 	mColorMap(),
 	m_nRows(nrows),
 	m_nColumns(ncols),
 	mScaledValues(0),
 	m_bIntensityChanged(false)
{
  setTitle("UserHelperFunction");
  setData(FunctionData(f,nrows,ncols,bRect,minz,maxz));
  double step = fabs(data().range().maxValue() - data().range().minValue())/5.0;

  QwtValueList contourLevels;
  for ( double level = data().range().minValue() + step;
      level < data().range().maxValue(); level += step )
    contourLevels += level;
  setContourLevels(contourLevels);
}


Spectrogram::~Spectrogram()
{
}


void Spectrogram::setContourLevels (const QwtValueList & levels)
{
  QwtPlotSpectrogram::setContourLevels(levels);
  createLabels();
}

void Spectrogram::updateData(Matrix *m)
{
  if (!m)
    return;

  QwtPlot *plot = this->plot();
  if (!plot)
    return;

  setData(MatrixData(m));
  setLevelsNumber(levels());

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  if (colorAxis)
  {
    colorAxis->setColorMap(data().range(), colorMap());
  }
  plot->setAxisScale(color_axis, data().range().minValue(), data().range().maxValue());
  plot->replot();
}

void Spectrogram::setLevelsNumber(int levels)
{
  double step = fabs(data().range().maxValue() - data().range().minValue())/(double)levels;

  QwtValueList contourLevels;
  for ( double level = data().range().minValue() + step;
      level < data().range().maxValue(); level += step )
    contourLevels += level;

  setContourLevels(contourLevels);
}

bool Spectrogram::hasColorScale()
{
  QwtPlot *plot = this->plot();
  if (!plot)
    return false;

  if (!plot->axisEnabled (color_axis))
    return false;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  return colorAxis->isColorBarEnabled();
}

void Spectrogram::showColorScale(int axis, bool on)
{
  if (hasColorScale() == on && color_axis == axis)
    return;

  QwtPlot *plot = this->plot();
  if (!plot)
    return;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  colorAxis->setColorBarEnabled(false);

  color_axis = axis;

  // We must switch main and the color scale axes and their respective scales
  int xAxis = this->xAxis();
  int yAxis = this->yAxis();
  int oldMainAxis;
  if (axis == QwtPlot::xBottom || axis == QwtPlot::xTop)
  {
    oldMainAxis = xAxis;
    xAxis = 5 - color_axis;
  }
  else if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight)
  {
    oldMainAxis = yAxis;
    yAxis = 1 - color_axis;
  }

  // First we switch axes
  setAxis(xAxis, yAxis);

  // Next we switch axes scales
  QwtScaleDiv *scDiv = plot->axisScaleDiv(oldMainAxis);
  if (axis == QwtPlot::xBottom || axis == QwtPlot::xTop)
    plot->setAxisScale(xAxis, scDiv->lBound(), scDiv->hBound());
  else if (axis == QwtPlot::yLeft || color_axis == QwtPlot::yRight)
    plot->setAxisScale(yAxis, scDiv->lBound(), scDiv->hBound());

  colorAxis = plot->axisWidget(color_axis);
  plot->setAxisScale(color_axis, data().range().minValue(), data().range().maxValue());
  colorAxis->setColorBarEnabled(on);
  colorAxis->setColorMap(data().range(), colorMap());
  if (!plot->axisEnabled(color_axis))
    plot->enableAxis(color_axis);
  colorAxis->show();
  plot->updateLayout();
}

int Spectrogram::colorBarWidth()
{
  QwtPlot *plot = this->plot();
  if (!plot)
    return 0;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  return colorAxis->colorBarWidth();
}

void Spectrogram::setColorBarWidth(int width)
{
  QwtPlot *plot = this->plot();
  if (!plot)
    return;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  colorAxis->setColorBarWidth(width);
}

Spectrogram* Spectrogram::copy()
{
  Spectrogram *new_s;
  Matrix * m=matrix();
  if(m)new_s= new Spectrogram(matrix());
  else new_s=new Spectrogram(d_funct,m_nRows,m_nColumns,boundingRect(),data().range().minValue(),data().range().maxValue());

  new_s->setDisplayMode(QwtPlotSpectrogram::ImageMode, testDisplayMode(QwtPlotSpectrogram::ImageMode));
  new_s->setDisplayMode(QwtPlotSpectrogram::ContourMode, testDisplayMode(QwtPlotSpectrogram::ContourMode));
  new_s->color_map_policy = color_map_policy;
  if (new_s->color_map_policy == GrayScale)
    new_s->setGrayScale();
  else new_s->setCustomColorMap(new_s->getColorMap());

  new_s->setAxis(xAxis(), yAxis());
  new_s->setDefaultContourPen(defaultContourPen());
  new_s->setLevelsNumber(levels());

  new_s->mutableColorMap().changeScaleType(getColorMap().getScaleType());
  return new_s;
}

void Spectrogram::setGrayScale()
{
  color_map = QwtLinearColorMap(Qt::black, Qt::white);
  setColorMap(color_map);
  //setColorMap(mColorMap);
  color_map_policy = GrayScale;

  QwtPlot *plot = this->plot();
  if (!plot)
    return;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  if (colorAxis)
  {
    colorAxis->setColorMap(data().range(), colorMap());
  }
}

void Spectrogram::setDefaultColorMap()
{
  //color_map = defaultColorMap();
  //setColorMap(color_map);
  setColorMap(mColorMap);
  color_map_policy = Default;

  QwtPlot *plot = this->plot();
  if (!plot)
    return;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  if (colorAxis)
    colorAxis->setColorMap(this->data().range(), this->colorMap());

}
void Spectrogram::setCustomColorMap(const QwtColorMap &map)
{
  setColorMap(map);
  //color_map = map;
  color_map_policy = Custom;
  QwtPlot *plot = this->plot();
  if (!plot)
    return;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  if (colorAxis)
  {
    colorAxis->setColorMap(this->data().range(), this->getColorMap());
  }
}
void Spectrogram::setCustomColorMap(const QwtLinearColorMap& map)
{
  setColorMap(map);
  color_map = map;
  color_map_policy = Custom;

  QwtPlot *plot = this->plot();
  if (!plot)
    return;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  if (colorAxis)
  {
    colorAxis->setColorMap(this->data().range(), this->colorMap());
  }
}

QwtLinearColorMap Spectrogram::defaultColorMap()
{
  QwtLinearColorMap colorMap(Qt::blue, Qt::red);
  colorMap.addColorStop(0.25, Qt::cyan);
  colorMap.addColorStop(0.5, Qt::green);
  colorMap.addColorStop(0.75, Qt::yellow);
  return colorMap;
}
void Spectrogram::setColorMapPen(bool on)
{
  if (d_color_map_pen == on)
    return;

  d_color_map_pen = on;
  if (on) {
    setDefaultContourPen(Qt::NoPen);
    d_pen_list.clear();
  }
}
/**
for creating contour line labels
 */
void Spectrogram::createLabels()
{
  foreach(QwtPlotMarker *m, d_labels_list){
    m->detach();
    delete m;
  }
  d_labels_list.clear();
  QwtValueList levels = contourLevels();
  const int numLevels = levels.size();
  for (int l = 0; l < numLevels; l++){
    PlotMarker *m = new PlotMarker(l, d_labels_angle);
    QwtText t = QwtText(QString::number(levels[l]));
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

    QwtPlot *d_plot = plot();
    if (!d_plot)
      return;
    if (d_plot && d_show_labels)
    {	m->attach(d_plot);
    }
    d_labels_list << m;
  }
}
void Spectrogram::showContourLineLabels(bool show)
{
  if (show == d_show_labels)
  {
    return;
  }
  d_show_labels = show;
  QwtPlot *d_plot = plot();
  if (!d_plot)
    return;
  foreach(PlotMarker *m, d_labels_list){
    if (d_show_labels)
    {	m->attach(d_plot);
    }
    else
      m->detach();
  }
}
bool Spectrogram::hasSelectedLabels()
{
  /*if (d_labels_list.isEmpty())
        return false;

    foreach(PlotMarker *m, d_labels_list){
        if (m->label().backgroundPen() == QPen(Qt::blue))
            return true;
        else
            return false;
    }
    return false;*/

  if (d_selected_label)
    return true;
  return false;
}
void Spectrogram::selectLabel(bool on)
{
  QwtPlot *d_plot = plot();
  if (!d_plot)
    return;
  if (on){
    //d_plot->deselect();
    //d_plot->notifyFontChange(d_labels_font);
  }

  foreach(PlotMarker *m, d_labels_list){
    QwtText t = m->label();
    if(t.text().isEmpty())
      return;

    if (d_selected_label && m == d_selected_label && on)
      t.setBackgroundPen(QPen(Qt::blue));
    else
      t.setBackgroundPen(QPen(Qt::NoPen));

    m->setLabel(t);
  }
  d_plot->replot();
}


bool Spectrogram::selectedLabels(const QPoint& pos)
{
  d_selected_label = NULL;
  QwtPlot *d_plot = plot();
  if (!d_plot)
    return false;

  /*if (d_plot->hasActiveTool())
		return false;*/
  foreach(PlotMarker *m, d_labels_list){
    int x = d_plot->transform(xAxis(), m->xValue());
    int y = d_plot->transform(yAxis(), m->yValue());

    QMatrix wm;
    wm.translate(x, y);
    wm.rotate(-d_labels_angle);
    if (wm.mapToPolygon(QRect(QPoint(0, 0), m->label().textSize())).containsPoint(pos, Qt::OddEvenFill)){
      d_selected_label = m;
      d_click_pos_x = d_plot->invTransform(xAxis(), pos.x());
      d_click_pos_y = d_plot->invTransform(yAxis(), pos.y());
      selectLabel(true);
      return true;
    }
  }
  return false;
}

QString Spectrogram::saveToString()
{
  QString s = "<spectrogram>\n";
  //s += "\t<matrix>" + QString(d_matrix->objectName()) + "</matrix>\n";

  if (color_map_policy != Custom)
    s += "\t<ColorPolicy>" + QString::number(color_map_policy) + "</ColorPolicy>\n";
  else
  {
    s += "\t<ColorMap>\n";
    if(!mCurrentColorMap.isEmpty())s+="\t\tFileName\t"+mCurrentColorMap+"\n";
    s += "\t\t<Mode>" + QString::number(color_map.mode()) + "</Mode>\n";
    s += "\t\t<MinColor>" + color_map.color1().name() + "</MinColor>\n";
    s += "\t\t<MaxColor>" + color_map.color2().name() + "</MaxColor>\n";
    QwtArray <double> colors = color_map.colorStops();
    int stops = (int)colors.size();
    s += "\t\t<ColorStops>" + QString::number(stops - 2) + "</ColorStops>\n";
    for (int i = 1; i < stops - 1; i++)
    {
      s += "\t\t<Stop>" + QString::number(colors[i]) + "\t";
      s += QColor(color_map.rgb(QwtDoubleInterval(0,1), colors[i])).name();
      s += "</Stop>\n";
    }
    s += "\t</ColorMap>\n";
  }
  s += "\t<Image>"+QString::number(testDisplayMode(QwtPlotSpectrogram::ImageMode))+"</Image>\n";

  bool contourLines = testDisplayMode(QwtPlotSpectrogram::ContourMode);
  s += "\t<ContourLines>"+QString::number(contourLines)+"</ContourLines>\n";
  if (contourLines)
  {
    s += "\t\t<Levels>"+QString::number(levels())+"</Levels>\n";
    bool defaultPen = defaultContourPen().style() != Qt::NoPen;
    if (defaultPen)
    {s += "\t\t<DefaultPen>"+QString::number(defaultPen)+"</DefaultPen>\n";
    s += "\t\t\t<PenColor>"+defaultContourPen().color().name()+"</PenColor>\n";
    s += "\t\t\t<PenWidth>"+QString::number(defaultContourPen().widthF())+"</PenWidth>\n";
    s += "\t\t\t<PenStyle>"+QString::number(defaultContourPen().style() - 1)+"</PenStyle>\n";
    }
    else if(!d_color_map_pen)
    {s += "\t\t<CustomPen>"+QString::number(!defaultPen)+"</CustomPen>\n";
    }
    else
      s += "\t\t<ColormapPen>"+QString::number(!defaultPen)+"</ColormapPen>\n";
  }
  QwtScaleWidget *colorAxis = plot()->axisWidget(color_axis);
  if (colorAxis && colorAxis->isColorBarEnabled())
  {
    s += "\t<ColorBar>\n\t\t<axis>" + QString::number(color_axis) + "</axis>\n";
    s += "\t\t<width>" + QString::number(colorAxis->colorBarWidth()) + "</width>\n";
    s += "\t</ColorBar>\n";
  }
  s += "\t<Visible>"+ QString::number(isVisible()) + "</Visible>\n";
  s+="\t<IntensityChanged>"+QString::number(isIntensityChanged())+"</IntensityChanged>\n";
  return s+"</spectrogram>\n";
}
/**
 * Returns a reference to the constant colormap
 */
const MantidColorMap & Spectrogram::getColorMap() const
{

  return mColorMap;

}
void Spectrogram::setMantidColorMap(const MantidColorMap &map)
{
  setColorMap(map);
}
/**
 * Returns a reference to the colormap
 */
MantidColorMap & Spectrogram::mutableColorMap()
{
  return mColorMap;
}
/**
 * Save properties of the window a persistent store
 */
void Spectrogram::saveSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/2DPlotSpectrogram");
  //settings.setValue("BackgroundColor", mInstrumentDisplay->currentBackgroundColor());
  settings.setValue("ColormapFile", mCurrentColorMap);
  settings.setValue("ScaleType",getColorMap().getScaleType());
  settings.endGroup();
}
/**
 * This method loads the setting from QSettings
 */
void Spectrogram::loadSettings()
{
  //Load Color
  QSettings settings;
  settings.beginGroup("Mantid/2DPlotSpectrogram");

  //Load Colormap. If the file is invalid the default stored colour map is used
  mCurrentColorMap = settings.value("ColormapFile", "").toString();
  // Set values from settings
  mutableColorMap().loadMap(mCurrentColorMap);

  GraphOptions::ScaleType type = (GraphOptions::ScaleType)settings.value("ScaleType", GraphOptions::Log10).toUInt();

  mutableColorMap().changeScaleType(type);

  settings.endGroup();
}
/**
 * This method saves the selectcolrmap file name to membervaraible
 */
void Spectrogram::setColorMapFileName(QString colormapName)
{
  mCurrentColorMap=colormapName;
}
QwtDoubleRect Spectrogram::boundingRect() const
{
  return d_matrix?d_matrix->boundingRect() : data().boundingRect();
}

double Spectrogram::getMinPositiveValue()const
{
  //const QwtRasterData* r = &data();
  const SpectrogramData* d = dynamic_cast<const SpectrogramData*>(&data());
  return d ? d->getMinPositiveValue() : 1e-10;
}

void Spectrogram::setContourPenList(QList<QPen> lst)
{
  d_pen_list = lst;
  setDefaultContourPen(Qt::NoPen);
  d_color_map_pen = false;
}

double MatrixData::value(double x, double y) const
{		
  x += 0.5*dx;
  y -= 0.5*dy;

  int i = abs((y - y_start)/dy);
  int j = abs((x - x_start)/dx);

  if (d_m && i >= 0 && i < n_rows && j >=0 && j < n_cols)
    return d_m[i][j];
  else
    return 0.0;
}

double MatrixData::getMinPositiveValue()const
{
  double zmin = DBL_MAX;
  for(int i=0;i<n_rows;++i)
  {
    for(int j=0;i<n_cols;++j)
    {
      double tmp = d_m[i][j];
      if (tmp > 0 && tmp < zmin)
      {
        zmin = tmp;
      }
    }
  }
  return zmin;
}

void Spectrogram::setLabelsRotation(double angle)
{
  if (angle == d_labels_angle)
    return;

  d_labels_angle = angle;

  foreach(PlotMarker *m, d_labels_list)
  m->setAngle(angle);
}

void Spectrogram::setLabelsOffset(double x, double y)
{
  if (x == d_labels_x_offset && y == d_labels_y_offset)
    return;

  d_labels_x_offset = x;
  d_labels_y_offset = y;
}

void Spectrogram::setLabelOffset(int index, double x, double y)
{
  if (index < 0 || index >= d_labels_list.size())
    return;

  PlotMarker *m = d_labels_list[index];
  if (!m)
    return;

  m->setLabelOffset(x, y);
}

void Spectrogram::setLabelsWhiteOut(bool whiteOut)
{
  if (whiteOut == d_white_out_labels)
    return;

  d_white_out_labels = whiteOut;

  foreach(QwtPlotMarker *m, d_labels_list){
    QwtText t = m->label();
    if (whiteOut)
      t.setBackgroundBrush(QBrush(Qt::white));
    else
      t.setBackgroundBrush(QBrush(Qt::transparent));
    m->setLabel(t);
  }
}
void Spectrogram::drawContourLines (QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QwtRasterData::ContourLines &contourLines) const
{

  //QwtPlotSpectrogram::drawContourLines(p, xMap, yMap, contourLines);
  QwtValueList levels = contourLevels();
  const int numLevels = (int)levels.size();
  for (int l = 0; l < numLevels; l++){
    const double level = levels[l];

    QPen pen = defaultContourPen();
    if ( pen.style() == Qt::NoPen )
      pen = contourPen(level);

    if ( pen.style() == Qt::NoPen )
      continue;

    p->setPen(pen);

    const QPolygonF &lines = contourLines[level];
    for ( int i = 0; i < (int)lines.size(); i += 2 ){
      const QPointF p1( xMap.xTransform(lines[i].x()),
          yMap.transform(lines[i].y()) );
      const QPointF p2( xMap.xTransform(lines[i + 1].x()),
          yMap.transform(lines[i + 1].y()) );

      p->drawLine(p1, p2);
    }
  }

  if (d_show_labels)
    updateLabels(p, xMap, yMap, contourLines);
}

void Spectrogram::updateLabels(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QwtRasterData::ContourLines &contourLines) const
{
  (void) p; //Avoid compiler warning
  (void) xMap; //Avoid compiler warning
  (void) yMap; //Avoid compiler warning

  QwtPlot *d_plot = plot();
  if (!d_plot)
    return;

  QwtValueList levels = contourLevels();
  const int numLevels = levels.size();
  int x_axis = xAxis();
  int y_axis = yAxis();
  for (int l = 0; l < numLevels; l++){
    const double level = levels[l];
    const QPolygonF &lines = contourLines[level];
    int i = (int)lines.size()/2;

    PlotMarker *mrk = d_labels_list[l];
    if (!mrk)
      return;
    QSize size = mrk->label().textSize();
    int dx =int((d_labels_x_offset )*0.01*size.height()); //int((d_labels_x_offset + mrk->xLabelOffset())*0.01*size.height());
    int dy = -int(((d_labels_y_offset )*0.01 + 0.5)*size.height());

    double x = lines[i].x();
    double y = lines[i].y();
    int x2 = d_plot->transform(x_axis, x) + dx;
    int y2 = d_plot->transform(y_axis, y) + dy;

    mrk->setValue(d_plot->invTransform(x_axis, x2),
        d_plot->invTransform(y_axis, y2));
  }

}
/**
     for setting the lables color on contour lines
 */
void Spectrogram::setLabelsColor(const QColor& c)
{
  if (c == d_labels_color)
    return;

  d_labels_color = c;

  foreach(QwtPlotMarker *m, d_labels_list){
    QwtText t = m->label();
    t.setColor(c);
    m->setLabel(t);
  }
}
/**
changes the intensity of the colors
 */
void Spectrogram::changeIntensity( double start,double end)
{
  setData(FunctionData(d_funct,m_nRows,m_nColumns,boundingRect(),start,end));
}
/**
 sets the flag for intensity changes
 */
void Spectrogram::setIntensityChange(bool on)
{
  m_bIntensityChanged=on;
}
/**
returns true if intensity(minz and maxz) changed
 */
bool Spectrogram::isIntensityChanged()
{ return m_bIntensityChanged;
}

#include "Mantid/MantidMatrix.h"

/**
 * Override QwtPlotSpectrogram::renderImage to draw ragged spectrograms. It is almost
 * a copy of QwtPlotSpectrogram::renderImage except that pixels of the image that are 
 * outside the boundaries of the histograms are set to a special colour (white).
 */
QImage Spectrogram::renderImage(
    const QwtScaleMap &xMap, const QwtScaleMap &yMap, 
    const QwtDoubleRect &area) const
{

  MantidMatrixFunction* mantidFun = dynamic_cast<MantidMatrixFunction*>(d_funct);
  if (!mantidFun)
  {
    return QwtPlotSpectrogram::renderImage(xMap,yMap,area);
  }

  if ( area.isEmpty() )
    return QImage();

  QRect rect = transform(xMap, yMap, area);

  QImage image(rect.size(), this->colorMap().format() == QwtColorMap::RGB
      ? QImage::Format_ARGB32 : QImage::Format_Indexed8 );

  const QwtDoubleInterval intensityRange = data().range();
  if ( !intensityRange.isValid() )
    return image;

  if ( this->colorMap().format() == QwtColorMap::RGB )
  {
    return QwtPlotSpectrogram::renderImage(xMap,yMap,area);
  }
  else if ( this->colorMap().format() == QwtColorMap::Indexed )
  {
    // Modify the colour table so that the last colour is white and transparent
    // which will indicate no value
    QVector<QRgb> ctable = this->colorMap().colorTable(intensityRange);
    ctable[255] = qRgba(255,255,255,0);
    image.setColorTable(ctable);

    image.fill(255);

    // image2matrix_yMap[image_row] ->  matrix_row or -1
    std::vector<int> image2matrix_yMap(rect.height(),-1);

    for(int row = 0;row<mantidFun->numRows();++row)
    {
      double ymin,ymax;
      mantidFun->getRowYRange(row,ymin,ymax);
      int imax = yMap.transform(ymin)-rect.top(); // image row corresponding to ymin
      int imin = yMap.transform(ymax)-rect.top(); // image row corresponding to ymax
      if (imin < 0)
      {
        if (imax < 0) break;
        else
        {
          imin = 0;
        }
      }
      if (imax > rect.height()-1)
      {
        if (imin > rect.height()-1)
        {
          continue;
        }
        else
        {
          imax = rect.height()-1;
        }
      }
      std::fill(image2matrix_yMap.begin()+imin,image2matrix_yMap.begin()+imax+1,row);

    }

    int imageWidth = rect.width();
    int row0 = -2;
    for(int i=0; i < static_cast<int>(image2matrix_yMap.size()) ;++i)
    {
      int row = image2matrix_yMap[i];
      if (row < 0)
      {
        continue;
      }
      if (row == row0)
      {
        unsigned char *line = image.scanLine(i);
        unsigned char *line0 = image.scanLine(i-1);
        std::copy(line0,line0+imageWidth,line);
        continue;
      }
      row0 = row;

      double xmin,xmax;
      mantidFun->getRowXRange(row,xmin,xmax);
      int jmin = -1;
      if (xmin != std::numeric_limits<double>::infinity() && xmin == xmin &&
          xmax != std::numeric_limits<double>::infinity() && xmax == xmax)
      {
        jmin = xMap.transform(xmin)-rect.left();
      }
      else
      {
        continue;
      }
      if (jmin < 0) jmin = 0;

      unsigned char *line = image.scanLine(i)+jmin;
      const Mantid::MantidVec& X = mantidFun->getMantidVec(row);
      int col = 0;
      int nX = X.size()-1;
      for(int j=jmin;j<imageWidth;++j)
      {
        double x = xMap.invTransform(j+rect.left());
        double x1 = X[col+1];
        while(x1 < x)
        {
          ++col;
          if (col >= nX) break;
          x1 = X[col+1];
        }
        if (col >= nX) break;
        double val = mantidFun->value(row,col);
        *line++ = this->colorMap().colorIndex(intensityRange,val);
      }

    }
  } // QwtColorMap::Indexed

  // Mirror the image in case of inverted maps

  const bool hInvert = xMap.p1() > xMap.p2();
  const bool vInvert = yMap.p1() < yMap.p2();
  if ( hInvert || vInvert )
  {
#ifdef __GNUC__
#warning Better invert the for loops above
#endif
#if QT_VERSION < 0x040000
    image = image.mirror(hInvert, vInvert);
#else
    image = image.mirrored(hInvert, vInvert);
#endif
  }

  return image;
}

