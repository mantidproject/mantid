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
#include <qwt_scale_engine.h>
#include <QPainter>
#include <qwt_symbol.h>

#include "Mantid/MantidMatrix.h"
#include "Mantid/MantidMatrixFunction.h"
#include "MantidAPI/IMDIterator.h"

#include "MantidQtAPI/PlotAxis.h"
#include "MantidQtAPI/QwtRasterDataMD.h"
#include "MantidQtAPI/SignalRange.h"

#include "TSVSerialiser.h"

#include <iostream>
#include <numeric>

Spectrogram::Spectrogram():
      QObject(), QwtPlotSpectrogram(),
			d_color_map_pen(false),
			d_matrix(0),d_funct(0),d_wsData(0), d_wsName(),
	    color_axis(QwtPlot::yRight),
	    color_map_policy(Default),
			color_map(QwtLinearColorMap()),
			d_show_labels(true),
			d_white_out_labels(false),
			d_labels_angle(0.0),
			d_selected_label(NULL),
			d_click_pos_x(0.),
			d_click_pos_y(0.),
			d_labels_x_offset(0),
			d_labels_y_offset(0),
			d_labels_align(Qt::AlignHCenter),
			m_nRows(0),
			m_nColumns(0),
			m_bIntensityChanged(false)
{
}

Spectrogram::Spectrogram(const QString & wsName, const Mantid::API::IMDWorkspace_const_sptr &workspace) :
      QObject(), QwtPlotSpectrogram(),
      d_matrix(NULL),d_funct(NULL),d_wsData(NULL), d_wsName(),
      color_axis(QwtPlot::yRight),
      color_map_policy(Default),mColorMap()
{
  d_wsData = dataFromWorkspace(workspace);
  setData(*d_wsData);
  d_wsName = wsName.toStdString();

  double step = fabs(data().range().maxValue() - data().range().minValue())/5.0;
  QwtValueList contourLevels;
  for ( double level = data().range().minValue() + step;
      level < data().range().maxValue(); level += step )
    contourLevels += level;

  setContourLevels(contourLevels);

  observePostDelete();
  observeADSClear();
  observeAfterReplace();
}

Spectrogram::Spectrogram(Matrix *m):
			QObject(), QwtPlotSpectrogram(QString(m->objectName())),
			d_matrix(m),d_funct(0),d_wsData(NULL), d_wsName(),
			color_axis(QwtPlot::yRight),
			color_map_policy(Default),mColorMap()
{
  setData(MatrixData(m));
  double step = fabs(data().range().maxValue() - data().range().minValue())/5.0;

  QwtValueList contourLevels;
  for ( double level = data().range().minValue() + step;
      level < data().range().maxValue(); level += step )
    contourLevels += level;

  setContourLevels(contourLevels);
}


Spectrogram::Spectrogram(Function2D *f,int nrows, int ncols,double left, double top, double width, double height,double minz,double maxz)
:	QObject(), QwtPlotSpectrogram(),
  d_matrix(0),d_funct(f), d_wsData(NULL), d_wsName(),
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

Spectrogram::Spectrogram(Function2D *f,int nrows, int ncols,QwtDoubleRect bRect,double minz,double maxz)
:	QObject(), QwtPlotSpectrogram(),
 	d_color_map_pen(false),
 	d_matrix(0),
  d_funct(f),d_wsData(NULL), d_wsName(),
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
  observePostDelete(false);
  observeADSClear(false);
  observeAfterReplace(false);
}

/**
 * Called after a workspace has been deleted
 * @param wsName The name of the workspace that has been deleted
 */
void Spectrogram::postDeleteHandle(const std::string &wsName)
{
  if (wsName == d_wsName)
  {
    observePostDelete(false);
    emit removeMe(this);
  }
}

/**
 * Called after a the ADS has been cleared
 */
void Spectrogram::clearADSHandle()
{
  observeADSClear(false);
  postDeleteHandle(d_wsName);
}

/**
 * @param wsName The name of the workspace that has been replaced
 * @param ws A pointer to the new workspace
 */
void Spectrogram::afterReplaceHandle(const std::string &wsName,
                                     const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if (wsName == d_wsName)
  {
    updateData(boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(ws));
  }
}

void Spectrogram::setContourLevels (const QwtValueList & levels)
{
  QwtPlotSpectrogram::setContourLevels(levels);
  createLabels();
}

void Spectrogram::updateData(Matrix *m)
{
  if (!m || !plot())
    return;

  setData(MatrixData(m));
  postDataUpdate();

}

void Spectrogram::updateData(const Mantid::API::IMDWorkspace_const_sptr &workspace)
{
  if(!workspace || !plot()) return;

  delete d_wsData;
  d_wsData = dataFromWorkspace(workspace);
  setData(*d_wsData);
  postDataUpdate();
}

MantidQt::API::QwtRasterDataMD *Spectrogram::dataFromWorkspace(const Mantid::API::IMDWorkspace_const_sptr &workspace)
{
  auto *wsData = new MantidQt::API::QwtRasterDataMD();
  wsData->setWorkspace(workspace);
  wsData->setFastMode(false);
  wsData->setNormalization(Mantid::API::NoNormalization);
  wsData->setZerosAsNan(false);

  // colour range
  QwtDoubleInterval fullRange = MantidQt::API::SignalRange(*workspace).interval();
  wsData->setRange(fullRange);

  auto dim0 = workspace->getDimension(0);
  auto dim1 = workspace->getDimension(1);
  Mantid::coord_t minX(dim0->getMinimum()), maxX(dim0->getMaximum()),
    minY(dim1->getMinimum()), maxY(dim1->getMaximum());
  Mantid::coord_t dx(dim0->getBinWidth()), dy(dim1->getBinWidth());
  const Mantid::coord_t width = (maxX - minX) + dx;
  const Mantid::coord_t height = (maxY - minY) + dy;
  QwtDoubleRect bounds(minX - 0.5*dx, minY - 0.5*dy,
                       width, height);
  wsData->setBoundingRect(bounds.normalized());
  return wsData;
}

void Spectrogram::postDataUpdate()
{
  auto *plot = this->plot();
  setLevelsNumber(levels());

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  if (colorAxis)
  {
    colorAxis->setColorMap(data().range(), colorMap());
  }

  plot->setAxisScale(color_axis, data().range().minValue(), data().range().maxValue());

  if ( d_wsData )
  {
    auto workspace = d_wsData->getWorkspace();
    if ( workspace )
    {
      using MantidQt::API::PlotAxis;
      plot->setAxisTitle(QwtPlot::xBottom,  PlotAxis(*workspace, 0).title());
      plot->setAxisTitle(QwtPlot::yLeft, PlotAxis(*workspace, 1).title());
    }
  }

  plot->replot();
}

void Spectrogram::setLevelsNumber(int levels)
{
  if (levels <= 0)
    return;

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
  int oldMainAxis = yAxis;
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
  MantidColorMap map = getDefaultColorMap();
  
  mCurrentColorMap = map.getFilePath();
  mColorMap = map;
  setColorMap(map);
      
  color_map_policy = Default;

  QwtPlot *plot = this->plot();
  if (!plot)
    return;

  QwtScaleWidget *colorAxis = plot->axisWidget(color_axis);
  if (colorAxis)
    colorAxis->setColorMap(this->data().range(), this->colorMap());

}


MantidColorMap Spectrogram::getDefaultColorMap()
{
  
  QSettings settings;
  settings.beginGroup("Mantid/2DPlotSpectrogram");
  //Load Colormap. If the file is invalid the default stored colour map is used
  QString lastColormapFile = settings.value("ColormapFile", "").toString();
  settings.endGroup();

  //if the file is not valid you will get the default
  MantidColorMap retColorMap(lastColormapFile,GraphOptions::Linear);

  return retColorMap;
}

void Spectrogram::loadColorMap(const QString& file)
{
  mColorMap.loadMap(file);
  setMantidColorMap(mColorMap);
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

void Spectrogram::setLabelsFont(const QFont& font)
{
  if (font == d_labels_font)
    return;

  d_labels_font = font;

  foreach(QwtPlotMarker *m, d_labels_list)
  {
    QwtText t = m->label();
    t.setFont(font);
    m->setLabel(t);
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

void Spectrogram::setContourLinePen(int index, const QPen &pen)
{
  QwtValueList levels = contourLevels();
  if (index < 0 || index >= levels.size())
    return;

  if (d_pen_list.isEmpty()){
    QPen p = defaultContourPen();
    for (int i = 0; i < levels.size(); i++){
      if (p.style() == Qt::NoPen)
        d_pen_list << contourPen(levels[i]);
      else
        d_pen_list << p;
    }
  }

  d_pen_list[index] = pen;
  setDefaultContourPen(Qt::NoPen);
  d_color_map_pen = false;
}

double MatrixData::value(double x, double y) const
{
  x += 0.5*dx;
  y -= 0.5*dy;

  int i = abs(int((y - y_start)/dy));
  int j = abs(int((x - x_start)/dx));

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

  if (d_labels_list.isEmpty()) return;

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
    if (lines.isEmpty()) continue;
    int i = (int)lines.size()/2;

    PlotMarker *mrk = d_labels_list[l];
    if (!mrk)
      return;
    QSize size = mrk->label().textSize();
    int dx =static_cast<int>((d_labels_x_offset )*0.01*size.height()); //static_cast<int>((d_labels_x_offset + mrk->xLabelOffset())*0.01*size.height());
    int dy = -static_cast<int>(((d_labels_y_offset )*0.01 + 0.5)*size.height());

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
  using namespace MantidQt::API;
  if(d_wsData)
  {
    d_wsData->setRange(QwtDoubleInterval(start,end));
    setData(*d_wsData);
  }
  else
  {
    setData(FunctionData(d_funct,m_nRows,m_nColumns,boundingRect(),start,end));
  }
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

/**
 * Override QwtPlotSpectrogram::renderImage to draw ragged spectrograms. It is almost
 * a copy of QwtPlotSpectrogram::renderImage except that pixels of the image that are
 * outside the boundaries of the histograms are set to a special colour (white).
 */
QImage Spectrogram::renderImage(
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QwtDoubleRect &area) const
{
  // Mantid workspace handled in QwtRasterDataMD
  if(d_wsData) return QwtPlotSpectrogram::renderImage(xMap,yMap,area);

  // Not Mantid function so just use base class
  auto *mantidFun = dynamic_cast<MantidMatrixFunction*>(d_funct);
  if (!mantidFun) return QwtPlotSpectrogram::renderImage(xMap,yMap,area);

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
    ctable.back() = qRgba(255,255,255,0);
    image.setColorTable(ctable);

    image.fill(255);

    // image2matrix_yMap[image_row] ->  matrix_row or -1
    std::vector<int> image2matrix_yMap(rect.height(),-1);

    for(size_t row = 0; row < mantidFun->rows(); ++row)
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
      std::fill(image2matrix_yMap.begin()+imin,image2matrix_yMap.begin()+imax+1,static_cast<int>(row));

    }

    int imageWidth = rect.width();
    int row0 = -2;
    for(size_t i=0; i < image2matrix_yMap.size() ;++i)
    {
      int row = image2matrix_yMap[i];
      if (row < 0)
      {
        continue;
      }
      if (row == row0)
      {
        unsigned char *line = image.scanLine(static_cast<int>(i));
        unsigned char *line0 = image.scanLine(static_cast<int>(i-1));
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

      unsigned char *line = image.scanLine(static_cast<int>(i))+jmin;
      const Mantid::MantidVec& X = mantidFun->getMantidVec(row);
      int col = 0;
      int nX = static_cast<int>(X.size())-1;
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
#if QT_VERSION < 0x040000
    image = image.mirror(hInvert, vInvert);
#else
    image = image.mirrored(hInvert, vInvert);
#endif
  }

  return image;
}

void Spectrogram::loadFromProject(const std::string& lines)
{
  using namespace Mantid::Kernel;

  TSVSerialiser tsv(lines);

  if(tsv.hasSection("ColorPolicy"))
  {
    std::string policyStr = tsv.sections("ColorPolicy").front();
    int policy = 0;
    Strings::convert<int>(policyStr, policy);
    if(policy == GrayScale)
      setGrayScale();
    else if(policy == Default)
      setDefaultColorMap();
  }
  else if(tsv.hasSection("ColorMap"))
  {
    const std::string cmStr = tsv.sections("ColorMap").front();
    TSVSerialiser cm(cmStr);

    std::string filename;
    if(cm.selectLine("FileName"))
        cm >> filename;

    const std::string modeStr = cm.sections("Mode")[0];
    const std::string minColStr = cm.sections("MinColor")[0];
    const std::string maxColStr = cm.sections("MaxColor")[0];
    std::vector<std::string> stopVec = cm.sections("Stop");

    int mode;
    Mantid::Kernel::Strings::convert<int>(modeStr, mode);
    QColor c1(QString::fromStdString(minColStr));
    QColor c2(QString::fromStdString(maxColStr));

    QwtLinearColorMap colorMap(c1, c2);
    colorMap.setMode((QwtLinearColorMap::Mode)mode);

    for(auto it = stopVec.begin(); it != stopVec.end(); ++it)
    {
      std::vector<std::string> stopParts;
      double pos;
      boost::split(stopParts, *it, boost::is_any_of("\t"));
      Mantid::Kernel::Strings::convert<double>(stopParts[0], pos);
      colorMap.addColorStop(pos, QColor(QString::fromStdString(stopParts[1])));
    }

    setCustomColorMap(colorMap);
  }

  if(tsv.hasSection("Image"))
  {
    std::string imgStr = tsv.sections("Image").front();
    int imageMode = 0;
    Strings::convert<int>(imgStr, imageMode);
    setDisplayMode(QwtPlotSpectrogram::ImageMode, (bool)imageMode);
  }

  if(tsv.hasSection("ContourLines"))
  {
    std::string clStr = tsv.sections("ContourLines").front();
    int contours = 0;
    Strings::convert<int>(clStr, contours);
    setDisplayMode(QwtPlotSpectrogram::ContourMode, (bool)contours);
  }

  if(tsv.hasSection("ColorBar"))
  {
    const std::string cbStr = tsv.sections("ColorBar").front();
    TSVSerialiser cb(cbStr);

    std::string axisStr = cb.sections("axis")[0];
    std::string widthStr = cb.sections("width")[0];
    int axis, width;
    Mantid::Kernel::Strings::convert<int>(axisStr, axis);
    Mantid::Kernel::Strings::convert<int>(widthStr, width);

    QwtScaleWidget* colorAxis = plot()->axisWidget(axis);
    if(colorAxis)
    {
      colorAxis->setColorBarWidth(width);
      colorAxis->setColorBarEnabled(true);
    }
  }

  if(tsv.hasSection("Visible"))
  {
    std::string visibleStr = tsv.sections("Visible").front();
    int visible = 1;
    Strings::convert<int>(visibleStr, visible);
    setVisible(visible);
  }

  if(tsv.hasSection("IntensityChanged"))
  {
    std::string intensityChangedStr = tsv.sections("IntensityChanged").front();
    int iC = 0;
    Strings::convert<int>(intensityChangedStr, iC);
    setIntensityChange(iC);
  }
}

std::string Spectrogram::saveToProject()
{
  using namespace Mantid::Kernel;
  TSVSerialiser tsv;
  tsv.writeRaw("<spectrogram>");
  if(!d_wsName.empty())
    tsv.writeLine("workspace") << d_wsName;
  if(d_matrix)
    tsv.writeLine("matrix") << d_matrix->name().toStdString();

  if (color_map_policy != Custom)
    tsv.writeInlineSection("ColorPolicy", Strings::toString<int>(color_map_policy));
  else
  {
    TSVSerialiser cm;
    if(!mCurrentColorMap.isEmpty())
      cm.writeLine("FileName") << mCurrentColorMap.toStdString();
    cm.writeInlineSection("Mode", Strings::toString<int>(color_map.mode()));
    cm.writeInlineSection("MinColor", color_map.color1().name().toStdString());
    cm.writeInlineSection("MaxColor", color_map.color2().name().toStdString());

    QwtArray <double> colors = color_map.colorStops();
    int stops = (int)colors.size();
    cm.writeInlineSection("ColorStops", Strings::toString<int>(stops - 2));
    for(int i = 1; i < stops - 1; i++)
    {
      QString stopStr = QString::number(colors[i]) + "\t";
      stopStr += QColor(color_map.rgb(QwtDoubleInterval(0,1), colors[i])).name();
      cm.writeInlineSection("Stop", stopStr.toStdString());
    }
    tsv.writeSection("ColorMap", cm.outputLines());
  }

  tsv.writeInlineSection("Image", Strings::toString<int>(testDisplayMode(QwtPlotSpectrogram::ImageMode)));

  const bool contourLines = testDisplayMode(QwtPlotSpectrogram::ContourMode);
  tsv.writeInlineSection("ContourLines", contourLines ? "1" : "0");

  QwtScaleWidget *colorAxis = plot()->axisWidget(color_axis);
  if(colorAxis && colorAxis->isColorBarEnabled())
  {
    TSVSerialiser cb;
    cb.writeInlineSection("axis", Strings::toString<int>(color_axis));
    cb.writeInlineSection("width", Strings::toString<int>(colorAxis->colorBarWidth()));
    tsv.writeSection("ColorBar", cb.outputLines());
  }

  tsv.writeInlineSection("Visible", isVisible() ? "1" : "0");
  tsv.writeInlineSection("IntensityChanged", isIntensityChanged() ? "1" : "0");

  tsv.writeRaw("</spectrogram>");
  return tsv.outputLines();
}
