/***************************************************************************
    File                 : Graph.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Graph widget

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
#include <QVarLengthArray>

#include "pixmaps.h"
#include "Graph.h"
#include "Grid.h"
#include "CanvasPicker.h"
#include "QwtErrorPlotCurve.h"
#include "LegendWidget.h"
#include "ArrowMarker.h"
#include "cursors.h"
#include "ScalePicker.h"
#include "TitlePicker.h"
#include "QwtPieCurve.h"
#include "ImageMarker.h"
#include "QwtBarCurve.h"
#include "BoxCurve.h"
#include "QwtHistogram.h"
#include "VectorCurve.h"
#include "ScaleDraw.h"
#include "ColorBox.h"
#include "PatternBox.h"
#include "SymbolBox.h"
#include "FunctionCurve.h"
#include "Spectrogram.h"
#include "SelectionMoveResizer.h"
#include "RangeSelectorTool.h"
#include "PlotCurve.h"
#include "ApplicationWindow.h"
#include "plot2D/ScaleEngine.h"

#include "MantidAPI/AnalysisDataService.h"
#include "Mantid/MantidMatrixCurve.h"
#include "MantidQtAPI/PlotAxis.h"
#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"
#include "Mantid/ErrorBarSettings.h"

#include "TSVSerialiser.h"

#ifdef EMF_OUTPUT
#include "EmfEngine.h"
#endif

#include <QApplication>
#include <QBitmap>
#include <QClipboard>
#include <QCursor>
#include <QImage>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QMenu>
#include <QTextStream>
#include <QLocale>
#include <QPrintDialog>
#include <QImageWriter>
#include <QFileInfo>

#if QT_VERSION >= 0x040300
#include <QSvgGenerator>
#endif

#include <qwt_painter.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_zoomer.h>
#if QWT_VERSION >= 0x050200
#include <qwt_plot_rescaler.h>
#endif
#include <qwt_scale_widget.h>
#include <qwt_scale_engine.h>
#include <qwt_text_label.h>
#include <qwt_color_map.h>

#include <climits>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <iostream>

// We can safely ignore warnings about assuming signed overflow does not occur from qvector.h
// (They really should have implemented it with unsigned types!
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
 #pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

namespace
{
  /// static logger
  Mantid::Kernel::Logger g_log("Graph");
}

Graph::Graph(int x, int y, int width, int height, QWidget* parent, Qt::WFlags f)
: QWidget(parent, f)
{	
  setWindowFlags(f);
  n_curves=0;

  d_waterfall_offset_x = 0;
  d_waterfall_offset_y = 0;

  d_active_tool = NULL;
  d_selected_text = NULL;
  d_legend = NULL; // no legend for an empty graph
  d_peak_fit_tool = NULL;
  d_magnifier = NULL;
  d_panner=NULL;
#if QWT_VERSION >= 0x050200
  d_rescaler = NULL;
#endif

  widthLine=1;
  selectedMarker=-1;
  drawTextOn=false;
  drawLineOn=false;
  drawArrowOn=false;
  ignoreResize = false;
  drawAxesBackbone = true;
  autoScaleFonts = false;
  d_antialiasing = true;
  d_scale_on_print = true;
  d_print_cropmarks = false;
  d_synchronize_scales = false;

  d_user_step = QVector<double>(QwtPlot::axisCnt);
  for (int i=0; i<QwtPlot::axisCnt; i++)
    d_user_step[i] = 0.0;

  setGeometry(x, y, width, height);
  // Mantid
  setAttribute(Qt::WA_DeleteOnClose, false);

  d_plot = new Plot(width, height, this);
  connect (d_plot,SIGNAL(dragMousePress(QPoint)), this, SLOT(slotDragMousePress(QPoint)));
  connect (d_plot,SIGNAL(dragMouseRelease(QPoint)), this, SLOT(slotDragMouseRelease(QPoint)));
  connect (d_plot,SIGNAL(dragMouseMove(QPoint)), this, SLOT(slotDragMouseMove(QPoint)));

  cp = new CanvasPicker(this);

  titlePicker = new TitlePicker(d_plot);
  scalePicker = new ScalePicker(d_plot);

  d_zoomer[0]= new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner, QwtPicker::AlwaysOff, d_plot->canvas());
  d_zoomer[0]->setRubberBandPen(QPen(Qt::black));
  d_zoomer[1] = new QwtPlotZoomer(QwtPlot::xTop, QwtPlot::yRight,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner,
      QwtPicker::AlwaysOff, d_plot->canvas());
  zoom(false);

  c_type = QVector<int>();
  c_keys = QVector<int>();

  setFocusPolicy(Qt::StrongFocus);
  //setFocusProxy(d_plot);
  setMouseTracking(true );


  connect (cp,SIGNAL(selectPlot()),this,SLOT(activateGraph()));
  connect (cp,SIGNAL(viewImageDialog()),this,SIGNAL(viewImageDialog()));
  connect (cp,SIGNAL(viewLineDialog()),this,SIGNAL(viewLineDialog()));
  connect (cp,SIGNAL(showPlotDialog(int)),this,SIGNAL(showPlotDialog(int)));
  connect (cp,SIGNAL(showMarkerPopupMenu()),this,SIGNAL(showMarkerPopupMenu()));
  connect (cp,SIGNAL(modified()), this, SIGNAL(modifiedGraph()));

  connect (titlePicker,SIGNAL(showTitleMenu()),this,SLOT(showTitleContextMenu()));
  connect (titlePicker,SIGNAL(doubleClicked()),this, SLOT(enableTextEditor()));
  connect (titlePicker,SIGNAL(removeTitle()),this,SLOT(removeTitle()));
  connect (titlePicker,SIGNAL(clicked()), this,SLOT(selectTitle()));

  connect (scalePicker,SIGNAL(clicked()),this,SLOT(activateGraph()));
  connect (scalePicker,SIGNAL(clicked()),this,SLOT(deselectMarker()));
  connect (scalePicker,SIGNAL(axisDblClicked(int)),this,SIGNAL(axisDblClicked(int)));
  connect (scalePicker,SIGNAL(axisTitleDblClicked()), this, SLOT(enableTextEditor()));
  connect (scalePicker,SIGNAL(axisTitleRightClicked()),this,SLOT(showAxisTitleMenu()));
  connect (scalePicker,SIGNAL(axisRightClicked(int)),this,SLOT(showAxisContextMenu(int)));

  connect (d_zoomer[0],SIGNAL(zoomed (const QwtDoubleRect &)),this,SLOT(zoomed (const QwtDoubleRect &)));

  m_isDistribution = false;
  m_normalizable = false;
}

void Graph::notifyChanges()
{
  emit modifiedGraph();
}

void Graph::activateGraph()
{
  emit selectedGraph(this);
  setFocus();
}

MultiLayer * Graph::multiLayer()
{
  return dynamic_cast<MultiLayer *>(this->parent()->parent()->parent());
}

void Graph::deselectMarker()
{
  selectedMarker = -1;
  if (d_markers_selector)
    delete d_markers_selector;

  emit enableTextEditor(NULL);

  cp->disableEditing();

  QObjectList lst = d_plot->children();
  foreach(QObject *o, lst) {
    if (o->inherits("LegendWidget")) {
      LegendWidget *lw = dynamic_cast<LegendWidget *>(o);
      if (lw)
        lw->setSelected(false);
    }
  }
}

void Graph::enableTextEditor()
{
  ApplicationWindow *app = multiLayer()->applicationWindow();
  if (!app)
    return;

  if (app->d_in_place_editing)
    emit enableTextEditor(this);
  else if (titlePicker->selected())
    viewTitleDialog();
  else
    showAxisTitleDialog();
}

QList <LegendWidget *> Graph::textsList()
{
  QList <LegendWidget *> texts;
  QObjectList lst = d_plot->children();
  foreach(QObject *o, lst){
    if (o->inherits("LegendWidget"))
      texts << dynamic_cast<LegendWidget *>(o);
  }
  return texts;
}

int Graph::selectedMarkerKey()
{
  return selectedMarker;
}

QwtPlotMarker* Graph::selectedMarkerPtr()
{
  return d_plot->marker(int(selectedMarker));
}

void Graph::setSelectedText(LegendWidget *l)
{
  if (l){
    selectTitle(false);
    scalePicker->deselect();
    deselectCurves();
    emit currentFontChanged(l->font());
  }

  d_selected_text = l;
}

void Graph::setSelectedMarker(int _mrk, bool add)
{
  int mrk = int(_mrk);
  if (mrk >= 0){
    selectTitle(false);
    scalePicker->deselect();
  }

  selectedMarker = mrk;
  if (add) {
    if (d_markers_selector) {
      if (d_lines.contains(mrk))
        d_markers_selector->add(dynamic_cast<ArrowMarker *>(d_plot->marker(mrk)));
      else if (d_images.contains(mrk))
        d_markers_selector->add(dynamic_cast<ImageMarker *>(d_plot->marker(mrk)));
      else
        return;
    } else {
      if (d_lines.contains(mrk))
        d_markers_selector = new SelectionMoveResizer(dynamic_cast<ArrowMarker *>(d_plot->marker(mrk)));
      else if (d_images.contains(mrk))
        d_markers_selector = new SelectionMoveResizer(dynamic_cast<ImageMarker *>(d_plot->marker(mrk)));
      else
        return;

      connect(d_markers_selector, SIGNAL(targetsChanged()), this, SIGNAL(modifiedGraph()));
    }
  } else {
    if (d_lines.contains(mrk)) {
      if (d_markers_selector) {
        if (d_markers_selector->contains(dynamic_cast<ArrowMarker *>(d_plot->marker(mrk))))
          return;
        delete d_markers_selector;
      }
      d_markers_selector = new SelectionMoveResizer(dynamic_cast<ArrowMarker *>(d_plot->marker(mrk)));
    } else if (d_images.contains(mrk)) {
      if (d_markers_selector) {
        if (d_markers_selector->contains(dynamic_cast<ImageMarker *>(d_plot->marker(mrk))))
          return;
        delete d_markers_selector;
      }
      d_markers_selector = new SelectionMoveResizer(dynamic_cast<ImageMarker *>(d_plot->marker(mrk)));
    } else
      return;

    connect(d_markers_selector, SIGNAL(targetsChanged()), this, SIGNAL(modifiedGraph()));
  }
}

void Graph::initFonts(const QFont &scaleTitleFnt, const QFont &numbersFnt)
{
  for (int i = 0;i<QwtPlot::axisCnt;i++)
  {
    d_plot->setAxisFont (i,numbersFnt);
    QwtText t = d_plot->axisTitle (i);
    t.setFont (scaleTitleFnt);
    d_plot->setAxisTitle(i, t);
  }
}

void Graph::setAxisFont(int axis,const QFont &fnt)
{
  d_plot->setAxisFont (axis, fnt);
  d_plot->replot();
  emit modifiedGraph();
}

QFont Graph::axisFont(int axis)
{
  return d_plot->axisFont (axis);
}

void Graph::enableAxis(int axis, bool on)
{
  d_plot->enableAxis(axis, on);
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale)
    scale->setMargin(0);

  scalePicker->refresh();
}

void Graph::setAxisMargin(int axis, int margin)
{
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale)
    scale->setMargin(margin);
}

bool Graph::isColorBarEnabled(int axis) const
{
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale)
  {
    return scale->isColorBarEnabled();
  }
  return false;
}
/** Finds out if the specified axis has a log scale or not
*  @param axis the aixs to check e.g. yright ...
*  @return true if there is a log scale on that axis
*/
bool Graph::isLog(const QwtPlot::Axis& axis) const
{
  ScaleEngine *sc_engine = dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(axis));
  return ( sc_engine && sc_engine->type() == QwtScaleTransformation::Log10 );
}

ScaleDraw::ScaleType Graph::axisType(int axis)
{
  if (!d_plot->axisEnabled(axis))
    return ScaleDraw::Numeric;

  return dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis))->scaleType();
}

void Graph::setLabelsNumericFormat(int axis, int format, int prec, const QString& formula)
{
  ScaleDraw *sd = new ScaleDraw(d_plot, formula.ascii());
  sd->setNumericFormat((ScaleDraw::NumericFormat)format);
  sd->setNumericPrecision(prec);
  sd->setScaleDiv(d_plot->axisScaleDraw(axis)->scaleDiv());
  d_plot->setAxisScaleDraw (axis, sd);
}

void Graph::setLabelsNumericFormat(const QStringList& l)
{
  for (int axis = 0; axis<4; axis++){
    ScaleDraw *sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis));
    if (!sd || !sd->hasComponent(QwtAbstractScaleDraw::Labels))
      continue;

    int aux = 2*axis;
    setLabelsNumericFormat(axis, l[aux].toInt(), l[aux + 1].toInt(), sd->formula());
  }
}

void Graph::enableAxisLabels(int axis, bool on)
{
  QwtScaleWidget *sc = d_plot->axisWidget(axis);
  if (sc){
    QwtScaleDraw *sd = d_plot->axisScaleDraw (axis);
    sd->enableComponent (QwtAbstractScaleDraw::Labels, on);
  }
}

void Graph::setMajorTicksType(const QList<int>& lst)
{
  if (d_plot->getMajorTicksType() == lst)
    return;

  for (int i=0;i<(int)lst.count();i++)
  {
    ScaleDraw *sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw (i));
    if (!sd)
      continue;

    if (lst[i]==ScaleDraw::None || lst[i]==ScaleDraw::In)
      sd->enableComponent (QwtAbstractScaleDraw::Ticks, false);
    else
    {
      sd->enableComponent(QwtAbstractScaleDraw::Ticks);
      sd->setTickLength(QwtScaleDiv::MinorTick, d_plot->minorTickLength());
      sd->setTickLength(QwtScaleDiv::MediumTick, d_plot->minorTickLength());
      sd->setTickLength(QwtScaleDiv::MajorTick, d_plot->majorTickLength());
    }
    sd->setMajorTicksStyle((ScaleDraw::TicksStyle)lst[i]);
  }
}

void Graph::setMajorTicksType(const QStringList& lst)
{
  for (int i=0; i<(int)lst.count(); i++)
    d_plot->setMajorTicksType(i, lst[i].toInt());
}

void Graph::setMinorTicksType(const QList<int>& lst)
{
  if (d_plot->getMinorTicksType() == lst)
    return;

  for (int i=0;i<(int)lst.count();i++)
    d_plot->setMinorTicksType(i, lst[i]);
}

void Graph::setMinorTicksType(const QStringList& lst)
{
  for (int i=0;i<(int)lst.count();i++)
    d_plot->setMinorTicksType(i,lst[i].toInt());
}

int Graph::minorTickLength()
{
  return d_plot->minorTickLength();
}

int Graph::majorTickLength()
{
  return d_plot->majorTickLength();
}

void Graph::setAxisTicksLength(int axis, int majTicksType, int minTicksType,
    int minLength, int majLength)
{
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (!scale)
    return;

  d_plot->setTickLength(minLength, majLength);

  ScaleDraw *sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis));
  sd->setMajorTicksStyle((ScaleDraw::TicksStyle)majTicksType);
  sd->setMinorTicksStyle((ScaleDraw::TicksStyle)minTicksType);

  if (majTicksType == ScaleDraw::None && minTicksType == ScaleDraw::None)
    sd->enableComponent (QwtAbstractScaleDraw::Ticks, false);
  else
    sd->enableComponent (QwtAbstractScaleDraw::Ticks);

  if (majTicksType == ScaleDraw::None || majTicksType == ScaleDraw::In)
    majLength = minLength;
  if (minTicksType == ScaleDraw::None || minTicksType == ScaleDraw::In)
    minLength = 0;

  sd->setTickLength (QwtScaleDiv::MinorTick, minLength);
  sd->setTickLength (QwtScaleDiv::MediumTick, minLength);
  sd->setTickLength (QwtScaleDiv::MajorTick, majLength);
}

void Graph::setTicksLength(int minLength, int majLength)
{
  QList<int> majTicksType = d_plot->getMajorTicksType();
  QList<int> minTicksType = d_plot->getMinorTicksType();

  for (int i=0; i<4; i++)
    setAxisTicksLength (i, majTicksType[i], minTicksType[i], minLength, majLength);
}

void Graph::changeTicksLength(int minLength, int majLength)
{
  if (d_plot->minorTickLength() == minLength &&
      d_plot->majorTickLength() == majLength)
    return;

  setTicksLength(minLength, majLength);

  d_plot->hide();
  for (int i=0; i<4; i++)
  {
    if (d_plot->axisEnabled(i))
    {
      d_plot->enableAxis (i,false);
      d_plot->enableAxis (i,true);
    }
  }
  d_plot->replot();
  d_plot->show();

  emit modifiedGraph();
}

void Graph::showAxis(int axis, int type, const QString& formatInfo, Table *table,
    bool axisOn, int majTicksType, int minTicksType, bool labelsOn,
    const QColor& c,  int format, int prec, int rotation, int baselineDist,
    const QString& formula, const QColor& labelsColor)
{
  d_plot->enableAxis(axis, axisOn);
  if (!axisOn)
    return;

  QList<int> majTicksTypeList = d_plot->getMajorTicksType();
  QList<int> minTicksTypeList = d_plot->getMinorTicksType();

  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  ScaleDraw *sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis));

  if (d_plot->axisEnabled (axis) == axisOn &&
      majTicksTypeList[axis] == majTicksType &&
      minTicksTypeList[axis] == minTicksType &&
      axisColor(axis) == c &&
      axisLabelsColor(axis) == labelsColor &&
      prec == d_plot->axisLabelPrecision (axis) &&
      format == d_plot->axisLabelFormat (axis) &&
      labelsRotation(axis) == rotation &&
      (int)sd->scaleType() == type &&
      sd->formatString() == formatInfo &&
      sd->formula() == formula &&
      scale->margin() == baselineDist &&
      sd->hasComponent (QwtAbstractScaleDraw::Labels) == labelsOn)
    return;

  scale->setMargin(baselineDist);
  QPalette pal = scale->palette();
  if (pal.color(QPalette::Active, QColorGroup::Foreground) != c)
    pal.setColor(QColorGroup::Foreground, c);
  if (pal.color(QPalette::Active, QColorGroup::Text) != labelsColor)
    pal.setColor(QColorGroup::Text, labelsColor);
  scale->setPalette(pal);

  if (!labelsOn)
    sd->enableComponent (QwtAbstractScaleDraw::Labels, false);
  else {
    if (type == ScaleDraw::Numeric)
      setLabelsNumericFormat(axis, format, prec, formula);
    else if (type == ScaleDraw::Day)
      setLabelsDayFormat (axis, format);
    else if (type == ScaleDraw::Month)
      setLabelsMonthFormat (axis, format);
    else if (type == ScaleDraw::Time || type == ScaleDraw::Date)
      setLabelsDateTimeFormat (axis, type, formatInfo);
    else
      setLabelsTextFormat(axis, type, formatInfo, table);

    setAxisLabelRotation(axis, rotation);
  }

  sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis));
  sd->enableComponent(QwtAbstractScaleDraw::Backbone, drawAxesBackbone);

  setAxisTicksLength(axis, majTicksType, minTicksType,
      d_plot->minorTickLength(), d_plot->majorTickLength());

  if (d_synchronize_scales && axisOn && (axis == QwtPlot::xTop || axis == QwtPlot::yRight))
  {
    updateSecondaryAxis(axis);//synchronize scale divisions
  }

  scalePicker->refresh();
  d_plot->updateLayout();	//This is necessary in order to enable/disable tick labels
  scale->repaint();
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::setLabelsDayFormat(int axis, int format)
{
  ScaleDraw *sd = new ScaleDraw(d_plot);
  sd->setDayFormat((ScaleDraw::NameFormat)format);
  sd->setScaleDiv(d_plot->axisScaleDraw(axis)->scaleDiv());
  d_plot->setAxisScaleDraw (axis, sd);
}

void Graph::setLabelsMonthFormat(int axis, int format)
{
  ScaleDraw *sd = new ScaleDraw(d_plot);
  sd->setMonthFormat((ScaleDraw::NameFormat)format);
  sd->setScaleDiv(d_plot->axisScaleDraw(axis)->scaleDiv());
  d_plot->setAxisScaleDraw (axis, sd);
}

void Graph::setLabelsTextFormat(int axis, int type, const QString& name, const QStringList& lst)
{
  if (type != ScaleDraw::Text && type != ScaleDraw::ColHeader)
    return;

  d_plot->setAxisScaleDraw(axis, new ScaleDraw(d_plot, lst, name, (ScaleDraw::ScaleType)type));
}

void Graph::setLabelsTextFormat(int axis, int type, const QString& labelsColName, Table *table)
{
  if (type != ScaleDraw::Text && type != ScaleDraw::ColHeader)
    return;

  QStringList list = QStringList();
  if (type == ScaleDraw::Text){
    if (!table)
      return;

    int r = table->numRows();
    int col = table->colIndex(labelsColName);
    for (int i=0; i < r; i++){
      QString s = table->text(i, col);
      if (!s.isEmpty())
        list << s;
    }
    d_plot->setAxisScaleDraw(axis, new ScaleDraw(d_plot, list, labelsColName, ScaleDraw::Text));
  } else if (type == ScaleDraw::ColHeader) {
    if (!table)
      return;

    for (int i=0; i<table->numCols(); i++){
      if (table->colPlotDesignation(i) == Table::Y)
        list << table->colLabel(i);
    }
    d_plot->setAxisScaleDraw(axis, new ScaleDraw(d_plot, list, table->objectName(), ScaleDraw::ColHeader));
  }
}

void Graph::setLabelsDateTimeFormat(int axis, int type, const QString& formatInfo)
{
  if (type < ScaleDraw::Time)
    return;

  QStringList list = formatInfo.split(";", QString::KeepEmptyParts);
  if ((int)list.count() < 2)
  {
    QMessageBox::critical(this, tr("MantidPlot - Error"),
        tr("Couldn't change the axis type to the requested format!"));
    return;
  }
  if (list[0].isEmpty() || list[1].isEmpty())
  {
    QMessageBox::critical(this, tr("MantidPlot - Error"),
        tr("Couldn't change the axis type to the requested format!"));
    return;
  }

  if (type == ScaleDraw::Time)
  {
    ScaleDraw *sd = new ScaleDraw(d_plot);
    sd->setTimeFormat(QTime::fromString (list[0]), list[1]);
    sd->enableComponent (QwtAbstractScaleDraw::Backbone, drawAxesBackbone);
    d_plot->setAxisScaleDraw (axis, sd);
  }
  else if (type == ScaleDraw::Date)
  {
    ScaleDraw *sd = new ScaleDraw(d_plot);
    sd->setDateFormat(QDateTime::fromString (list[0], Qt::ISODate), list[1]);
    sd->enableComponent (QwtAbstractScaleDraw::Backbone, drawAxesBackbone);
    d_plot->setAxisScaleDraw (axis, sd);
  }
}

void Graph::setAxisLabelRotation(int axis, int rotation)
{
  if (axis==QwtPlot::xBottom)
  {
    if (rotation > 0)
      d_plot->setAxisLabelAlignment(axis, Qt::AlignRight|Qt::AlignVCenter);
    else if (rotation < 0)
      d_plot->setAxisLabelAlignment(axis, Qt::AlignLeft|Qt::AlignVCenter);
    else if (rotation == 0)
      d_plot->setAxisLabelAlignment(axis, Qt::AlignHCenter|Qt::AlignBottom);
  }
  else if (axis==QwtPlot::xTop)
  {
    if (rotation > 0)
      d_plot->setAxisLabelAlignment(axis, Qt::AlignLeft|Qt::AlignVCenter);
    else if (rotation < 0)
      d_plot->setAxisLabelAlignment(axis, Qt::AlignRight|Qt::AlignVCenter);
    else if (rotation == 0)
      d_plot->setAxisLabelAlignment(axis, Qt::AlignHCenter|Qt::AlignTop);
  }
  d_plot->setAxisLabelRotation (axis, (double)rotation);
}

int Graph::labelsRotation(int axis)
{
  ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis));
  return (int)sclDraw->labelRotation();
}

void Graph::setAxisTitleFont(int axis,const QFont &fnt)
{
  QwtText t = d_plot->axisTitle (axis);
  t.setFont (fnt);
  d_plot->setAxisTitle(axis, t);
  d_plot->replot();
  emit modifiedGraph();
}

QFont Graph::axisTitleFont(int axis)
{
  return d_plot->axisTitle(axis).font();
}

QColor Graph::axisTitleColor(int axis)
{
  QColor c;
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale)
    c = scale->title().color();
  return c;
}

void Graph::setAxisLabelsColor(int axis, const QColor& color)
{
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale){
    QPalette pal = scale->palette();
    pal.setColor(QColorGroup::Text, color);
    scale->setPalette(pal);
  }
}

void Graph::setAxisColor(int axis, const QColor& color)
{
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale){
    QPalette pal = scale->palette();
    pal.setColor(QColorGroup::Foreground, color);
    scale->setPalette(pal);
  }
}

QColor Graph::axisColor(int axis)
{
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale)
    return scale->palette().color(QPalette::Active, QColorGroup::Foreground);
  else
    return QColor(Qt::black);
}

QColor Graph::axisLabelsColor(int axis)
{
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale)
    return scale->palette().color(QPalette::Active, QColorGroup::Text);
  else
    return QColor(Qt::black);
}

void Graph::setTitleColor(const QColor & c)
{
  QwtText t = d_plot->title();
  t.setColor(c);
  d_plot->setTitle (t);
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::setTitleAlignment(int align)
{
  QwtText t = d_plot->title();
  t.setRenderFlags(static_cast<int>(align));
  d_plot->setTitle (t);
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::setTitleFont(const QFont &fnt)
{
  QwtText t = d_plot->title();
  t.setFont(fnt);
  d_plot->setTitle (t);
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::setYAxisTitle(const QString& text)
{
  d_plot->setAxisTitle(QwtPlot::yLeft, text);
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::setXAxisTitle(const QString& text)
{
  d_plot->setAxisTitle(QwtPlot::xBottom, text);
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::setRightAxisTitle(const QString& text)
{
  d_plot->setAxisTitle(QwtPlot::yRight, text);
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::setTopAxisTitle(const QString& text)
{
  d_plot->setAxisTitle(QwtPlot::xTop, text);
  d_plot->replot();
  emit modifiedGraph();
}

int Graph::axisTitleAlignment (int axis)
{
  return d_plot->axisTitle(axis).renderFlags();
}

void Graph::setAxisTitleAlignment(int axis, int align)
{
  QwtText t = d_plot->axisTitle(axis);
  t.setRenderFlags(align);
  d_plot->setAxisTitle (axis, t);
}

int Graph::axisTitleDistance(int axis)
{
  if (!d_plot->axisEnabled(axis))
    return 0;

  return d_plot->axisWidget(axis)->spacing();
}

void Graph::setAxisTitleDistance(int axis, int dist)
{
  if (!d_plot->axisEnabled(axis))
    return;

  QwtScaleWidget *scale = d_plot->axisWidget(axis);
  if (scale)
    scale->setSpacing(dist);
}


void Graph::setScaleTitle(int axis, const QString& text)
{
  int a = 0;
  switch (axis)
  {
  case 0:
    a=2;
    break;
  case 1:
    a=0;
    break;
  case 2:
    a=3;
    break;
  case 3:
    a=1;
    break;
  }
  d_plot->setAxisTitle(a, text);
}

void Graph::setAxisTitle(int axis, const QString& text)
{
  if (text.isEmpty())//avoid empty titles due to plot layout behavior
    d_plot->setAxisTitle(axis, " ");
  else
    d_plot->setAxisTitle(axis, text);

  d_plot->replot();
  emit modifiedGraph();
}

void Graph::updateSecondaryAxis(int axis)
{
  for (int i=0; i<n_curves; i++){
    QwtPlotItem *it = plotItem(i);
    if (!it)
      continue;

    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
      Spectrogram *sp = dynamic_cast<Spectrogram *>(it);
      if (sp->colorScaleAxis() == axis)
        return;
    }

    if ((axis == QwtPlot::yRight && it->yAxis() == QwtPlot::yRight) ||
        (axis == QwtPlot::xTop && it->xAxis () == QwtPlot::xTop))
      return;
  }

  int a = QwtPlot::xBottom;
  if (axis == QwtPlot::yRight)
    a = QwtPlot::yLeft;

  if (!d_plot->axisEnabled(a))
    return;

  ScaleEngine *sc_engine = dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(axis));
  sc_engine->clone(dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(a)));

  /*QwtScaleEngine *qwtsc_engine = d_plot->axisScaleEngine(axis);
	ScaleEngine *sc_engine=dynamic_cast<ScaleEngine*>(qwtsc_engine);
	if(sc_engine!=NULL)
	{
		sc_engine->clone(sc_engine);
	}*/

  d_plot->setAxisScaleDiv (axis, *d_plot->axisScaleDiv(a));
  d_user_step[axis] = d_user_step[a];
}

void Graph::enableAutoscaling(bool yes)
{
  for (int i = 0; i < QwtPlot::axisCnt; i++)
  {
    if (yes)
    {
      d_plot->setAxisAutoScale(i);
    }
    else
    {
      // We need this hack due to the fact that in Qwt 5.0 we can't
      // disable autoscaling in an easier way, like for example: setAxisAutoScale(axisId, false)
      d_plot->setAxisScaleDiv(i, *d_plot->axisScaleDiv(i));
    }
  }
}

void Graph::setAutoScale()
{
  enableAutoscaling(true);
  
  updateScale();
  
  for (int i = 0; i < QwtPlot::axisCnt; i++)
  {
    if ( isLog(QwtPlot::Axis(i)) )
    {
      niceLogScales(QwtPlot::Axis(i));
    }
  }
  emit modifiedGraph();
}

void Graph::initScaleLimits()
{//We call this function the first time we add curves to a plot in order to avoid curves with cut symbols.
  d_plot->replot();

  QwtDoubleInterval intv[QwtPlot::axisCnt];
  const QwtPlotItemList& itmList = d_plot->itemList();
  QwtPlotItemIterator it;
  double maxSymbolSize = 0;
  for ( it = itmList.begin(); it != itmList.end(); ++it ){
    const QwtPlotItem *item = *it;
    if (item->rtti() != QwtPlotItem::Rtti_PlotCurve)
      continue;

    const QwtPlotCurve *c = dynamic_cast<const QwtPlotCurve *>(item);
    const QwtSymbol s = c->symbol();
    if (s.style() != QwtSymbol::NoSymbol && s.size().width() >= maxSymbolSize)
      maxSymbolSize = s.size().width();

    const QwtDoubleRect rect = item->boundingRect();
    intv[item->xAxis()] |= QwtDoubleInterval(rect.left(), rect.right());
    intv[item->yAxis()] |= QwtDoubleInterval(rect.top(), rect.bottom());
  }

  if (maxSymbolSize == 0.0)
    return;

  maxSymbolSize *= 0.5;

  QwtScaleDiv *div = d_plot->axisScaleDiv(QwtPlot::xBottom);
  double start = div->lBound();
  double end = div->hBound();
  QwtValueList majTicksLst = div->ticks(QwtScaleDiv::MajorTick);
  int ticks = majTicksLst.size();
  double step = fabs(end - start)/(double)(ticks - 1.0);
  d_user_step[QwtPlot::xBottom] = step;
  d_user_step[QwtPlot::xTop] = step;

  const QwtScaleMap &xMap = d_plot->canvasMap(QwtPlot::xBottom);
  double x_left = xMap.xTransform(intv[QwtPlot::xBottom].minValue());

  if (start >= xMap.invTransform(x_left - maxSymbolSize))
    start = div->lBound() - step;

  double x_right = xMap.xTransform(intv[QwtPlot::xBottom].maxValue());
  if (end <= xMap.invTransform(x_right + maxSymbolSize))
    end = div->hBound() + step;

  d_plot->setAxisScale(QwtPlot::xBottom, start, end, step);
  d_plot->setAxisScale(QwtPlot::xTop, start, end, step);

  div = d_plot->axisScaleDiv(QwtPlot::yLeft);
  start = div->lBound();
  end = div->hBound();
  majTicksLst = div->ticks(QwtScaleDiv::MajorTick);
  ticks = majTicksLst.size();
  step = fabs(end - start)/(double)(ticks - 1.0);
  d_user_step[QwtPlot::yLeft] = step;
  d_user_step[QwtPlot::yRight] = step;

  const QwtScaleMap &yMap = d_plot->canvasMap(QwtPlot::yLeft);
  double y_bottom = yMap.xTransform(intv[QwtPlot::yLeft].minValue());
  if (start >= yMap.invTransform(y_bottom + maxSymbolSize))
    start = div->lBound() - step;

  double y_top = yMap.xTransform(intv[QwtPlot::yLeft].maxValue());
  if (end <= yMap.invTransform(y_top - maxSymbolSize))
    end = div->hBound() + step;

  d_plot->setAxisScale(QwtPlot::yLeft, start, end, step);
  d_plot->setAxisScale(QwtPlot::yRight, start, end, step);
  d_plot->replot();
}

/** Ensure that there are numbers on the log scale
 *  by setting the extreme ends of the scale to major tick
 *  numbers e.g. 1, 10, 100 etc.
 */
void Graph::niceLogScales(QwtPlot::Axis axis)
{
  const QwtScaleDiv *scDiv = d_plot->axisScaleDiv(axis);
  double start = QMIN(scDiv->lBound(), scDiv->hBound());
  double end = QMAX(scDiv->lBound(), scDiv->hBound());

  // log scales can't represent zero or negative values, 1e-10 as a
  // low range is enough to display all data but still be plottable on
  // a log scale
  start = start < 1e-90 ? 1e-10 : start;
  // improve the scale labelling by ensuring that the graph starts and
  // ends on numbers that can have major ticks e.g. 0.1 or 1 or 100
  const double exponent = floor(log10(start));
  start = pow(10.0, exponent);
  end = ceil(log10(end));
  end = pow(10.0, end);

  ScaleEngine *scaleEng = dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(axis));
  if (!scaleEng)
    return;

  // call the QTiPlot function set scale which takes many arguments,
  // fill the arguments with the same settings the plot already has
  setScale(axis, start, end, axisStep(axis),
      scDiv->ticks(QwtScaleDiv::MajorTick).count(),
      d_plot->axisMaxMinor(axis),
      QwtScaleTransformation::Log10,
      scaleEng->testAttribute(QwtScaleEngine::Inverted),
      scaleEng->axisBreakLeft(),
      scaleEng->axisBreakRight(),
      scaleEng->minTicksBeforeBreak(),
      scaleEng->minTicksAfterBreak(),
      scaleEng->log10ScaleAfterBreak(),
      scaleEng->breakWidth(),
      scaleEng->hasBreakDecoration());
}

void Graph::setScale(int axis, double start, double end, double step,
    int majorTicks, int minorTicks, int type, bool inverted,
    double left_break, double right_break, int breakPos,
    double stepBeforeBreak, double stepAfterBreak, int minTicksBeforeBreak,
    int minTicksAfterBreak, bool log10AfterBreak, int breakWidth, bool breakDecoration)
{
  ScaleEngine* se = dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(axis));
  se->setBreakRegion(left_break, right_break);
  se->setBreakPosition(breakPos);
  se->setBreakWidth(breakWidth);
  se->drawBreakDecoration(breakDecoration);
  se->setStepBeforeBreak(stepBeforeBreak);
  se->setStepAfterBreak(stepAfterBreak);
  se->setMinTicksBeforeBreak(minTicksBeforeBreak);
  se->setMinTicksAfterBreak(minTicksAfterBreak);
  se->setLog10ScaleAfterBreak(log10AfterBreak);
  se->setAttribute(QwtScaleEngine::Inverted, inverted);

  setAxisScale(axis, start, end, type, step, majorTicks, minorTicks);

  for (int i=0; i<n_curves; i++){
    QwtPlotItem *it = plotItem(i);
    if (!it)
      continue;
    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
      Spectrogram *sp = dynamic_cast<Spectrogram *>(it);
      if(sp)
      {	updatedaxis[axis]=1;
      }
    }
  }
}
/** Overload of setScale() to that only allows setting the axis type
 *  to linear or log. Does nothing if the scale is already the that type
 *  @param axis :: the scale to change either QwtPlot::xBottom or QwtPlot::yLeft
 *  @param scaleType :: either QwtScaleTransformation::Log10 or ::Linear
 */
void Graph::setScale(QwtPlot::Axis axis, QwtScaleTransformation::Type scaleType)
{
  //check if the scale is already of the desired type, 
  ScaleEngine *sc_engine = dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(axis));
  QwtScaleTransformation::Type type = sc_engine->type();
  if ( scaleType == QwtScaleTransformation::Log10 )
  {
    if ( type ==  QwtScaleTransformation::Log10 )
    {
      return;
    }
  }
  else if ( type == QwtScaleTransformation::Linear )
  {
    return;
  }

  const QwtScaleDiv *scDiv = d_plot->axisScaleDiv(axis);
  double start = QMIN(scDiv->lBound(), scDiv->hBound());
  double end = QMAX(scDiv->lBound(), scDiv->hBound());

  ScaleEngine *scaleEng = dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(axis));

  // call the QTiPlot function set scale which takes many arguments, fill the arguments with the same settings the plot already has
  setScale(axis, start, end, axisStep(axis),
      scDiv->ticks(QwtScaleDiv::MajorTick).count(),
      d_plot->axisMaxMinor(axis), scaleType,
      scaleEng->testAttribute(QwtScaleEngine::Inverted),
      scaleEng->axisBreakLeft(),
      scaleEng->axisBreakRight(),
      scaleEng->minTicksBeforeBreak(),
      scaleEng->minTicksAfterBreak(),
      scaleEng->log10ScaleAfterBreak(),
      scaleEng->breakWidth(),
      scaleEng->hasBreakDecoration());
}
/** This setScale overload allows setting the scale type by passing "linear"
 *  or "log" as a string
 *  @param axis :: the scale to change either QwtPlot::xBottom or QwtPlot::yLeft
 *  @param logOrLin :: either "log" or "linear"
 */
void Graph::setScale(QwtPlot::Axis axis, QString logOrLin)
{
  if ( logOrLin == "log" )
  {
    setScale(axis, QwtScaleTransformation::Log10);
  }
  else if ( logOrLin == "linear" )
  {
    setScale(axis, QwtScaleTransformation::Linear);
  }
}

void Graph::logLogAxes()
{
  setScale(QwtPlot::xBottom, QwtScaleTransformation::Log10);
  setScale(QwtPlot::yLeft, QwtScaleTransformation::Log10);
  notifyChanges();
}

void Graph::logXLinY()
{
  setScale(QwtPlot::xBottom, QwtScaleTransformation::Log10);
  setScale(QwtPlot::yLeft, QwtScaleTransformation::Linear);
  notifyChanges();
}

void Graph::logYlinX()
{
  setScale(QwtPlot::xBottom, QwtScaleTransformation::Linear);
  setScale(QwtPlot::yLeft, QwtScaleTransformation::Log10);
  notifyChanges();
}

void Graph::linearAxes()
{
  setScale(QwtPlot::xBottom, QwtScaleTransformation::Linear);
  setScale(QwtPlot::yLeft, QwtScaleTransformation::Linear);
  notifyChanges();
}

void Graph::logColor()
{
  setScale(QwtPlot::yRight, QwtScaleTransformation::Log10);
  notifyChanges();
}

void Graph::linColor()
{
  setScale(QwtPlot::yRight, QwtScaleTransformation::Linear);
  notifyChanges();
}

void Graph::setAxisScale(int axis, double start, double end, int type, double step,
    int majorTicks, int minorTicks)
{
  ScaleEngine *sc_engine = dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(axis));
  if(!sc_engine)
    return;

  QwtScaleTransformation::Type old_type = sc_engine->type();

  // If not specified, keep the same as now
  if( type < 0 ) type = axisType(axis);

  if (type != old_type)
  {
    // recalculate boundingRect of MantidCurves
    emit axisScaleChanged(axis,type == QwtScaleTransformation::Log10);
  }

  if (type == GraphOptions::Log10)
  {
    sc_engine->setType(QwtScaleTransformation::Log10);
    if (start <= 0)
    {
      double s_min = DBL_MAX;
      // for the y axis rely on the bounding rects
      for(int i=0;i<curves();++i)
      {
        QwtPlotCurve* c = curve(i);
        if (c)
        {
          double s;
          if (axis == QwtPlot::yRight || axis == QwtPlot::yLeft)
          {
            s = c->boundingRect().y();
          }
          else
          {
            s = c->boundingRect().x();
          }
          if (s > 0 && s < s_min)
          {
            s_min = s;
          }
        }
      }

      if (s_min != DBL_MAX && s_min > 0)
      {
        start = s_min;
      }
      else
      {
        if (end <= 0)
        {
          start = 1;
          end = 1000;
        }
        else
        {
          start = 0.01 * end;
        }
      }
    }
    // log scales can't represent zero or negative values, 1e-10 is a low number that I hope will be lower than most of the data but is still sensible for many color plots
    //start = start < 1e-90 ? 1e-10 : start;
  }
  else
  {
    sc_engine->setType(QwtScaleTransformation::Linear);
  }

  if (axis == QwtPlot::yRight)
  {
    for (int i=0; i<n_curves; i++){
      QwtPlotItem *it = plotItem(i);
      if (!it)
        continue;
      if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
        Spectrogram *sp = dynamic_cast<Spectrogram *>(it);
        if(sp)
        {
          QwtScaleWidget *rightAxis = d_plot->axisWidget(QwtPlot::yRight);
          if(rightAxis)
          {
            if (type == QwtScaleTransformation::Log10 && (start <= 0 || start == DBL_MAX))
            {
              start = sp->getMinPositiveValue();
            }
            sp->mutableColorMap().changeScaleType((GraphOptions::ScaleType)type);
            rightAxis->setColorMap(QwtDoubleInterval(start, end), sp->getColorMap());
            sp->setColorMap(sp->getColorMap());
            // we could check if(sp->isIntensityChanged()) but this doesn't work when one value is changing from zero to say 10^-10, which is a big problem for log plots
            sp->changeIntensity( start,end);
          }
        }
      }
    }
  }

  int max_min_intervals = minorTicks;
  if (minorTicks == 1) max_min_intervals = 3;
  if (minorTicks > 1) max_min_intervals = minorTicks + 1;
  QwtScaleDiv div = sc_engine->divideScale (QMIN(start, end), QMAX(start, end), majorTicks, max_min_intervals, step);
  d_plot->setAxisMaxMajor (axis, majorTicks);
  d_plot->setAxisMaxMinor (axis, minorTicks);

  d_plot->setAxisScaleDiv (axis, div);

  d_zoomer[0]->setZoomBase();
  //below code is commented as it was zooming the right color  axis on scaling
  d_zoomer[1]->setZoomBase();

  d_user_step[axis] = step;

  if (axis == QwtPlot::xBottom || axis == QwtPlot::yLeft)
  {
    updateSecondaryAxis(QwtPlot::xTop);
    updateSecondaryAxis(QwtPlot::yRight);
  }
  d_plot->replot();
  ////keep markers on canvas area
  updateMarkersBoundingRect();
  d_plot->replot();
  d_plot->axisWidget(axis)->repaint();

}


QStringList Graph::analysableCurvesList()
{
  QStringList cList;
  QList<int> keys = d_plot->curveKeys();
  for (int i=0; i<(int)keys.count(); i++)
  {
    QwtPlotCurve *c = d_plot->curve(keys[i]);
    if (c && c_type[i] != ErrorBars)
      cList << c->title().text();
  }
  return cList;
}

QStringList Graph::curvesList()
{
  QStringList cList;
  QList<int> keys = d_plot->curveKeys();
  for (int i=0; i<(int)keys.count(); i++)
  {
    QwtPlotCurve *c = d_plot->curve(keys[i]);
    if (c)
      cList << c->title().text();
  }
  return cList;
}

QStringList Graph::plotItemsList()
{
  QStringList cList;
  QList<int> keys = d_plot->curveKeys();
  for (int i=0; i<(int)keys.count(); i++)
  {
    QwtPlotItem *it = d_plot->plotItem(keys[i]);
    if (it)
      cList << it->title().text();
  }
  return cList;
}

void Graph::copyImage()
{
  QApplication::clipboard()->setPixmap(graphPixmap(), QClipboard::Clipboard);
}

QPixmap Graph::graphPixmap()
{
  return QPixmap::grabWidget(this);
}

void Graph::exportToFile(const QString& fileName)
{
  if ( fileName.isEmpty() ){
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Please provide a valid file name!"));
    return;
  }

  if (fileName.contains(".eps") || fileName.contains(".pdf") || fileName.contains(".ps")){
    exportVector(fileName);
    return;
  } else if(fileName.contains(".svg")){
    exportSVG(fileName);
    return;
  } else {
    QList<QByteArray> list = QImageWriter::supportedImageFormats();
    for(int i=0 ; i<list.count() ; i++){
      if (fileName.contains( "." + list[i].toLower())){
        exportImage(fileName);
        return;
      }
    }
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("File format not handled, operation aborted!"));
  }
}

void Graph::exportImage(const QString& fileName, int quality, bool transparent)
{
  QPixmap pic(d_plot->size());
  QPainter p(&pic);
  d_plot->print(&p, d_plot->rect());
  p.end();

  if (transparent){
    QBitmap mask(pic.size());
    mask.fill(Qt::color1);
    QPainter p(&mask);
    p.setPen(Qt::color0);

    QColor background = QColor (Qt::white);
    QRgb backgroundPixel = background.rgb ();
    QImage image = pic.convertToImage();
    for (int y=0; y<image.height(); y++){
      for ( int x=0; x<image.width(); x++ ){
        QRgb rgb = image.pixel(x, y);
        if (rgb == backgroundPixel) // we want the frame transparent
          p.drawPoint(x, y);
      }
    }
    p.end();
    pic.setMask(mask);
  }
  pic.save(fileName, 0, quality);
}

void Graph::exportVector(const QString& fileName, int, bool color, bool keepAspect, QPrinter::PageSize pageSize)
{
  if ( fileName.isEmpty() ){
    QMessageBox::critical(this, tr("MantidPlot - Error"), tr("Please provide a valid file name!"));
    return;
  }

  QPrinter printer;
  printer.setCreator("MantidPlot");
  printer.setFullPage(true);
  //if (res) //only printing with screen resolution works correctly for the moment
  //printer.setResolution(res);

  printer.setOutputFileName(fileName);
  if (fileName.contains(".eps"))
    printer.setOutputFormat(QPrinter::PostScriptFormat);

  if (color)
    printer.setColorMode(QPrinter::Color);
  else
    printer.setColorMode(QPrinter::GrayScale);

  QRect plotRect = d_plot->rect();
  if (pageSize == QPrinter::Custom)
    printer.setPageSize(minPageSize(printer, plotRect));
  else
    printer.setPageSize(pageSize);

  double plot_aspect = double(d_plot->frameGeometry().width())/double(d_plot->frameGeometry().height());
  if (plot_aspect < 1)
    printer.setOrientation(QPrinter::Portrait);
  else
    printer.setOrientation(QPrinter::Landscape);

  if (keepAspect){// export should preserve plot aspect ratio
    double page_aspect = double(printer.width())/double(printer.height());
    if (page_aspect > plot_aspect){
      int margin = (int) ((0.1/2.54)*printer.logicalDpiY()); // 1 mm margins
      int height = printer.height() - 2*margin;
      int width = static_cast<int>(height*plot_aspect);
      int x = (printer.width()- width)/2;
      plotRect = QRect(x, margin, width, height);
    } else if (plot_aspect >= page_aspect){
      int margin = (int) ((0.1/2.54)*printer.logicalDpiX()); // 1 mm margins
      int width = printer.width() - 2*margin;
      int height = static_cast<int>(width/plot_aspect);
      int y = (printer.height()- height)/2;
      plotRect = QRect(margin, y, width, height);
    }
  } else {
    int x_margin = (int) ((0.1/2.54)*printer.logicalDpiX()); // 1 mm margins
    int y_margin = (int) ((0.1/2.54)*printer.logicalDpiY()); // 1 mm margins
    int width = printer.width() - 2*x_margin;
    int height = printer.height() - 2*y_margin;
    plotRect = QRect(x_margin, y_margin, width, height);
  }

  QPainter paint(&printer);
  d_plot->print(&paint, plotRect);
}

void Graph::print()
{
  QPrinter printer;
  printer.setColorMode (QPrinter::Color);
  printer.setFullPage(true);

  //printing should preserve plot aspect ratio, if possible
  double aspect = double(d_plot->width())/double(d_plot->height());
  if (aspect < 1)
    printer.setOrientation(QPrinter::Portrait);
  else
    printer.setOrientation(QPrinter::Landscape);

  QPrintDialog printDialog(&printer);
  if (printDialog.exec() == QDialog::Accepted){
    QRect plotRect = d_plot->rect();
    QRect paperRect = printer.paperRect();
    if (d_scale_on_print){
      int dpiy = printer.logicalDpiY();
      int margin = (int) ((2/2.54)*dpiy ); // 2 cm margins

      int width = qRound(aspect*printer.height()) - 2*margin;
      int x=qRound(abs(printer.width()- width)*0.5);

      plotRect = QRect(x, margin, width, printer.height() - 2*margin);
      if (x < margin){
        plotRect.setLeft(margin);
        plotRect.setWidth(printer.width() - 2*margin);
      }
    } else {
      int x_margin = (paperRect.width() - plotRect.width())/2;
      int y_margin = (paperRect.height() - plotRect.height())/2;
      plotRect.moveTo(x_margin, y_margin);
    }

    QPainter paint(&printer);
    if (d_print_cropmarks){
      QRect cr = plotRect; // cropmarks rectangle
      cr.addCoords(-1, -1, 2, 2);
      paint.save();
      paint.setPen(QPen(QColor(Qt::black), 0.5, Qt::DashLine));
      paint.drawLine(paperRect.left(), cr.top(), paperRect.right(), cr.top());
      paint.drawLine(paperRect.left(), cr.bottom(), paperRect.right(), cr.bottom());
      paint.drawLine(cr.left(), paperRect.top(), cr.left(), paperRect.bottom());
      paint.drawLine(cr.right(), paperRect.top(), cr.right(), paperRect.bottom());
      paint.restore();
    }

    d_plot->print(&paint, plotRect);
  }
}

void Graph::exportSVG(const QString& fname)
{
  QSvgGenerator svg;
  svg.setFileName(fname);
  svg.setSize(d_plot->size());

  QPainter p(&svg);
  d_plot->print(&p, d_plot->rect());
  p.end();
}

int Graph::selectedCurveID()
{
  if (d_range_selector)
    return curveKey(curveIndex(d_range_selector->selectedCurve()));
  else
    return -1;
}

QString Graph::selectedCurveTitle()
{
  if (d_range_selector)
    return d_range_selector->selectedCurve()->title().text();
  else
    return QString::null;
}

bool Graph::markerSelected()
{
  return (selectedMarker >= 0 || d_selected_text);
}

void Graph::removeMarker()
{
  if (selectedMarker>=0){
    if (d_markers_selector) {
      if (d_lines.contains(selectedMarker))
        d_markers_selector->removeAll(dynamic_cast<ArrowMarker *>(d_plot->marker(selectedMarker)));
      else if (d_images.contains(selectedMarker))
        d_markers_selector->removeAll(dynamic_cast<ImageMarker *>(d_plot->marker(selectedMarker)));
    }
    d_plot->removeMarker(selectedMarker);
    d_plot->replot();
    emit modifiedGraph();

    if (d_lines.contains(selectedMarker))
    {
      int index = d_lines.indexOf(selectedMarker);
      int last_line_marker = (int)d_lines.size() - 1;
      for (int i=index; i < last_line_marker; i++)
        d_lines[i]=d_lines[i+1];
      d_lines.resize(last_line_marker);
    } else if(d_images.contains(selectedMarker)){
      int index=d_images.indexOf(selectedMarker);
      int last_image_marker = d_images.size() - 1;
      for (int i=index; i < last_image_marker; i++)
        d_images[i]=d_images[i+1];
      d_images.resize(last_image_marker);
    }
    selectedMarker=-1;
  } else if (d_selected_text){
    if (d_selected_text == d_legend)
      d_legend = NULL;
    d_selected_text->close();
    d_selected_text = NULL;
  }
}

bool Graph::arrowMarkerSelected()
{
  return (d_lines.contains(selectedMarker));
}

bool Graph::imageMarkerSelected()
{
  return (d_images.contains(selectedMarker));
}

void Graph::deselect()
{
  deselectMarker();
  scalePicker->deselect();  
  if(d_legend != NULL)
  {
    d_legend->setSelected(false);
  }
  titlePicker->setSelected(false);
  deselectCurves();
}

void Graph::deselectCurves()
{
  QList<QwtPlotItem *> curves = d_plot->curvesList();
  foreach(QwtPlotItem *i, curves){
    PlotCurve *c = dynamic_cast<PlotCurve *>(i);
    DataCurve *dc = dynamic_cast<DataCurve *>(i);
    if(c && dc &&
       i->rtti() != QwtPlotItem::Rtti_PlotSpectrogram &&
       c->type() != Graph::Function &&  dc->hasSelectedLabels()  )
      {
        dc->setLabelsSelected(false);
        return;
      }
  }
}

DataCurve* Graph::selectedCurveLabels()
{
  QList<QwtPlotItem *> curves = d_plot->curvesList();
  foreach(QwtPlotItem *i, curves){
    PlotCurve *c = dynamic_cast<PlotCurve *>(i);
    DataCurve *dc = dynamic_cast<DataCurve *>(i);
    if(dc && i->rtti() != QwtPlotItem::Rtti_PlotSpectrogram &&
        c->type() != Graph::Function && dc->hasSelectedLabels())
      return dc;
  }
  return NULL;
}

bool Graph::titleSelected()
{
  return titlePicker->selected();
}

void Graph::selectTitle(bool select)
{
  if(d_legend != NULL)
  {
    d_legend->setSelected(!select);
  }
  titlePicker->setSelected(select);
  
  if (select){
    deselect();
    emit selectedGraph(this);
    emit currentFontChanged(d_plot->title().font());
  }
}

void Graph::setTitle(const QString& t)
{
  d_plot->setTitle (t);
  emit modifiedGraph();
}

void Graph::removeTitle()
{
  d_plot->setTitle("");
  emit modifiedGraph();
}

void Graph::initTitle(bool on, const QFont& fnt)
{
  if (on){
    QwtText t = d_plot->title();
    t.setFont(fnt);
    t.setText(tr("Title"));
    d_plot->setTitle (t);
  }
}

void Graph::setCurveTitle(int index, const QString & title)
{
  QwtPlotItem *curve = plotItem(index);
  if( !curve ) return;
  curve->setTitle(title);
  legend()->setText(legendText());
  legend()->repaint();
}

void Graph::removeLegend()
{
  if (d_legend){
    d_legend->deleteLater();
    d_legend = NULL;
  }
}

void Graph::updateImageMarker(int x, int y, int w, int h)
{
  ImageMarker* mrk = dynamic_cast<ImageMarker *>(d_plot->marker(selectedMarker));
  mrk->setRect(x, y, w, h);
  d_plot->replot();
  emit modifiedGraph();
}

QString Graph::legendText()
{
  QString text="";
  for (int i=0; i<n_curves; i++){
    const QwtPlotCurve *c = curve(i);
    if (c && c->rtti() != QwtPlotItem::Rtti_PlotSpectrogram && c_type[i] != ErrorBars ){
      text+="\\l(";
      text+=QString::number(i+1);
      text+=")%(";
      text+=QString::number(i+1);
      text+=")\n";
    }
  }
  return text.trimmed();
}

QString Graph::pieLegendText()
{
  QString text="";
  QList<int> keys= d_plot->curveKeys();
  const QwtPlotCurve *curve = dynamic_cast<QwtPlotCurve *>(d_plot->curve(keys[0]));
  if (curve){
    for (int i=0;i<static_cast<int>(curve->dataSize());i++){
      text+="\\p{";
      text+=QString::number(i+1);
      text+="} ";
      text+=QString::number(i+1);
      text+="\n";
    }
  }
  return text.trimmed();
}

void Graph::updateCurvesData(Table* w, const QString& yColName)
{
  QList<int> keys = d_plot->curveKeys();
  int updated_curves = 0;
  for (int i=0; i<(int)keys.count(); i++){
    QwtPlotItem *it = d_plot->plotItem(keys[i]);
    if (!it) continue;

    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
      continue;
    PlotCurve *c = dynamic_cast<PlotCurve *>(it);
    if (!c) continue;

    if (c->type() == Function) continue;

    DataCurve *dc = dynamic_cast<DataCurve *>(it);
    if (!dc) continue;

    if(dc->updateData(w, yColName))
      updated_curves++;
  }
  if (updated_curves){
    for (int i = 0; i < QwtPlot::axisCnt; i++){
      QwtScaleWidget *scale = d_plot->axisWidget(i);
      if (scale)
        connect(scale, SIGNAL(scaleDivChanged()), this, SLOT(updateMarkersBoundingRect()));
    }
    updatePlot();
  }
}

QColor Graph::canvasFrameColor()
{
  QwtPlotCanvas* canvas=(QwtPlotCanvas*) d_plot->canvas();
  QPalette pal =canvas->palette();
  return pal.color(QPalette::Active, QColorGroup::Foreground);
}

int Graph::canvasFrameWidth()
{
  QwtPlotCanvas* canvas=(QwtPlotCanvas*) d_plot->canvas();
  return canvas->lineWidth();
}

void Graph::setCanvasFrame(int width, const QColor& color)
{
  QwtPlotCanvas* canvas=(QwtPlotCanvas*) d_plot->canvas();
  QPalette pal = canvas->palette();

  if (canvas->lineWidth() == width &&
      pal.color(QPalette::Active, QColorGroup::Foreground) == color)
    return;

  canvas->setLineWidth(width);
  pal.setColor(QColorGroup::Foreground,color);
  canvas->setPalette(pal);
  emit modifiedGraph();
}

void Graph::drawAxesBackbones(bool yes)
{

  drawAxesBackbone = yes;

  for (int i=0; i<QwtPlot::axisCnt; i++)
  {
    QwtScaleWidget *scale= dynamic_cast<QwtScaleWidget*>(d_plot->axisWidget(i));
    if (scale)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw (i));
      if (isColorBarEnabled(i)) //always draw the backbone for a colour bar axis
      {
        sclDraw->enableComponent (QwtAbstractScaleDraw::Backbone, true);
      }
      else
      {
        sclDraw->enableComponent (QwtAbstractScaleDraw::Backbone, yes);
      }
      scale->repaint();
    }
  }
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::loadAxesOptions(const QString& s)
{
  if (s == "1")
    return;

  drawAxesBackbone = false;

  for (int i=0; i<QwtPlot::axisCnt; i++)
  {
    QwtScaleWidget *scale=dynamic_cast<QwtScaleWidget*>(d_plot->axisWidget(i));
    if (scale)
    {
      ScaleDraw *sclDraw = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw (i));
      sclDraw->enableComponent (QwtAbstractScaleDraw::Backbone, false);
      scale->repaint();
    }
  }
}

void Graph::setAxesLinewidth(int width)
{
  if (d_plot->axesLinewidth() == width)
    return;

  d_plot->setAxesLinewidth(width);

  for (int i=0; i<QwtPlot::axisCnt; i++){
    QwtScaleWidget *scale=dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(i));
    if (scale){
      scale->setPenWidth(width);
      scale->repaint();
    }
  }
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::loadAxesLinewidth(int width)
{
  d_plot->setAxesLinewidth(width);
}

void Graph::setAxisTitleColor(int axis, const QColor& c)
{
  QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(axis));
  if (scale){
    QwtText title = scale->title();
    title.setColor(c);
    scale->setTitle(title);
  }
}

QString Graph::savePieCurveLayout()
{
  QString s="PieCurve\t";

  QwtPieCurve *pie = dynamic_cast<QwtPieCurve*>(curve(0));
  s+= pie->title().text()+"\t";
  QPen pen = pie->pen();
  s+=QString::number(pen.widthF())+"\t";
  s+=pen.color().name()+"\t";
  s+=penStyleName(pen.style()) + "\t";
  s+=QString::number(PatternBox::patternIndex(pie->pattern()))+"\t";
  s+=QString::number(pie->radius())+"\t";
  s+=QString::number(pie->firstColor())+"\t";
  s+=QString::number(pie->startRow())+"\t"+QString::number(pie->endRow())+"\t";
  s+=QString::number(pie->isVisible())+"\t";

  //Starting with version 0.9.3-rc3
  s+=QString::number(pie->startAzimuth())+"\t";
  s+=QString::number(pie->viewAngle())+"\t";
  s+=QString::number(pie->thickness())+"\t";
  s+=QString::number(pie->horizontalOffset())+"\t";
  s+=QString::number(pie->labelsEdgeDistance())+"\t";
  s+=QString::number(pie->counterClockwise())+"\t";
  s+=QString::number(pie->labelsAutoFormat())+"\t";
  s+=QString::number(pie->labelsValuesFormat())+"\t";
  s+=QString::number(pie->labelsPercentagesFormat())+"\t";
  s+=QString::number(pie->labelCategories())+"\t";
  s+=QString::number(pie->fixedLabelsPosition())+"\n";
  return s;
}

QString Graph::saveCurveLayout(int index)
{
  QString s = QString::null;
  int style = c_type[index];
  QwtPlotCurve *c = dynamic_cast<QwtPlotCurve*>(curve(index));
  if (c){
    s += QString::number(style)+"\t";
    if (style == Spline)
      s+="5\t";
    else if (style == VerticalSteps)
      s+="6\t";
    else
      s+=QString::number(c->style())+"\t";
    s+=QString::number(ColorBox::colorIndex(c->pen().color()))+"\t";
    s+=QString::number(c->pen().style()-1)+"\t";
    s+=QString::number(c->pen().widthF())+"\t";

    const QwtSymbol symbol = c->symbol();
    s+=QString::number(symbol.size().width())+"\t";
    s+=QString::number(SymbolBox::symbolIndex(symbol.style()))+"\t";
    s+=QString::number(ColorBox::colorIndex(symbol.pen().color()))+"\t";
    if (symbol.brush().style() != Qt::NoBrush)
      s+=QString::number(ColorBox::colorIndex(symbol.brush().color()))+"\t";
    else
      s+=QString::number(-1)+"\t";

    bool filled = c->brush().style() == Qt::NoBrush ? false : true;
    s+=QString::number(filled)+"\t";

    s+=QString::number(ColorBox::colorIndex(c->brush().color()))+"\t";
    s+=QString::number(PatternBox::patternIndex(c->brush().style()))+"\t";
    if (style <= LineSymbols || style == Box)
      s+=QString::number(symbol.pen().widthF())+"\t";
  }

  if(style == VerticalBars || style == HorizontalBars || style == Histogram){
    QwtBarCurve *b = dynamic_cast<QwtBarCurve *>(c);
    if (b) {
      s+=QString::number(b->gap())+"\t";
      s+=QString::number(b->offset())+"\t";
    }
  }

  if (style == Histogram){
    QwtHistogram *h = dynamic_cast<QwtHistogram *>(c);
    if (h) {
      s+=QString::number(h->autoBinning())+"\t";
      s+=QString::number(h->binSize())+"\t";
      s+=QString::number(h->begin())+"\t";
      s+=QString::number(h->end())+"\t";
    }
  } else if(style == VectXYXY || style == VectXYAM){
    VectorCurve *v = dynamic_cast<VectorCurve *>(c);
    if (v) {
      s+=v->color().name()+"\t";
      s+=QString::number(v->width())+"\t";
      s+=QString::number(v->headLength())+"\t";
      s+=QString::number(v->headAngle())+"\t";
      s+=QString::number(v->filledArrowHead())+"\t";

      QStringList colsList = v->plotAssociation().split(",", QString::SkipEmptyParts);
      s+=colsList[2].remove("(X)").remove("(A)")+"\t";
      s+=colsList[3].remove("(Y)").remove("(M)");
      if (style == VectXYAM)
        s+="\t"+QString::number(v->position());
      s+="\t";
    }
  } else if(style == Box){
    BoxCurve *b = static_cast<BoxCurve*>(c);
    s+=QString::number(SymbolBox::symbolIndex(b->maxStyle()))+"\t";
    s+=QString::number(SymbolBox::symbolIndex(b->p99Style()))+"\t";
    s+=QString::number(SymbolBox::symbolIndex(b->meanStyle()))+"\t";
    s+=QString::number(SymbolBox::symbolIndex(b->p1Style()))+"\t";
    s+=QString::number(SymbolBox::symbolIndex(b->minStyle()))+"\t";
    s+=QString::number(b->boxStyle())+"\t";
    s+=QString::number(b->boxWidth())+"\t";
    s+=QString::number(b->boxRangeType())+"\t";
    s+=QString::number(b->boxRange())+"\t";
    s+=QString::number(b->whiskersRangeType())+"\t";
    s+=QString::number(b->whiskersRange())+"\t";
  }
  return s;
}

LegendWidget* Graph::newLegend(const QString& text)
{
  LegendWidget* l = new LegendWidget(d_plot);

  QString s = text;
  if (s.isEmpty()){
    if (isPiePlot())
      s = pieLegendText();
    else
      s = legendText();
  }
  l->setText(s);
  ApplicationWindow *app = multiLayer()->applicationWindow();
  if (app){
    l->setFrameStyle(app->legendFrameStyle);
    l->setFont(app->plotLegendFont);
    l->setTextColor(app->legendTextColor);
    l->setBackgroundColor(app->legendBackground);
  }

  d_legend = l;
  emit modifiedGraph();
  return l;
}

void Graph::addTimeStamp()
{
  LegendWidget* l = newLegend(QDateTime::currentDateTime().toString(Qt::LocalDate));

  QPoint p = d_plot->canvas()->pos();
  l->move(QPoint(p.x() + d_plot->canvas()->width()/2, p.y() + 10));
  emit modifiedGraph();
}

LegendWidget* Graph::insertText(const std::string& type, const std::string& line)
{
  const QStringList list = QString::fromUtf8(line.c_str()).split("\t");
  QStringList fList = list;
  bool pieLabel = (type == "PieLabel") ? true : false;
  LegendWidget* l = NULL;
  if (pieLabel)
    l = new PieLabel(d_plot);
  else
    l = new LegendWidget(d_plot);

  l->move(QPoint(fList[1].toInt(),fList[2].toInt()));

  QFont fnt=QFont (fList[3],fList[4].toInt(),fList[5].toInt(),fList[6].toInt());
  fnt.setUnderline(fList[7].toInt());
  fnt.setStrikeOut(fList[8].toInt());
  l->setFont(fnt);

  l->setAngle(fList[11].toInt());

  QString text = QString();
  l->setTextColor(QColor(fList[9]));
  l->setFrameStyle(fList[10].toInt());
  QColor c = QColor(fList[12]);
  c.setAlpha(fList[13].toInt());
  l->setBackgroundColor(c);

  int n = (int)fList.count();
  if (n > 14)
    text += fList[14];

  for (int i=1; i<n-14; i++){
    int j = 14+i;
    if (n > j)
      text += "\n" + fList[j];
  }

  l->setText(text);
  if (pieLabel){
    QwtPieCurve *pie = dynamic_cast<QwtPieCurve *>(curve(0));
    if(pie)
      pie->addLabel(dynamic_cast<PieLabel *>(l));
  }
  return l;
}

void Graph::addArrow(QStringList list, int fileVersion)
{
  ArrowMarker* mrk = new ArrowMarker();
  int mrkID=d_plot->insertMarker(mrk);
  int linesOnPlot = d_lines.size();
  d_lines.resize(++linesOnPlot);
  d_lines[linesOnPlot-1]=mrkID;

  if (fileVersion < 86){
    mrk->setStartPoint(QPoint(list[1].toInt(), list[2].toInt()));
    mrk->setEndPoint(QPoint(list[3].toInt(), list[4].toInt()));
  } else
    mrk->setBoundingRect(list[1].toDouble(), list[2].toDouble(),
        list[3].toDouble(), list[4].toDouble());

  mrk->setWidth(list[5].toDouble());
  mrk->setColor(QColor(list[6]));
  mrk->setStyle(getPenStyle(list[7]));
  mrk->drawEndArrow(list[8]=="1");
  mrk->drawStartArrow(list[9]=="1");
  if (list.count()>10){
    mrk->setHeadLength(list[10].toInt());
    mrk->setHeadAngle(list[11].toInt());
    mrk->fillArrowHead(list[12]=="1");
  }
}

ArrowMarker* Graph::addArrow(ArrowMarker* mrk)
{
  ArrowMarker* aux = new ArrowMarker();
  int linesOnPlot = d_lines.size();
  d_lines.resize(++linesOnPlot);
  d_lines[linesOnPlot-1] = d_plot->insertMarker(aux);

  aux->setBoundingRect(mrk->startPointCoord().x(), mrk->startPointCoord().y(),
      mrk->endPointCoord().x(), mrk->endPointCoord().y());
  aux->setWidth(mrk->width());
  aux->setColor(mrk->color());
  aux->setStyle(mrk->style());
  aux->drawEndArrow(mrk->hasEndArrow());
  aux->drawStartArrow(mrk->hasStartArrow());
  aux->setHeadLength(mrk->headLength());
  aux->setHeadAngle(mrk->headAngle());
  aux->fillArrowHead(mrk->filledArrowHead());
  return aux;
}

ArrowMarker* Graph::arrow(int id)
{
  return dynamic_cast<ArrowMarker*>(d_plot->marker(id));
}

ImageMarker* Graph::imageMarker(int id)
{
  return dynamic_cast<ImageMarker*>(d_plot->marker(id));
}

LegendWidget* Graph::insertText(LegendWidget* t)
{
  LegendWidget* aux = new LegendWidget(d_plot);
  aux->clone(t);
  return aux;
}

double Graph::selectedXStartValue()
{
  if (d_range_selector)
    return d_range_selector->minXValue();
  else
    return 0;
}

double Graph::selectedXEndValue()
{
  if (d_range_selector)
    return d_range_selector->maxXValue();
  else
    return 0;
}

QwtPlotItem* Graph::plotItem(int index)
{
  if (!n_curves || index >= n_curves || index < 0)
    return 0;

  return d_plot->plotItem(c_keys[index]);
}

int Graph::plotItemIndex(QwtPlotItem *it) const
{
  if (!it)
    return -1;

  for (int i = 0; i < n_curves; i++){
    if (d_plot->plotItem(c_keys[i]) == it)
      return i;
  }
  return -1;
}

QwtPlotCurve *Graph::curve(int index)
{
  if (!n_curves || index >= n_curves || index < 0)
    return 0;

  return d_plot->curve(c_keys[index]);
}

int Graph::curveIndex(QwtPlotCurve *c) const
{
  return plotItemIndex(c);
}

//! get curve title string by index (convenience function for scripts)
QString Graph::curveTitle(int index)
{
        QwtPlotItem *item = plotItem(index);
        if (item)
                return item->title().text();

        return QString::null;
}

int Graph::range(int index, double *start, double *end)
{
  if (d_range_selector && d_range_selector->selectedCurve() == curve(index)) {
    *start = d_range_selector->minXValue();
    *end = d_range_selector->maxXValue();
    return d_range_selector->dataSize();
  } else {
    QwtPlotCurve *c = curve(index);
    if (!c)
      return 0;

    *start = c->x(0);
    *end = c->x(c->dataSize() - 1);
    return c->dataSize();
  }
}

CurveLayout Graph::initCurveLayout()
{
  CurveLayout cl;
  cl.connectType = 1;
  cl.lStyle = 0;
  cl.lWidth = 1;
  cl.sSize = 3;
  cl.sType = 0;
  cl.filledArea = 0;
  cl.aCol = 0;
  cl.aStyle = 0;
  cl.lCol = 0;
  cl.penWidth = 1;
  cl.symCol = 0;
  cl.fillCol = 0;
  return cl;
}

CurveLayout Graph::initCurveLayout(int style, int curves)
{
  int i = n_curves - 1;

  CurveLayout cl = initCurveLayout();
  int color;
  guessUniqueCurveLayout(color, cl.sType);

  cl.lCol = color;
  cl.symCol = color;
  cl.fillCol = color;

  if (style == Graph::Line)
    cl.sType = 0;
  else if (style == Graph::Scatter)
    cl.connectType = 0;
  else if (style == Graph::VerticalDropLines)
    cl.connectType = 2;
  else if (style == Graph::HorizontalSteps || style == Graph::VerticalSteps){
    cl.connectType = 3;
    cl.sType = 0;
  } else if (style == Graph::Spline)
    cl.connectType = 5;
  else if (curves && (style == Graph::VerticalBars || style == Graph::HorizontalBars)){
    cl.filledArea = 1;
    cl.lCol = 0;//black color pen
    cl.aCol = i + 1;
    cl.sType = 0;
    if (c_type[i] == Graph::VerticalBars || style == Graph::HorizontalBars){
      QwtBarCurve *b = dynamic_cast<QwtBarCurve *>(curve(i));
      if (b){
        b->setGap(qRound(100*(1-1.0/(double)curves)));
        b->setOffset(-50*(curves-1) + i*100);
      }
    }
  } else if (style == Graph::Histogram){
    cl.filledArea = 1;
    cl.lCol = i + 1;//start with red color pen
    cl.aCol = i + 1; //start with red fill color
    cl.aStyle = 4;
    cl.sType = 0;
  } else if (style == Graph::Area){
    cl.filledArea = 1;
    cl.aCol = color;
    cl.sType = 0;
    cl.connectType = 1;
  }
  return cl;
}

void Graph::setCurveType(int curve, int style)
{
  c_type[curve] = style;
}

void Graph::updateCurveLayout(PlotCurve* c, const CurveLayout *cL)
{	
  if (!c || c_type.isEmpty())
    return;

  int index = curveIndex(c);
  if (c_type.size() < index)
    return;

  QPen pen = QPen(ColorBox::color(cL->symCol), cL->penWidth, Qt::SolidLine);
  if (cL->fillCol != -1)
    c->setSymbol(QwtSymbol(SymbolBox::style(cL->sType), QBrush(ColorBox::color(cL->fillCol)), pen, QSize(cL->sSize, cL->sSize)));
  else
    c->setSymbol(QwtSymbol(SymbolBox::style(cL->sType), QBrush(), pen, QSize(cL->sSize, cL->sSize)));

  c->setPen(QPen(ColorBox::color(cL->lCol), cL->lWidth, getPenStyle(cL->lStyle)));

  switch (c_type[index]){
  case Scatter:
    c->setStyle(QwtPlotCurve::NoCurve);
    break;
  case Spline:
    c->setStyle(QwtPlotCurve::Lines);
    c->setCurveAttribute(QwtPlotCurve::Fitted, true);
    break;
  case VerticalSteps:
    c->setStyle(QwtPlotCurve::Steps);
    c->setCurveAttribute(QwtPlotCurve::Inverted, true);
    break;
  default:
    c->setStyle((QwtPlotCurve::CurveStyle)cL->connectType);
    break;
  }

  QBrush brush = QBrush(ColorBox::color(cL->aCol));
  if (cL->filledArea)
    brush.setStyle(PatternBox::brushStyle(cL->aStyle));
  else
    brush.setStyle(Qt::NoBrush);
  c->setBrush(brush);
}

void Graph::updateErrorBars(QwtErrorPlotCurve *er, bool xErr, double width, int cap, const QColor& c,
    bool plus, bool minus, bool through)
{
  if (!er)
    return;

  if (er->width() == width && er->capLength() == cap &&
      er->color() == c && er->plusSide() == plus &&
      er->minusSide() == minus && er->throughSymbol() == through && er->xErrors() == xErr)
    return;

  er->setWidth(width);
  er->setCapLength(cap);
  er->setColor(c);
  er->setXErrors(xErr);
  er->drawThroughSymbol(through);
  er->drawPlusSide(plus);
  er->drawMinusSide(minus);
  d_plot->replot();
  emit modifiedGraph();
}

QwtErrorPlotCurve* Graph::addErrorBars(const QString& yColName, Table *errTable, const QString& errColName,
    int type, double width, int cap, const QColor& color, bool through, bool minus, bool plus)
{
  QList<int> keys = d_plot->curveKeys();
  for (int i = 0; i<n_curves; i++ ){
    DataCurve *c = dynamic_cast<DataCurve *>(d_plot->curve(keys[i]));
    if (c && c->title().text() == yColName && c_type[i] != ErrorBars){
      return addErrorBars(c->xColumnName(), yColName, errTable, errColName,
          type, width, cap, color, through, minus, plus);
    }
  }
  return NULL;
}

QwtErrorPlotCurve* Graph::addErrorBars(const QString& xColName, const QString& yColName,
    Table *errTable, const QString& errColName, int type, double width, int cap,
    const QColor& color, bool through, bool minus, bool plus)
{
  DataCurve *master_curve = masterCurve(xColName, yColName);
  if (!master_curve)
    return NULL;

  QwtErrorPlotCurve *er = new QwtErrorPlotCurve(type, errTable, errColName);

  c_type.resize(++n_curves);
  c_type[n_curves-1] = ErrorBars;

  c_keys.resize(n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(er);

  er->setMasterCurve(master_curve);
  er->setCapLength(cap);
  er->setColor(color);
  er->setWidth(width);
  er->drawPlusSide(plus);
  er->drawMinusSide(minus);
  er->drawThroughSymbol(through);

  updatePlot();
  return er;
}

/** Adds the display of error to an existing MantidCurve
 *  @param curveName :: The name of the curve
 */
void Graph::addMantidErrorBars(const QString& curveName,bool drawAll)
{
  MantidMatrixCurve * c = dynamic_cast<MantidMatrixCurve*>(curve(curveName));
  // Give a message if this isn't a MantidCurve
  if (!c)
  {
    QMessageBox::critical(0,"MantidPlot","The selected curve is not Mantid workspace data");
    return;
  }

  c->setErrorBars(true,drawAll);
  updatePlot();
  return;
}

/** Removes the error bars form a MantidCurve
 *  @param curveName :: The name of the curve
 */
void Graph::removeMantidErrorBars(const QString& curveName)
{
  MantidMatrixCurve * c = dynamic_cast<MantidMatrixCurve*>(curve(curveName));
  // Give a message if this isn't a MantidCurve
  if (!c)
  {
    QMessageBox::critical(0,"MantidPlot","The selected curve is not Mantid workspace data");
    return;
  }

  c->setErrorBars(false);
  updatePlot();
  return;
}

ErrorBarSettings* Graph::errorBarSettings(int curveIndex, int errorBarIndex)
{
  PlotCurve* c = dynamic_cast<PlotCurve*>(curve(curveIndex));
  if ( c && errorBarIndex >= 0 )
  {
    QList<ErrorBarSettings*> settings = c->errorBarSettingsList();
    if ( errorBarIndex < settings.size() )
    {
      return settings[errorBarIndex];
    }
  }
  return NULL;
}

QwtPieCurve* Graph::plotPie(Table* w, const QString& name, const QPen& pen, int brush,
    int size, int firstColor, int startRow, int endRow, bool visible,
    double d_start_azimuth, double d_view_angle, double d_thickness,
    double d_horizontal_offset, double d_edge_dist, bool d_counter_clockwise,
    bool d_auto_labeling, bool d_values, bool d_percentages,
    bool d_categories, bool d_fixed_labels_pos)
{
  if (endRow < 0)
    endRow = w->numRows() - 1;

  QwtPieCurve *pie = new QwtPieCurve(w, name, startRow, endRow);

  c_keys.resize(++n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(pie);

  c_type.resize(n_curves);
  c_type[n_curves-1] = Pie;

  pie->loadData();
  pie->setPen(pen);
  pie->setRadius(size);
  pie->setFirstColor(firstColor);
  pie->setBrushStyle(PatternBox::brushStyle(brush));
  pie->setVisible(visible);

  pie->setStartAzimuth(d_start_azimuth);
  pie->setViewAngle(d_view_angle);
  pie->setThickness(d_thickness);
  pie->setHorizontalOffset(d_horizontal_offset);
  pie->setLabelsEdgeDistance(d_edge_dist);
  pie->setCounterClockwise(d_counter_clockwise);
  pie->setLabelsAutoFormat(d_auto_labeling);
  pie->setLabelValuesFormat(d_values);
  pie->setLabelPercentagesFormat(d_percentages);
  pie->setLabelCategories(d_categories);
  pie->setFixedLabelsPosition(d_fixed_labels_pos);
  return pie;
}

QwtPieCurve* Graph::plotPie(Table* w, const QString& name, int startRow, int endRow)
{
  for (int i=0; i<QwtPlot::axisCnt; i++)
    d_plot->enableAxis(i, false);
  scalePicker->refresh();

  d_plot->setTitle(QString::null);

  QwtPlotCanvas* canvas=(QwtPlotCanvas*) d_plot->canvas();
  canvas->setLineWidth(1);

  QwtPieCurve *pie = new QwtPieCurve(w, name, startRow, endRow);

  c_keys.resize(++n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(pie);

  c_type.resize(n_curves);
  c_type[n_curves-1] = Pie;

  pie->loadData();
  pie->initLabels();
  d_plot->replot();
  return pie;
}

void Graph::insertPlotItem(QwtPlotItem *i, int type)
{
  c_type.resize(++n_curves);
  c_type[n_curves-1] = type;

  c_keys.resize(n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(i);

  if (i->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
    addLegendItem();
}

bool Graph::addCurves(Table* w, const QStringList& names, int style, double lWidth,
    int sSize, int startRow, int endRow)
{
  if (style == Pie)
    plotPie(w, names[0], startRow, endRow);
  else if (style == Box)
    plotBoxDiagram(w, names, startRow, endRow);
  else if (style == VectXYXY || style == VectXYAM)
    plotVectorCurve(w, names, style, startRow, endRow);
  else {
    QStringList drawableNames;
    int noOfErrorCols = 0;
    QString xColNameGiven;

    // Select only those column names which we can draw and search for any X columns specified
    for (int i = 0; i < names.count(); i++)
    {
      int c = w->colIndex(names[i]);
      if (c < 0)
      {
        continue;
      }
      int d = w->colPlotDesignation(c);

      switch(d)
      {
        case Table::Y:
          // Y columns should be drawn first, so we are keeping them at the beginning of the list
          drawableNames.prepend(names[i]);
          break;

        case Table::xErr: case Table::yErr:
          noOfErrorCols++;
          // Fall through, as we want errors to be at the end of the list in the same way as labels
          // are. So _no break_ here on purpose.

        case Table::Label:
          // Keep error/label columns at the end of the list
          drawableNames.append(names[i]);
          break;

        case Table::X:
          if(!xColNameGiven.isEmpty())
            // If multiple X columns are specified, it's an error, as we don't know which one to use
            return false;
          xColNameGiven = names[i];
          break;

        default:
          break;
      }
    }

    for (int i = 0; i < drawableNames.count(); i++){
      QString colName = drawableNames[i];
      int colIndex = w->colIndex(colName);
      int colType = w->colPlotDesignation(colIndex);

      QString yColName;
      if(colType == Table::Y)
        // For Y columns we use the column itself as Y
        yColName = colName;
      else 
        // For other column types, we find associated Y column
        yColName = w->colName(w->colY(colIndex));

      QString xColName;
      if(!xColNameGiven.isEmpty())
        // If X column is given - use it
        xColName = xColNameGiven;
      else
        // Otherise, use associated one
        xColName = w->colName(w->colX(colIndex));

      if (xColName.isEmpty() || yColName.isEmpty())
        return false;

      PlotCurve* newCurve(NULL);

      // --- Drawing error columns -----------------------------
      if (colType == Table::xErr || colType == Table::yErr){
        int dir;
        if(colType == Table::xErr)
          dir = QwtErrorPlotCurve::Horizontal;
        else
          dir = QwtErrorPlotCurve::Vertical;

        newCurve = addErrorBars(xColName, yColName, w, colName, dir);
      // --- Drawing label columns -----------------------------
      } else if (colType == Table::Label){
        DataCurve* mc = masterCurve(xColName, yColName);
        if (!mc)
          return false;

        d_plot->replot();
        mc->setLabelsColumnName(colName);
      // --- Drawing Y columns -----------------------------
      } else if (colType == Table::Y)
      {
        newCurve = insertCurve(w, xColName, yColName, style, startRow, endRow);
      }

      // Set a layout for the new curve, if we've added one
      if (newCurve)
      {
        CurveLayout cl = initCurveLayout(style, drawableNames.count() - noOfErrorCols);
        cl.sSize = sSize;
        cl.lWidth = static_cast<float>(lWidth);

        updateCurveLayout(newCurve, &cl);
      }
    }
  }
  initScaleLimits();
  return true;
}

PlotCurve* Graph::insertCurve(Table* w, const QString& name, int style, int startRow, int endRow)
{//provided for convenience
  int ycol = w->colIndex(name);
  int xcol = w->colX(ycol);

  PlotCurve* c = insertCurve(w, w->colName(xcol), w->colName(ycol), style, startRow, endRow);
  if (c)
    emit modifiedGraph();
  return c;
}

PlotCurve* Graph::insertCurve(Table* w, int xcol, const QString& name, int style)
{
  return insertCurve(w, w->colName(xcol), w->colName(w->colIndex(name)), style);
}

PlotCurve* Graph::insertCurve(Table* w, const QString& xColName, const QString& yColName, int style, int startRow, int endRow)
{
  int xcol=w->colIndex(xColName);
  int ycol=w->colIndex(yColName);
  if (xcol < 0 || ycol < 0)
    return NULL;

  int xColType = w->columnType(xcol);
  int yColType = w->columnType(ycol);
  int size=0;
  QString date_time_fmt = w->columnFormat(xcol);
  QStringList xLabels, yLabels;// store text labels
  QTime time0;
  QDateTime date0;

  if (endRow < 0)
    endRow = w->numRows() - 1;

  int r = abs(endRow - startRow) + 1;
  QVector<double> X(r), Y(r);
  if (xColType == Table::Time){
    for (int i = startRow; i<=endRow; i++ ){
      QString xval=w->text(i,xcol);
      if (!xval.isEmpty()){
        time0 = QTime::fromString (xval, date_time_fmt);
        if (time0.isValid())
          break;
      }
    }
  } else if (xColType == Table::Date){
    for (int i = startRow; i<=endRow; i++ ){
      QString xval=w->text(i,xcol);
      if (!xval.isEmpty()){
        date0 = QDateTime::fromString(xval, date_time_fmt);
        if (date0.isValid())
          break;
      }
    }
  }

  for (int i = startRow; i<=endRow; i++ ){
    QString xval=w->text(i,xcol);
    QString yval=w->text(i,ycol);
    if (!xval.isEmpty() && !yval.isEmpty()){
      bool valid_data = true;
      if (xColType == Table::Text){
        if (xLabels.contains(xval) == 0)
          xLabels << xval;
        X[size] = (double)(xLabels.findIndex(xval)+1);
      } else if (xColType == Table::Time){
        QTime time = QTime::fromString (xval, date_time_fmt);
        if (time.isValid())
          X[size] = time0.msecsTo (time);
        else
          X[size] = 0;
      } else if (xColType == Table::Date){
        QDateTime d = QDateTime::fromString (xval, date_time_fmt);
        if (d.isValid())
          X[size] = (double) date0.secsTo(d);
      } else
        X[size] = d_plot->locale().toDouble(xval, &valid_data);

      if (yColType == Table::Text){
        yLabels << yval;
        Y[size] = (double) (size + 1);
      } else
        Y[size] = d_plot->locale().toDouble(yval, &valid_data);

      if (valid_data)
        size++;
    }
  }

  if (!size)
    return NULL;

  X.resize(size);
  Y.resize(size);

  DataCurve *c = 0;
  if (style == VerticalBars){
    c = new QwtBarCurve(QwtBarCurve::Vertical, w, xColName, yColName, startRow, endRow);
  } else if (style == HorizontalBars){
    c = new QwtBarCurve(QwtBarCurve::Horizontal, w, xColName, yColName, startRow, endRow);
  } else if (style == Histogram){
    c = new QwtHistogram(w, xColName, yColName, startRow, endRow);
    QwtHistogram *histo = dynamic_cast<QwtHistogram *>(c);
    if (histo)
      histo->initData(Y.data(), size);
  } else
    c = new DataCurve(w, xColName, yColName, startRow, endRow);

  c_type.resize(++n_curves);
  c_type[n_curves - 1] = style;
  c_keys.resize(n_curves);
  c_keys[n_curves - 1] = d_plot->insertCurve(c);

  c->setPen(QPen(Qt::black, widthLine));

  if (style == HorizontalBars)
    c->setData(Y.data(), X.data(), size);
  else if (style != Histogram)
    c->setData(X.data(), Y.data(), size);

  if (xColType == Table::Text ){
    if (style == HorizontalBars)
      d_plot->setAxisScaleDraw(QwtPlot::yLeft, new ScaleDraw(d_plot, xLabels, xColName));
    else
      d_plot->setAxisScaleDraw (QwtPlot::xBottom, new ScaleDraw(d_plot, xLabels, xColName));
  } else if (xColType == Table::Time){
    QString fmtInfo = time0.toString() + ";" + date_time_fmt;
    if (style == HorizontalBars)
      setLabelsDateTimeFormat(QwtPlot::yLeft, ScaleDraw::Time, fmtInfo);
    else
      setLabelsDateTimeFormat(QwtPlot::xBottom, ScaleDraw::Time, fmtInfo);
  } else if (xColType == Table::Date ){
    QString fmtInfo = date0.toString(Qt::ISODate) + ";" + date_time_fmt;
    if (style == HorizontalBars)
      setLabelsDateTimeFormat(QwtPlot::yLeft, ScaleDraw::Date, fmtInfo);
    else
      setLabelsDateTimeFormat(QwtPlot::xBottom, ScaleDraw::Date, fmtInfo);
  }

  if (yColType == Table::Text)
    d_plot->setAxisScaleDraw (QwtPlot::yLeft, new ScaleDraw(d_plot, yLabels, yColName));

  addLegendItem();
  return c;
}

PlotCurve* Graph::insertCurve(QString workspaceName, int index, bool err, Graph::CurveType style)
{
  return (new MantidMatrixCurve(workspaceName,this,index, MantidMatrixCurve::Spectrum, err,false,style));
}

/**  Insert a curve with its own data source. It does not have to be
 *   a Table or a Function. The Graph takes ownership of the curve.
 */
PlotCurve* Graph::insertCurve(PlotCurve* c, int lineWidth, int curveType)
{
  MantidMatrixCurve* mc = dynamic_cast<MantidMatrixCurve*>(c);
  if (mc)
  {
    if (curves() == 0)
    {
      m_xUnits = mc->xUnits();
      m_yUnits = mc->yUnits();
      m_isDistribution = mc->isDistribution();
    }

    //If we don't have any units, let's use the new curve's units.
    if(!m_xUnits)
      m_xUnits = mc->xUnits();
    if(!m_yUnits)
      m_yUnits = mc->yUnits();

    // Compare units. X units are compared by ID, Y units - by caption. That's because Y units will
    // always be of type Label, hence will always have ID "Label", and the caption is what we are
    // interested in.
    if((m_xUnits && m_xUnits->unitID() != mc->xUnits()->unitID()) || (m_yUnits && m_yUnits->caption() != mc->yUnits()->caption()))
    {
      g_log.warning("You are overlaying plots from data having differing units!");
    }
    if ( m_isDistribution != mc->isDistribution() )
    {
      g_log.warning("You are overlaying distribution and non-distribution data!");
    }
  }

  c_type.resize(++n_curves);
  c_type[n_curves - 1] = curveType;
  c_keys.resize(n_curves);
  c_keys[n_curves - 1] = d_plot->insertCurve(c);

  int colorIndex , symbolIndex;
  guessUniqueCurveLayout(colorIndex, symbolIndex);
  if (lineWidth < 0) lineWidth = widthLine;
  c->setPen(QPen(ColorBox::color(colorIndex), lineWidth));
  QwtSymbol symbol = c->symbol();
  symbol.setPen(c->pen());
  symbol.setBrush(QBrush(ColorBox::color(colorIndex)));
  c->setSymbol(symbol);

  addLegendItem();
  connect(c,SIGNAL(removeMe(PlotCurve*)),this,SLOT(removeCurve(PlotCurve*)));
  connect(c,SIGNAL(dataUpdated()), this, SLOT(updatePlot()), Qt::QueuedConnection);
  return c;
}

void Graph::insertCurve(Graph* g, int i)
{
  if( g == this || !g ) return;
  PlotCurve *plotCurve = dynamic_cast<PlotCurve*>(g->curve(i));
  if( !plotCurve ) return;
  int curveType = g->curveType(i);
  this->insertCurve(plotCurve, -1, curveType);
}


QwtHistogram* Graph::addHistogram(Matrix *m)
{
  if (!m)
    return NULL;

  QwtHistogram *c = new QwtHistogram(m);
  c->setStyle(QwtPlotCurve::UserCurve);
  c->setPen(QPen(Qt::black, widthLine));
  c->setBrush(QBrush(Qt::black));
  c->loadData();

  c_type.resize(++n_curves);
  c_type[n_curves - 1] = Histogram;
  c_keys.resize(n_curves);
  c_keys[n_curves - 1] = d_plot->insertCurve(c);

  addLegendItem();
  updatePlot();
  return c;
}

QwtHistogram* Graph::restoreHistogram(Matrix *m, const QStringList& l)
{
  if (!m)
    return NULL;

  QwtHistogram *h = new QwtHistogram(m);
  h->setBinning(l[17].toInt(), l[18].toDouble(), l[19].toDouble(), l[20].toDouble());
  h->setGap(l[15].toInt());
  h->setOffset(l[16].toInt());
  h->loadData();
  h->setAxis(l[l.count()-5].toInt(), l[l.count()-4].toInt());
  h->setVisible(l.last().toInt());

  c_type.resize(++n_curves);
  c_type[n_curves - 1] = Histogram;
  c_keys.resize(n_curves);
  c_keys[n_curves - 1] = d_plot->insertCurve(h);
  return h;
}

VectorCurve* Graph::plotVectorCurve(Table* w, const QStringList& colList, int style, int startRow, int endRow)
{
  if (colList.count() != 4)
    return NULL;

  if (endRow < 0)
    endRow = w->numRows() - 1;

  VectorCurve *v = 0;
  if (style == VectXYAM)
    v = new VectorCurve(VectorCurve::XYAM, w, colList[0], colList[1], colList[2], colList[3], startRow, endRow);
  else
    v = new VectorCurve(VectorCurve::XYXY, w, colList[0], colList[1], colList[2], colList[3], startRow, endRow);

  if (!v)
    return NULL;

  c_type.resize(++n_curves);
  c_type[n_curves-1] = style;

  c_keys.resize(n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(v);

  v->loadData();
  v->setStyle(QwtPlotCurve::NoCurve);

  addLegendItem();
  updatePlot();
  return v;
}

void Graph::updateVectorsLayout(int curve, const QColor& color, double width,
    int arrowLength, int arrowAngle, bool filled, int position,
    const QString& xEndColName, const QString& yEndColName)
{
  VectorCurve *vect = dynamic_cast<VectorCurve *>(this->curve(curve));
  if (!vect)
    return;

  vect->setColor(color);
  vect->setWidth(width);
  vect->setHeadLength(arrowLength);
  vect->setHeadAngle(arrowAngle);
  vect->fillArrowHead(filled);
  vect->setPosition(position);

  if (!xEndColName.isEmpty() && !yEndColName.isEmpty())
    vect->setVectorEnd(xEndColName, yEndColName);
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::updatePlot()
{
  if ( isWaterfallPlot() ) updateDataCurves();

  updateScale();
}

void Graph::updateScale()
{
  d_plot->replot();
  updateMarkersBoundingRect();

  if (d_synchronize_scales)
  {
    updateSecondaryAxis(QwtPlot::xTop);
    updateSecondaryAxis(QwtPlot::yRight);
  }

  d_plot->replot();//TODO: avoid 2nd replot!
  d_zoomer[0]->setZoomBase(false);
}

void Graph::setBarsGap(int curve, int gapPercent, int offset)
{
  QwtBarCurve *bars = dynamic_cast<QwtBarCurve *>(this->curve(curve));
  if (!bars)
    return;

  if (bars->gap() == gapPercent && bars->offset() == offset)
    return;

  bars->setGap(gapPercent);
  bars->setOffset(offset);
}

void Graph::removePie()
{
  if (d_legend)
    d_legend->setText(QString::null);

  QList <PieLabel *> labels = dynamic_cast<QwtPieCurve *>(curve(0))->labelsList();
  foreach(PieLabel *l, labels)
  l->setPieCurve(0);

  d_plot->removeCurve(c_keys[0]);
  d_plot->replot();

  c_keys.resize(0);
  c_type.resize(0);
  n_curves=0;
  emit modifiedGraph();
}

void Graph::removeCurves(const QString& s)
{
  QList<int> keys = d_plot->curveKeys();
  for (int i=0; i<(int)keys.count(); i++)
  {
    QwtPlotItem *it = d_plot->plotItem(keys[i]);
    if (!it)
      continue;

    if (it->title().text() == s)
    {
      removeCurve(i);
      continue;
    }

    if (it->rtti() != QwtPlotItem::Rtti_PlotCurve)
      continue;

    PlotCurve *pc = dynamic_cast<PlotCurve *>(it);
    if (!pc || pc->type() == Function)
      continue;

    DataCurve * dc = dynamic_cast<DataCurve *>(it);
    if (!dc) continue;
    if(dc->plotAssociation().contains(s))
      removeCurve(i);
  }
  d_plot->replot();
}

void Graph::removeCurve(const QString& s)
{
  removeCurve(plotItemsList().findIndex(s));
}

void Graph::removeCurve(int index)
{
  if (index < 0 || index >= n_curves)
    return;

  QwtPlotItem *it = plotItem(index);
  if (!it)
    return;

  PlotCurve * c = dynamic_cast<PlotCurve *>(it);
  if(c) // Only 1D curves need to be considered here
  {
    disconnect(c,SIGNAL(removeMe(PlotCurve*)),this,SLOT(removeCurve(PlotCurve*)));
    disconnect(c,SIGNAL(dataUpdated()), this, SLOT(updatePlot()));

    DataCurve * dc = dynamic_cast<DataCurve *>(it);

    removeLegendItem(index);

    if (it->rtti() != QwtPlotItem::Rtti_PlotSpectrogram)
    {
      if (c->type() == ErrorBars) {
        QwtErrorPlotCurve *epc = dynamic_cast<QwtErrorPlotCurve *>(it);
        epc->detachFromMasterCurve();
      } else if (c->type() != Function && dc) {
        dc->clearErrorBars();
        dc->clearLabels();
      }

      if (d_fit_curves.contains(dynamic_cast<QwtPlotCurve *>(it)))
      {
        int i = d_fit_curves.indexOf(dynamic_cast<QwtPlotCurve *>(it));
        if (i >= 0 && i < d_fit_curves.size())
          d_fit_curves.removeAt(i);
      }
    }

    if (d_range_selector && curve(index) == d_range_selector->selectedCurve())
    {
      if (n_curves > 1 && (index - 1) >= 0)
        d_range_selector->setSelectedCurve(curve(index - 1));
      else if (n_curves > 1 && index + 1 < n_curves)
        d_range_selector->setSelectedCurve(curve(index + 1));
      else
        disableTools();
    }
    c->aboutToBeDeleted();
  }

  d_plot->removeCurve(c_keys[index]);
  d_plot->replot();
  n_curves--;

  for (int i=index; i<n_curves; i++)
  {
    c_type[i] = c_type[i+1];
    c_keys[i] = c_keys[i+1];
  }
  c_type.resize(n_curves);
  c_keys.resize(n_curves);
  emit modifiedGraph();
  emit curveRemoved();
}

/** Intended to be called in response of PlotCurve::removeMe signal; 
 *  the Graph is replotted.
 */
void Graph::removeCurve(PlotCurve* c)
{
  removeCurve(curveIndex(c));
}

/**
 * Removes the spectrogram from being managed by this Graph
 * @param sp A pointer to the Spectrogram to delete
 */
void Graph::removeSpectrogram(Spectrogram *sp)
{
  removeCurve(plotItemIndex(sp));
}

void Graph::removeLegendItem(int index)
{
  if (!d_legend || c_type[index] == ErrorBars)
    return;

  if (isPiePlot()){
    d_legend->setText(QString::null);
    return;
  }

  QString text = d_legend->text();
  QStringList items = text.split( "\n", QString::SkipEmptyParts);

  if (index >= (int) items.count())
    return;

  QStringList l = items.grep( "\\l(" + QString::number(index+1) + ")" );
  if (l.isEmpty())
    return;

  items.remove(l[0]);//remove the corresponding legend string

  for (int i=0; i<items.count(); i++){//set new curves indexes in legend text
    QString item = items[i];
    int pos1 = item.indexOf("\\l(");
    int pos2 = item.indexOf(")", pos1);
    int pos = pos1 + 3;
    int n = pos2 - pos;
    int cv = item.mid(pos, n).toInt();
    if (cv > index){
      int id = cv - 1;
      if (!id)
        id = 1;
      item.replace(pos, n, QString::number(id));
    }
    pos1 = item.indexOf("%(", pos2);
    pos2 = item.indexOf(")", pos1);
    pos = pos1 + 2;
    n = pos2 - pos;
    cv = item.mid(pos, n).toInt();
    if (cv > index){
      int id = cv - 1;
      if (!id)
        id = 1;
      item.replace(pos, n, QString::number(id));
    }
    items[i] = item;
  }
  text = items.join ( "\n" );
  d_legend->setText(text);
}

void Graph::addLegendItem()
{
  const int curveIndex = n_curves - 1;
  if( c_type[curveIndex] == ErrorBars ) return;
  if (d_legend){
    QString text = d_legend->text();
    if ( !text.endsWith ("\n") && !text.isEmpty() )
      text.append("\n");
    text.append("\\l("+QString::number(n_curves)+")");//+"%("+QString::number(n_curves)+")");

    // RJT (23/09/09): Insert actual text directly into legend rather than a 'code' for later parsing
    PlotCurve *c = dynamic_cast<PlotCurve *>(d_plot->curve(c_keys[curveIndex]));
    if (c )
      text.append(c->title().text());
    else
      text.append("%("+QString::number(c_keys[curveIndex])+")");

    d_legend->setText(text);
    d_legend->repaint();
  }
}

QString Graph::yAxisTitleFromFirstCurve()
{
  // I really don't like this...
  if(auto *firstCurve = dynamic_cast<MantidMatrixCurve*>(curve(0)))
  {
    using namespace Mantid::API;
    QString wsName = firstCurve->workspaceName();
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString());
    return MantidQt::API::PlotAxis(m_isDistribution, *ws).title();
  }
  else
  {
    return axisTitle(0);
  }
}

void Graph::contextMenuEvent(QContextMenuEvent *e)
{
  if (selectedMarker>=0) {
    emit showMarkerPopupMenu();
    return;
  }

  QPoint pos = d_plot->canvas()->mapFrom(d_plot, e->pos());
  int dist, point;
  const int curve = d_plot->closestCurve(pos.x(), pos.y(), dist, point);
  const DataCurve *c = dynamic_cast<DataCurve *>(d_plot->curve(curve));

  if (c && dist < 10)//10 pixels tolerance
    emit showCurveContextMenu(curve);
  else
    emit showContextMenu();

  e->accept();
}

void Graph::closeEvent(QCloseEvent *e)
{
  emit closedGraph();
  e->accept();
}

void Graph::hideEvent(QHideEvent* e)
{
  (void) e;
  for(int i=0;i<curves();++i)
  {
    PlotCurve* c = dynamic_cast<PlotCurve*>(curve(i));
    if (c)
    {
      c->aboutToBeDeleted();
    }
  }
}

bool Graph::zoomOn()
{
  return (d_zoomer[0]->isEnabled() || d_zoomer[1]->isEnabled());
}

void Graph::zoomed (const QwtDoubleRect &)
{
  emit modifiedGraph();
}
bool Graph::hasActiveTool()
{
  if (zoomOn() || drawLineActive() || d_active_tool || d_peak_fit_tool ||
      d_magnifier || d_panner ||
      (d_range_selector && d_range_selector->isVisible()))
    return true;

  return false;
}


void Graph::zoom(bool on)
{
  d_zoomer[0]->setEnabled(on);
  d_zoomer[1]->setEnabled(false);
  for (int i=0; i<n_curves; i++)
  {
    Spectrogram *sp = dynamic_cast<Spectrogram *>(this->curve(i));
    if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
    {
      if (sp->colorScaleAxis() == QwtPlot::xBottom || sp->colorScaleAxis() == QwtPlot::yLeft)
        d_zoomer[0]->setEnabled(false);
      else
        d_zoomer[1]->setEnabled(false);


    }
  }

  QCursor cursor=QCursor (getQPixmap("lens_xpm"),-1,-1);
  if (on)
    d_plot->canvas()->setCursor(cursor);
  else
    d_plot->canvas()->setCursor(Qt::arrowCursor);

}

void Graph::zoomOut()
{
  d_zoomer[0]->zoom(-1);
  //d_zoomer[1]->zoom(-1);

  if (d_synchronize_scales)
  {
    updateSecondaryAxis(QwtPlot::xTop);
    updateSecondaryAxis(QwtPlot::yRight);
  }
}


void Graph::drawText(bool on)
{
  deselectMarker();

  QCursor c = QCursor(Qt::IBeamCursor);
  if (on){
    d_plot->canvas()->setCursor(c);
    //d_plot->setCursor(c);
  } else {
    d_plot->canvas()->setCursor(Qt::arrowCursor);
    //d_plot->setCursor(Qt::arrowCursor);
  }
  drawTextOn = on;
}


ImageMarker* Graph::addImage(ImageMarker* mrk)
{
  if (!mrk)
    return 0;

  ImageMarker* mrk2 = new ImageMarker(mrk->fileName());

  int imagesOnPlot = d_images.size();
  d_images.resize(++imagesOnPlot);
  d_images[imagesOnPlot-1]=d_plot->insertMarker(mrk2);

  mrk2->setBoundingRect(mrk->xValue(), mrk->yValue(), mrk->right(), mrk->bottom());
  return mrk2;
}

ImageMarker* Graph::addImage(const QString& fileName)
{
  if (fileName.isEmpty() || !QFile::exists(fileName)){
    QMessageBox::warning(0, tr("MantidPlot - File open error"),
        tr("Image file: <p><b> %1 </b><p>does not exist anymore!").arg(fileName));
    return 0;
  }

  ImageMarker* mrk = new ImageMarker(fileName);
  int imagesOnPlot = d_images.size();
  d_images.resize(++imagesOnPlot);
  d_images[imagesOnPlot-1] = d_plot->insertMarker(mrk);

  QSize picSize = mrk->pixmap().size();
  int w = d_plot->canvas()->width();
  if (picSize.width()>w)
    picSize.setWidth(w);

  int h=d_plot->canvas()->height();
  if (picSize.height()>h)
    picSize.setHeight(h);

  mrk->setSize(picSize);
  d_plot->replot();

  emit modifiedGraph();
  return mrk;
}

void Graph::insertImageMarker(const QStringList& lst, int fileVersion)
{
  QString fn = lst[1];
  if (!QFile::exists(fn)){
    QMessageBox::warning(0, tr("MantidPlot - File open error"),
        tr("Image file: <p><b> %1 </b><p>does not exist anymore!").arg(fn));
  } else {
    ImageMarker* mrk = new ImageMarker(fn);
    if (!mrk)
      return;

    int imagesOnPlot = d_images.size();
    d_images.resize(++imagesOnPlot);
    d_images[imagesOnPlot-1] = d_plot->insertMarker(mrk);

    if (fileVersion < 86){
      mrk->setOrigin(QPoint(lst[2].toInt(), lst[3].toInt()));
      mrk->setSize(QSize(lst[4].toInt(), lst[5].toInt()));
    } else if (fileVersion < 90) {
      double left = lst[2].toDouble();
      double right = left + lst[4].toDouble();
      double top = lst[3].toDouble();
      double bottom = top - lst[5].toDouble();
      mrk->setBoundingRect(left, top, right, bottom);
    } else
      mrk->setBoundingRect(lst[2].toDouble(), lst[3].toDouble(), lst[4].toDouble(), lst[5].toDouble());
  }
}

void Graph::drawLine(bool on, bool arrow)
{
  drawLineOn=on;
  drawArrowOn=arrow;
  if (!on)
    emit drawLineEnded(true);
}

void Graph::modifyFunctionCurve(int curve, int type, const QStringList &formulas,
    const QString& var, double start, double end, int points)
{
  FunctionCurve *c = dynamic_cast<FunctionCurve *>(this->curve(curve));
  if (!c)
    return;

  if (c->functionType() == type &&
      c->variable() == var &&
      c->formulas() == formulas &&
      c->startRange() == start &&
      c->endRange() == end &&
      c->dataSize() == points)
    return;

  QString oldLegend = c->legend();

  c->setFunctionType((FunctionCurve::FunctionType)type);
  c->setRange(start, end);
  c->setFormulas(formulas);
  c->setVariable(var);
  c->loadData(points);

  if (d_legend){//update the legend marker
    QString text = (d_legend->text()).replace(oldLegend, c->legend());
    d_legend->setText(text);
  }
  updatePlot();
  emit modifiedGraph();
}

QString Graph::generateFunctionName(const QString& name)
{
  int index = 1;
  QString newName = name + QString::number(index);

  QStringList lst;
  for (int i=0; i<n_curves; i++){
    PlotCurve *c = dynamic_cast<PlotCurve*>(this->curve(i));
    if (!c)
      continue;

    if (c->type() == Function)
      lst << c->title().text();
  }

  while(lst.contains(newName))
    newName = name + QString::number(++index);
  return newName;
}

FunctionCurve* Graph::addFunction(const QStringList &formulas, double start, double end, int points, const QString &var, int type, const QString& title)
{
  QString name;
  if (!title.isEmpty())
    name = title;
  else
    name = generateFunctionName();

  FunctionCurve *c = new FunctionCurve((FunctionCurve::FunctionType)type, name);
  c->setRange(start, end);
  c->setFormulas(formulas);
  c->setVariable(var);
  c->loadData(points);

  c_type.resize(++n_curves);
  c_type[n_curves-1] = Line;

  c_keys.resize(n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(c);

  int colorIndex = 0, symbolIndex;
  guessUniqueCurveLayout(colorIndex, symbolIndex);
  c->setPen(QPen(ColorBox::color(colorIndex), widthLine));

  addLegendItem();
  updatePlot();

  emit modifiedGraph();
  return c;
}

FunctionCurve* Graph::insertFunctionCurve(const QString& formula, int points, int fileVersion)
{
  int type = 0;
  QStringList formulas;
  QString var, name = QString::null;
  double start = 0.0, end = 0.0;

  QStringList curve = formula.split(",");
  if (fileVersion < 87) {
    if (curve[0][0]=='f') {
      type = FunctionCurve::Normal;
      formulas += curve[0].section('=',1,1);
      var = curve[1];
      start = curve[2].toDouble();
      end = curve[3].toDouble();
    } else if (curve[0][0]=='X') {
      type = FunctionCurve::Parametric;
      formulas += curve[0].section('=',1,1);
      formulas += curve[1].section('=',1,1);
      var = curve[2];
      start = curve[3].toDouble();
      end = curve[4].toDouble();
    } else if (curve[0][0]=='R') {
      type = FunctionCurve::Polar;
      formulas += curve[0].section('=',1,1);
      formulas += curve[1].section('=',1,1);
      var = curve[2];
      start = curve[3].toDouble();
      end = curve[4].toDouble();
    }
  } else {
    type = curve[0].toInt();
    name = curve[1];

    if (type == FunctionCurve::Normal) {
      formulas << curve[2];
      var = curve[3];
      start = curve[4].toDouble();
      end = curve[5].toDouble();
    } else if (type == FunctionCurve::Polar || type == FunctionCurve::Parametric) {
      formulas << curve[2];
      formulas << curve[3];
      var = curve[4];
      start = curve[5].toDouble();
      end = curve[6].toDouble();
    }
  }
  return addFunction(formulas, start, end, points,  var, type, name);
}

void Graph::restoreFunction(const QStringList& lst)
{	
  FunctionCurve::FunctionType type = FunctionCurve::Normal;
  int points = 0, style = 0;
  QStringList formulas;
  QString var, title = QString::null;
  double start = 0.0, end = 0.0;

  QStringList::const_iterator line = lst.begin();
  for (line++; line != lst.end(); ++line){
    QString s = *line;
    if (s.contains("<Type>"))
      type = (FunctionCurve::FunctionType)s.remove("<Type>").remove("</Type>").stripWhiteSpace().toInt();
    else if (s.contains("<Title>"))
      title = s.remove("<Title>").remove("</Title>").stripWhiteSpace();
    else if (s.contains("<Expression>"))
      formulas = s.remove("<Expression>").remove("</Expression>").split("\t");
    else if (s.contains("<Variable>"))
      var = s.remove("<Variable>").remove("</Variable>").stripWhiteSpace();
    else if (s.contains("<Range>")){
      QStringList l = s.remove("<Range>").remove("</Range>").split("\t");
      if (l.size() == 2){
        start = l[0].toDouble();
        end = l[1].toDouble();
      }
    } else if (s.contains("<Points>"))
      points = s.remove("<Points>").remove("</Points>").stripWhiteSpace().toInt();
    else if (s.contains("<Style>")){
      style = s.remove("<Style>").remove("</Style>").stripWhiteSpace().toInt();
      break;
    }
  }

  FunctionCurve *c = new FunctionCurve(type, title);
  c->setRange(start, end);
  c->setFormulas(formulas);
  c->setVariable(var);
  c->loadData(points);

  c_type.resize(++n_curves);
  c_type[n_curves-1] = style;
  c_keys.resize(n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(c);

  QStringList l;
  for (line++; line != lst.end(); ++line)
    l << *line;
  c->restoreCurveLayout(l);

  addLegendItem();
  updatePlot();
}

void Graph::createTable(const QString& curveName)
{
  if (curveName.isEmpty())
    return;

  const QwtPlotCurve* cv = curve(curveName);
  if (!cv)
    return;

  createTable(cv);
}

void Graph::createTable(const QwtPlotCurve* curve)
{
  if (!curve)
    return;

  int size = curve->dataSize();
  QString text = "1\t2\n";
  for (int i=0; i<size; i++)
  {
    text += QString::number(curve->x(i))+"\t";
    text += QString::number(curve->y(i))+"\n";
  }
  QString legend = tr("Data set generated from curve") + ": " + curve->title().text();
  emit createTable(tr("Table") + "1" + "\t" + legend, size, 2, text);
}

void Graph::updateMarkersBoundingRect()
{
  int lines = d_lines.size();
  int images = d_images.size();
  if (!lines && !images)
    return;

  for (int i=0; i<lines; i++){
    ArrowMarker* a = dynamic_cast<ArrowMarker*>(d_plot->marker(d_lines[i]));
    if (a)
      a->updateBoundingRect();
  }

  for (int i=0; i<images; i++){
    ImageMarker* im = dynamic_cast<ImageMarker*>(d_plot->marker(d_images[i]));
    if (im)
      im->updateBoundingRect();
  }
  d_plot->replot();
}

void Graph::resizeEvent ( QResizeEvent *e )
{
  if (ignoreResize || !this->isVisible())
    return;

  if (!autoScaleFonts){
    d_plot->resize(e->size());
    d_plot->updateCurveLabels();
  }
}

void Graph::scaleFonts(double factor)
{
  QObjectList lst = d_plot->children();
  foreach(QObject *o, lst){
    if (o->inherits("LegendWidget")){
      LegendWidget *lw = dynamic_cast<LegendWidget *>(o);
      if (!lw)
        continue;

      QFont font = lw->font();
      font.setPointSizeFloat(factor*font.pointSizeFloat());
      lw->setFont(font);
    }
  }

  for (int i = 0; i<QwtPlot::axisCnt; i++){
    QFont font = axisFont(i);
    font.setPointSizeFloat(factor*font.pointSizeFloat());
    d_plot->setAxisFont(i, font);

    QwtText title = d_plot->axisTitle(i);
    font = title.font();
    font.setPointSizeFloat(factor*font.pointSizeFloat());
    title.setFont(font);
    d_plot->setAxisTitle(i, title);
  }

  QwtText title = d_plot->title();
  QFont font = title.font();
  font.setPointSizeFloat(factor*font.pointSizeFloat());
  title.setFont(font);
  d_plot->setTitle(title);

  QList<QwtPlotItem *> curves = d_plot->curvesList();
  foreach(QwtPlotItem *i, curves){
    DataCurve *dc = dynamic_cast<DataCurve *>(i);
    if (dc && dc->rtti() != QwtPlotItem::Rtti_PlotSpectrogram &&
        dc->type() != Graph::Function &&
        dc->hasLabels()){
      QFont font = dc->labelsFont();
      font.setPointSizeFloat(factor*font.pointSizeFloat());
      dc->setLabelsFont(font);
      if (dc->hasSelectedLabels())
        notifyFontChange(font);
    }
  }
  d_plot->replot();
}

void Graph::setMargin (int d)
{
  if (d_plot->margin() == d)
    return;

  d_plot->setMargin(d);
  emit modifiedGraph();
}

void Graph::setFrame (int width, const QColor& color)
{
  if (d_plot->frameColor() == color && width == d_plot->lineWidth())
    return;

  QPalette pal = d_plot->palette();
  pal.setColor(QColorGroup::Foreground, color);
  d_plot->setPalette(pal);

  d_plot->setLineWidth(width);
}

void Graph::setBackgroundColor(const QColor& color)
{
  QColorGroup cg;
  QPalette p = d_plot->palette();
  p.setColor(QColorGroup::Window, color);
  d_plot->setPalette(p);

  d_plot->setAutoFillBackground(true);
  emit modifiedGraph();
}

void Graph::setCanvasBackground(const QColor& color)
{
  d_plot->setCanvasBackground(color);
  emit modifiedGraph();
}

QString Graph::penStyleName(Qt::PenStyle style)
{
  if (style==Qt::SolidLine)
    return "SolidLine";
  else if (style==Qt::DashLine)
    return "DashLine";
  else if (style==Qt::DotLine)
    return "DotLine";
  else if (style==Qt::DashDotLine)
    return "DashDotLine";
  else if (style==Qt::DashDotDotLine)
    return "DashDotDotLine";
  else
    return "SolidLine";
}

Qt::PenStyle Graph::getPenStyle(int style)
{
  Qt::PenStyle linePen = Qt::SolidLine;
  switch (style)
  {
  case 0:
    break;
  case 1:
    linePen=Qt::DashLine;
    break;
  case 2:
    linePen=Qt::DotLine;
    break;
  case 3:
    linePen=Qt::DashDotLine;
    break;
  case 4:
    linePen=Qt::DashDotDotLine;
    break;
  }
  return linePen;
}

Qt::PenStyle Graph::getPenStyle(const QString& s)
{
  Qt::PenStyle style = Qt::SolidLine;
  if (s == "DashLine")
    style=Qt::DashLine;
  else if (s == "DotLine")
    style=Qt::DotLine;
  else if (s == "DashDotLine")
    style=Qt::DashDotLine;
  else if (s == "DashDotDotLine")
    style=Qt::DashDotDotLine;
  return style;
}

int Graph::obsoleteSymbolStyle(int type)
{
  if (type <= 4)
    return type+1;
  else
    return type+2;
}

int Graph::curveType(int curveIndex)
{
  if (curveIndex < (int)c_type.size() && curveIndex >= 0)
    return c_type[curveIndex];
  else
    return -1;
}

void Graph::showPlotErrorMessage(QWidget *parent, const QStringList& emptyColumns)
{
  QApplication::restoreOverrideCursor();

  int n = (int)emptyColumns.count();
  if (n > 1)
  {
    QString columns;
    for (int i = 0; i < n; i++)
      columns += "<p><b>" + emptyColumns[i] + "</b></p>";

    QMessageBox::warning(parent, tr("MantidPlot - Warning"),
        tr("The columns") + ": " + columns + tr("are empty and will not be added to the plot!"));
  }
  else if (n == 1)
    QMessageBox::warning(parent, tr("MantidPlot - Warning"),
        tr("The column") + " <b>" + emptyColumns[0] + "</b> " + tr("is empty and will not be added to the plot!"));
}

void Graph::showTitleContextMenu()
{
  QMenu titleMenu(this);
  titleMenu.insertItem(getQPixmap("cut_xpm"), tr("&Cut"),this, SLOT(cutTitle()));
  titleMenu.insertItem(getQPixmap("copy_xpm"), tr("&Copy"),this, SLOT(copyTitle()));
  titleMenu.insertItem(tr("&Delete"),this, SLOT(removeTitle()));
  titleMenu.insertSeparator();
  titleMenu.insertItem(tr("&Properties..."), this, SIGNAL(viewTitleDialog()));
  titleMenu.exec(QCursor::pos());
}

void Graph::cutTitle()
{
  QApplication::clipboard()->setText(d_plot->title().text(), QClipboard::Clipboard);
  removeTitle();
}

void Graph::copyTitle()
{
  QApplication::clipboard()->setText(d_plot->title().text(), QClipboard::Clipboard);
}

void Graph::removeAxisTitle()
{
  int selectedAxis = scalePicker->currentAxis()->alignment();
  int axis = (selectedAxis + 2)%4;//unconsistent notation in Qwt enumerations between
  //QwtScaleDraw::alignment and QwtPlot::Axis
  d_plot->setAxisTitle(axis, " ");//due to the plot layout updates, we must always have a non empty title
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::cutAxisTitle()
{
  copyAxisTitle();
  removeAxisTitle();
}

void Graph::copyAxisTitle()
{
  int selectedAxis = scalePicker->currentAxis()->alignment();
  int axis = (selectedAxis + 2)%4;//unconsistent notation in Qwt enumerations between
  //QwtScaleDraw::alignment and QwtPlot::Axis
  QApplication::clipboard()->setText(d_plot->axisTitle(axis).text(), QClipboard::Clipboard);
}

void Graph::showAxisTitleMenu()
{
  QMenu titleMenu(this);
  titleMenu.insertItem(getQPixmap("cut_xpm"), tr("&Cut"), this, SLOT(cutAxisTitle()));
  titleMenu.insertItem(getQPixmap("copy_xpm"), tr("&Copy"), this, SLOT(copyAxisTitle()));
  titleMenu.insertItem(tr("&Delete"),this, SLOT(removeAxisTitle()));
  titleMenu.insertSeparator();
  titleMenu.insertItem(tr("&Properties..."), this, SIGNAL(showAxisTitleDialog()));
  titleMenu.exec(QCursor::pos());
}

void Graph::showAxisContextMenu(int axis)
{
  QMenu menu(this);
  menu.setCheckable(true);

  menu.insertItem(getQPixmap("unzoom_xpm"), tr("&Rescale to show all"), this, SLOT(setAutoScale()), tr("Ctrl+Shift+R"));
  menu.insertSeparator();
  menu.insertItem(tr("&Hide axis"), this, SLOT(hideSelectedAxis()));

  int gridsID = menu.insertItem(tr("&Show grids"), this, SLOT(showGrids()));
  if (axis == QwtScaleDraw::LeftScale || axis == QwtScaleDraw::RightScale){
    if (d_plot->grid()->yEnabled())
      menu.setItemChecked(gridsID, true);
  } else {
    if (d_plot->grid()->xEnabled())
      menu.setItemChecked(gridsID, true);
  }

  menu.insertSeparator();

  menu.insertItem(tr("&Scale..."), this, SLOT(showScaleDialog()));
  menu.insertItem(tr("&Properties..."), this, SLOT(showAxisDialog()));
  menu.exec(QCursor::pos());
}

void Graph::showAxisDialog()
{
  QwtScaleWidget *scale = scalePicker->currentAxis();
  if (scale)
    emit showAxisDialog(scale->alignment());
}

void Graph::showScaleDialog()
{
  emit axisDblClicked(scalePicker->currentAxis()->alignment());
}

void Graph::hideSelectedAxis()
{
  int axis = -1;
  int selectedAxis = scalePicker->currentAxis()->alignment();
  if (selectedAxis == QwtScaleDraw::LeftScale || selectedAxis == QwtScaleDraw::RightScale)
    axis = selectedAxis - 2;
  else
    axis = selectedAxis + 2;

  d_plot->enableAxis(axis, false);
  scalePicker->refresh();
  emit modifiedGraph();
}

void Graph::showGrids()
{
  showGrid (scalePicker->currentAxis()->alignment());
}

void Graph::showGrid()
{
  showGrid(QwtScaleDraw::LeftScale);
  showGrid(QwtScaleDraw::BottomScale);
}

void Graph::showGrid(int axis)
{
  Grid *grid = d_plot->grid();
  if (!grid)
    return;

  if (axis == QwtScaleDraw::LeftScale || axis == QwtScaleDraw::RightScale){
    grid->enableY(!grid->yEnabled());
    grid->enableYMin(!grid->yMinEnabled());
  } else if (axis == QwtScaleDraw::BottomScale || axis == QwtScaleDraw::TopScale){
    grid->enableX(!grid->xEnabled());
    grid->enableXMin(!grid->xMinEnabled());
  } else
    return;
  d_plot->replot();
  emit modifiedGraph();
}

void Graph::copy(Graph* g)
{
  d_waterfall_offset_x = g->waterfallXOffset();
  d_waterfall_offset_y = g->waterfallYOffset();

  Plot *plot = g->plotWidget();
  d_plot->setMargin(plot->margin());
  setBackgroundColor(plot->paletteBackgroundColor());
  setFrame(plot->lineWidth(), plot->frameColor());
  setCanvasBackground(plot->canvasBackground());

  for (int i = 0; i<QwtPlot::axisCnt; i++){
    if (plot->axisEnabled (i)){
      d_plot->enableAxis(i);
      QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(d_plot->axisWidget(i));
      if (scale){
        scale->setMargin(plot->axisWidget(i)->margin());
        QPalette pal = scale->palette();
        pal.setColor(QColorGroup::Foreground, g->axisColor(i));
        pal.setColor(QColorGroup::Text, g->axisLabelsColor(i));
        scale->setPalette(pal);
        d_plot->setAxisFont (i, plot->axisFont(i));

        QwtText src_axis_title = plot->axisTitle(i);
        QwtText title = scale->title();
        title.setText(src_axis_title.text());
        title.setColor(src_axis_title.color());
        title.setFont (src_axis_title.font());
        title.setRenderFlags(src_axis_title.renderFlags());
        scale->setTitle(title);
      }
    } else
      d_plot->enableAxis(i, false);
  }

  grid()->copy(g->grid());
  d_plot->setTitle (g->plotWidget()->title());
  setCanvasFrame(g->canvasFrameWidth(), g->canvasFrameColor());
  setAxesLinewidth(plot->axesLinewidth());
  removeLegend();

  for (int i=0; i<g->curves(); i++){
    QwtPlotItem *it = (QwtPlotItem *)g->plotItem(i);
    if (it->rtti() == QwtPlotItem::Rtti_PlotCurve){
      DataCurve *cv = dynamic_cast<DataCurve *>(it);
      if (!cv) continue;
      int n = cv->dataSize();
      int style = dynamic_cast<PlotCurve *>(it)->type();
      QVector<double> x(n);
      QVector<double> y(n);
      for (int j=0; j<n; j++){
        x[j]=cv->x(j);
        y[j]=cv->y(j);
      }

      PlotCurve *c = 0;
      c_keys.resize(++n_curves);
      c_type.resize(n_curves);
      c_type[i] = g->curveType(i);

      if (style == Pie){
        c = new QwtPieCurve(cv->table(), cv->title().text(), cv->startRow(), cv->endRow());
        c_keys[i] = d_plot->insertCurve(c);
      } else if (style == Function) {
        c = new FunctionCurve(cv->title().text());
        c_keys[i] = d_plot->insertCurve(c);
        dynamic_cast<FunctionCurve*>(c)->copy(dynamic_cast<FunctionCurve*>(cv));
      } else if (style == VerticalBars || style == HorizontalBars) {
        c = new QwtBarCurve(dynamic_cast<QwtBarCurve *>(cv)->orientation(), cv->table(), cv->xColumnName(),
            cv->title().text(), cv->startRow(), cv->endRow());
        c_keys[i] = d_plot->insertCurve(c);
        (dynamic_cast<QwtBarCurve *>(c))->copy(dynamic_cast<const QwtBarCurve *>(cv));
      } else if (style == ErrorBars) {
        QwtErrorPlotCurve *er = dynamic_cast<QwtErrorPlotCurve*>(cv);
        DataCurve *master_curve = masterCurve(er);
        if (master_curve) {
          c = new QwtErrorPlotCurve(cv->table(), cv->title().text());
          c_keys[i] = d_plot->insertCurve(c);
          dynamic_cast<QwtErrorPlotCurve*>(c)->copy(er);
          dynamic_cast<QwtErrorPlotCurve*>(c)->setMasterCurve(master_curve);
        }
      } else if (style == Histogram) {
        QwtHistogram *h = dynamic_cast<QwtHistogram *>(cv);
        if (h->matrix())
          c = new QwtHistogram(h->matrix());
        else
          c = new QwtHistogram(cv->table(), cv->xColumnName(), cv->title().text(), cv->startRow(), cv->endRow());
        c_keys[i] = d_plot->insertCurve(c);
        dynamic_cast<QwtHistogram*>(c)->copy(h);
      } else if (style == VectXYXY || style == VectXYAM) {
        VectorCurve::VectorStyle vs = VectorCurve::XYXY;
        if (style == VectXYAM)
          vs = VectorCurve::XYAM;
        c = new VectorCurve(vs, cv->table(), cv->xColumnName(), cv->title().text(),
            dynamic_cast<VectorCurve*>(cv)->vectorEndXAColName(),
            dynamic_cast<VectorCurve*>(cv)->vectorEndYMColName(),
            cv->startRow(), cv->endRow());
        c_keys[i] = d_plot->insertCurve(c);
        dynamic_cast<VectorCurve*>(c)->copy(dynamic_cast<const VectorCurve *>(cv));
      } else if (style == Box) {
        c = new BoxCurve(cv->table(), cv->title().text(), cv->startRow(), cv->endRow());
        c_keys[i] = d_plot->insertCurve(c);
        dynamic_cast<BoxCurve*>(c)->copy(dynamic_cast<const BoxCurve *>(cv));
        QwtSingleArrayData dat(x[0], y, n);
        c->setData(dat);
      } else {
        c = new DataCurve(cv->table(), cv->xColumnName(), cv->title().text(), cv->startRow(), cv->endRow());
        c_keys[i] = d_plot->insertCurve(c);
      }

      if (c_type[i] != Box && c_type[i] != ErrorBars){
        c->setData(x.data(), y.data(), n);
        if (c->type() != Function && c->type() != Pie)
          dynamic_cast<DataCurve*>(c)->clone(cv);
        else if (c->type() == Pie)
          dynamic_cast<QwtPieCurve*>(c)->clone(dynamic_cast<QwtPieCurve*>(cv));
      }

      c->setPen(cv->pen());
      c->setBrush(cv->brush());
      c->setStyle(cv->style());
      c->setSymbol(cv->symbol());

      if (cv->testCurveAttribute (QwtPlotCurve::Fitted))
        c->setCurveAttribute(QwtPlotCurve::Fitted, true);
      else if (cv->testCurveAttribute (QwtPlotCurve::Inverted))
        c->setCurveAttribute(QwtPlotCurve::Inverted, true);

      c->setAxis(cv->xAxis(), cv->yAxis());
      c->setVisible(cv->isVisible());

      QList<QwtPlotCurve *>lst = g->fitCurvesList();
      if (lst.contains(dynamic_cast<QwtPlotCurve *>(it)))
        d_fit_curves << c;
    }else if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
      Spectrogram *sp = (dynamic_cast<Spectrogram *>(it))->copy();
      c_keys.resize(++n_curves);
      c_keys[i] = d_plot->insertCurve(sp);


      QwtScaleWidget *rightAxis = sp->plot()->axisWidget(QwtPlot::yRight);
      if (g->curveType(i) == ColorMap)
        rightAxis->setColorBarEnabled(true);
      else
        rightAxis->setColorBarEnabled(false);
      sp->plot()->enableAxis(QwtPlot::yRight, true);
      sp->mutableColorMap().changeScaleType(sp->getColorMap().getScaleType());

      rightAxis->setColorMap(sp->data().range(),sp->mutableColorMap());
      sp->plot()->setAxisScale(QwtPlot::yRight,
          sp->data().range().minValue(),
          sp->data().range().maxValue());
      sp->plot()->setAxisScaleDiv(QwtPlot::yRight, *sp->plot()->axisScaleDiv(QwtPlot::yRight));

      //sp->showColorScale((dynamic_cast<Spectrogram *>(it))->colorScaleAxis(), (dynamic_cast<Spectrogram *>(it))->hasColorScale());
      /* sp->setColorBarWidth((dynamic_cast<Spectrogram *>(it))->colorBarWidth());
      sp->setVisible(it->isVisible());*/

      c_type.resize(n_curves);
      c_type[i] = g->curveType(i);
    }
  }

  for (int i=0; i<QwtPlot::axisCnt; i++){
    QwtScaleWidget *sc = g->plotWidget()->axisWidget(i);
    if (!sc)
      continue;

    ScaleDraw *sdg = dynamic_cast<ScaleDraw *>(g->plotWidget()->axisScaleDraw (i));
    if (sdg->hasComponent(QwtAbstractScaleDraw::Labels))
    {
      ScaleDraw::ScaleType type = sdg->scaleType();
      if (type == ScaleDraw::Numeric)
        setLabelsNumericFormat(i, plot->axisLabelFormat(i), plot->axisLabelPrecision(i), sdg->formula());
      else if (type == ScaleDraw::Day)
        setLabelsDayFormat(i, sdg->nameFormat());
      else if (type == ScaleDraw::Month)
        setLabelsMonthFormat(i, sdg->nameFormat());
      else if (type == ScaleDraw::Time || type == ScaleDraw::Date)
        setLabelsDateTimeFormat(i, type, sdg->formatString());
      else{
        ScaleDraw *sd = dynamic_cast<ScaleDraw *>(plot->axisScaleDraw(i));
        d_plot->setAxisScaleDraw(i, new ScaleDraw(d_plot, sd->labelsList(), sd->formatString(), sd->scaleType()));
      }
    } else {
      ScaleDraw *sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw (i));
      sd->enableComponent (QwtAbstractScaleDraw::Labels, false);
    }
  }
  for (int i=0; i<QwtPlot::axisCnt; i++){//set same scales
    const ScaleEngine *se = dynamic_cast<ScaleEngine *>(plot->axisScaleEngine(i));
    if (!se)
      continue;

    ScaleEngine *sc_engine = dynamic_cast<ScaleEngine *>(d_plot->axisScaleEngine(i));
    sc_engine->clone(se);

    int majorTicks = plot->axisMaxMajor(i);
    int minorTicks = plot->axisMaxMinor(i);
    d_plot->setAxisMaxMajor (i, majorTicks);
    d_plot->setAxisMaxMinor (i, minorTicks);

    double step = g->axisStep(i);
    d_user_step[i] = step;
    const QwtScaleDiv *sd = plot->axisScaleDiv(i);
    QwtScaleDiv div = sc_engine->divideScale (QMIN(sd->lBound(), sd->hBound()),
        QMAX(sd->lBound(), sd->hBound()), majorTicks, minorTicks, step);

    if (se->testAttribute(QwtScaleEngine::Inverted))
      div.invert();
    d_plot->setAxisScaleDiv (i, div);
  }

  drawAxesBackbones(g->drawAxesBackbone);
  setMajorTicksType(g->plotWidget()->getMajorTicksType());
  setMinorTicksType(g->plotWidget()->getMinorTicksType());
  setTicksLength(g->minorTickLength(), g->majorTickLength());

  setAxisLabelRotation(QwtPlot::xBottom, g->labelsRotation(QwtPlot::xBottom));
  setAxisLabelRotation(QwtPlot::xTop, g->labelsRotation(QwtPlot::xTop));

  QVector<int> imag = g->imageMarkerKeys();
  for (int i=0; i<(int)imag.size(); i++)
    addImage(dynamic_cast<ImageMarker*>(g->imageMarker(imag[i])));

  QList<LegendWidget *> texts = g->textsList();
  foreach (LegendWidget *t, texts){
    if (t == g->legend())
      d_legend = insertText(t);
    else if (t->isA("PieLabel")){
      QwtPieCurve *pie = dynamic_cast<QwtPieCurve*>(curve(0));
      if (pie)
        pie->addLabel(dynamic_cast<PieLabel *>(t), true);
      else
        insertText(t);
    } else
      insertText(t);
  }

  QVector<int> l = g->lineMarkerKeys();
  for (int i=0; i<(int)l.size(); i++){
    ArrowMarker* lmrk=dynamic_cast<ArrowMarker*>(g->arrow(l[i]));
    if (lmrk)
      addArrow(lmrk);
  }
  setAntialiasing(g->antialiasing(), true);
  d_synchronize_scales = g->hasSynchronizedScaleDivisions();
  d_plot->replot();
}

void Graph::plotBoxDiagram(Table *w, const QStringList& names, int startRow, int endRow)
{
  if (endRow < 0)
    endRow = w->numRows() - 1;

  for (int j = 0; j <(int)names.count(); j++){
    BoxCurve *c = new BoxCurve(w, names[j], startRow, endRow);

    c_keys.resize(++n_curves);
    c_keys[n_curves-1] = d_plot->insertCurve(c);
    c_type.resize(n_curves);
    c_type[n_curves-1] = Box;

    c->setData(QwtSingleArrayData(double(j+1), QwtArray<double>(), 0));
    c->loadData();

    c->setPen(QPen(ColorBox::color(j), 1));
    c->setSymbol(QwtSymbol(QwtSymbol::NoSymbol, QBrush(), QPen(ColorBox::color(j), 1), QSize(7,7)));
  }

  if (d_legend)
    d_legend->setText(legendText());

  d_plot->setAxisScaleDraw (QwtPlot::xBottom, new ScaleDraw(d_plot, w->selectedYLabels(), w->objectName(), ScaleDraw::ColHeader));
  d_plot->setAxisMaxMajor(QwtPlot::xBottom, names.count()+1);
  d_plot->setAxisMaxMinor(QwtPlot::xBottom, 0);

  d_plot->setAxisScaleDraw (QwtPlot::xTop, new ScaleDraw(d_plot, w->selectedYLabels(), w->objectName(), ScaleDraw::ColHeader));
  d_plot->setAxisMaxMajor(QwtPlot::xTop, names.count()+1);
  d_plot->setAxisMaxMinor(QwtPlot::xTop, 0);
}

void Graph::setCurveStyle(int index, int s)
{
  QwtPlotCurve *c = curve(index);
  if (!c)
    return;

  int curve_type = c_type[index];
  if (curve_type == VerticalBars || curve_type == HorizontalBars || curve_type == Histogram ||
      curve_type == Pie || curve_type == Box || curve_type == ErrorBars ||
      curve_type == VectXYXY || curve_type == VectXYAM)
    return;//these are not line styles, but distinct curve types and this function must not change the curve type

  c->setCurveAttribute(QwtPlotCurve::Fitted, false);
  c->setCurveAttribute(QwtPlotCurve::Inverted, false);

  if (s == 5){//ancient spline style in Qwt 4.2.0
    s = QwtPlotCurve::Lines;
    c->setCurveAttribute(QwtPlotCurve::Fitted, true);
    c_type[index] = Spline;
  } else if (s == 6){// Vertical Steps
    s = QwtPlotCurve::Steps;
    c->setCurveAttribute(QwtPlotCurve::Inverted, false);
    c_type[index] = VerticalSteps;
  } else if (s == QwtPlotCurve::Steps){// Horizontal Steps
    c_type[index] = HorizontalSteps;
    c->setCurveAttribute(QwtPlotCurve::Inverted, true);
  } else if (s == QwtPlotCurve::Sticks)
    c_type[index] = VerticalDropLines;
  else {//QwtPlotCurve::Lines || QwtPlotCurve::Dots
    if (c->symbol().style() == QwtSymbol::NoSymbol)
      c_type[index] = Line;
    else if (c->symbol().style() != QwtSymbol::NoSymbol && (QwtPlotCurve::CurveStyle)s == QwtPlotCurve::NoCurve)
      c_type[index] = Scatter;
    else
      c_type[index] = LineSymbols;
  }

  c->setStyle((QwtPlotCurve::CurveStyle)s);
}

void Graph::setCurveSymbol(int index, const QwtSymbol& s)
{
  QwtPlotCurve *c = curve(index);
  if (!c)
    return;

  c->setSymbol(s);
}

void Graph::setCurvePen(int index, const QPen& p)
{
  QwtPlotCurve *c = curve(index);
  if (!c)
    return;

  c->setPen(p);
}

void Graph::setCurveBrush(int index, const QBrush& b)
{
  QwtPlotCurve *c = curve(index);
  if (!c)
    return;

  c->setBrush(b);
}

void Graph::setCurveSkipSymbolsCount(int index, int count)
{
  PlotCurve *c = dynamic_cast<PlotCurve*>(curve(index));
  if (!c)
    return;

  c->setSkipSymbolsCount(count);
}

BoxCurve* Graph::openBoxDiagram(Table *w, const QStringList& l, int fileVersion)
{
  if (!w)
    return NULL;

  int startRow = 0;
  int endRow = w->numRows()-1;
  if (fileVersion >= 90) {
    startRow = l[l.count()-3].toInt();
    endRow = l[l.count()-2].toInt();
  }

  BoxCurve *c = new BoxCurve(w, l[2], startRow, endRow);

  c_keys.resize(++n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(c);
  c_type.resize(n_curves);
  c_type[n_curves-1] = Box;

  c->setData(QwtSingleArrayData(l[1].toDouble(), QwtArray<double>(), 0));
  c->setData(QwtSingleArrayData(l[1].toDouble(), QwtArray<double>(), 0));
  c->loadData();

  c->setMaxStyle(SymbolBox::style(l[16].toInt()));
  c->setP99Style(SymbolBox::style(l[17].toInt()));
  c->setMeanStyle(SymbolBox::style(l[18].toInt()));
  c->setP1Style(SymbolBox::style(l[19].toInt()));
  c->setMinStyle(SymbolBox::style(l[20].toInt()));

  c->setBoxStyle(l[21].toInt());
  c->setBoxWidth(l[22].toInt());
  c->setBoxRange(l[23].toInt(), l[24].toDouble());
  c->setWhiskersRange(l[25].toInt(), l[26].toDouble());
  return c;
}

void Graph::setActiveTool(PlotToolInterface *tool)
{
  if (tool && tool->rtti() == PlotToolInterface::Rtti_MultiPeakFitTool){
    if (d_range_selector)
      d_range_selector->setEnabled(false);
    return;
  }

  if(d_active_tool)
    delete d_active_tool;

  d_active_tool=tool;
}

void Graph::disableTools()
{
  if (zoomOn())
    zoom(false);
  enablePanningMagnifier(false);
  if (drawLineActive())
    drawLine(false);

  if(d_active_tool)
    delete d_active_tool;
  d_active_tool = NULL;

  if (d_range_selector)
    delete d_range_selector;
  d_range_selector = NULL;
}

bool Graph::enableRangeSelectors(const QObject *status_target, const char *status_slot)
{
  if (d_range_selector){
    delete d_range_selector;
    d_range_selector = NULL;
  }
  d_range_selector = new RangeSelectorTool(this, status_target, status_slot);
  setActiveTool(d_range_selector);
  connect(d_range_selector, SIGNAL(changed()), this, SIGNAL(dataRangeChanged()));
  return true;
}

void Graph::guessUniqueCurveLayout(int& colorIndex, int& symbolIndex)
{
  colorIndex = 0;
  symbolIndex = 0;

  int curve_index = n_curves - 1;
  if (curve_index >= 0 && c_type[curve_index] == ErrorBars)
  {// find out the pen color of the master curve
    QwtErrorPlotCurve *er = dynamic_cast<QwtErrorPlotCurve *>(d_plot->curve(c_keys[curve_index]));
    DataCurve *master_curve = er->masterCurve();
    if (master_curve){
      colorIndex = ColorBox::colorIndex(master_curve->pen().color());
      return;
    }
  }

  for (int i=0; i<n_curves; ++i)
  {
    const PlotCurve *c = dynamic_cast<PlotCurve *>(curve(i));
    if (c)
    {
      colorIndex = std::max(ColorBox::colorIndex(c->pen().color()), colorIndex);

      QwtSymbol symb = c->symbol();
      symbolIndex = std::max(SymbolBox::symbolIndex(symb.style()), symbolIndex);
    }
  }
  if (n_curves > 1)
    colorIndex = (colorIndex+1)%ColorBox::numPredefinedColors();
  if (ColorBox::color(colorIndex) == Qt::white) //avoid white invisible curves
    ++colorIndex;

  symbolIndex = (symbolIndex+1)%15;
  if (symbolIndex == 0)
    ++symbolIndex;
}

void Graph::addFitCurve(QwtPlotCurve *c)
{
  if (c)
    d_fit_curves << c;
}

void Graph::deleteFitCurves()
{
  QList<int> keys = d_plot->curveKeys();
  foreach(QwtPlotCurve *c, d_fit_curves)
  removeCurve(curveIndex(c));

  d_plot->replot();
}

Spectrogram* Graph::plotSpectrogram(Matrix *m, CurveType type)
{
  if (type != GrayScale && type != ColorMap && type != Contour)
    return 0;

  Spectrogram *d_spectrogram = new Spectrogram(m);
  return plotSpectrogram(d_spectrogram,type);
}
Spectrogram* Graph::plotSpectrogram(Function2D *f,int nrows, int ncols,double left, double top, double width, double height,double minz,double maxz, CurveType type)
{
  if (type != GrayScale && type != ColorMap && type != Contour)
    return 0;

  Spectrogram *d_spectrogram = new Spectrogram(f,nrows,ncols,left,top,width,height,minz,maxz);

  return plotSpectrogram(d_spectrogram,type);
}

/*
 * Returns true if the graph has a 2D plot
 *
 * NOTE: returns false if we are running an old version of QWT (pre-5.2.0)
 *
 */
bool Graph::isSpectrogram()
{
#if QWT_VERSION >= 0x050200
  for (int i=0; i<c_type.count(); i++)
  {
    if (!(c_type[i] == GrayScale || c_type[i] == ColorMap || c_type[i] == Contour || c_type[i] == ColorMapContour))
      return false;
  }
  return true;
#else
  return false;
#endif

}

/** Returns a pointer to a 2D plot, if the Graph has one (if more than one, will return the first).
 *  Otherwise, returns a null pointer.
 */
Spectrogram* Graph::spectrogram()
{
  foreach (QwtPlotItem *item, d_plot->curves())
  {
    if(item && item->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
    {
      Spectrogram *s = dynamic_cast<Spectrogram *>(item);
      if (s) return s;
    }
  }
  return NULL;

}

Spectrogram* Graph::plotSpectrogram(Function2D *f,int nrows, int ncols,QwtDoubleRect bRect,double minz,double maxz,CurveType type)
{
  if (type != GrayScale && type != ColorMap && type != Contour && type != ColorMapContour)
    return 0;

  Spectrogram *d_spectrogram = new Spectrogram(f,nrows,ncols,bRect,minz,maxz);

  return plotSpectrogram(d_spectrogram,type);
}

Spectrogram* Graph::plotSpectrogram(Spectrogram *d_spectrogram, CurveType type)
{
  if (type == GrayScale)
    d_spectrogram->setGrayScale();

  else if (type == Contour)
  {
    d_spectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, false);
    d_spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, true);
  }
  else if (type == ColorMap)
  {
    d_spectrogram->mutableColorMap().changeScaleType(GraphOptions::Linear);
    d_spectrogram->setDefaultColorMap();
    d_spectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
    d_spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, false);
  }
  else if (type == ColorMapContour)
  {
    d_spectrogram->mutableColorMap().changeScaleType(GraphOptions::Linear);
    d_spectrogram->setDefaultColorMap();
    d_spectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
    d_spectrogram->setDisplayMode(QwtPlotSpectrogram::ContourMode, true);
  }
  c_keys.resize(++n_curves);
  c_keys[n_curves-1] = d_plot->insertCurve(d_spectrogram);

  c_type.resize(n_curves);
  c_type[n_curves-1] = type;

  QwtScaleWidget *rightAxis = d_plot->axisWidget(QwtPlot::yRight);
  if(!rightAxis) return 0;
  rightAxis->setColorBarEnabled(type != Contour);
  d_plot->enableAxis(QwtPlot::yRight, type != Contour);
  // Ensure that labels are shown on color scale axis
  enableAxisLabels(QwtPlot::yRight);

  //d_spectrogram->setDefaultColorMap();
  if(type == GrayScale) rightAxis->setColorBarEnabled(false); //rightAxis->setColorMap(d_spectrogram->data().range(),d_spectrogram->colorMap());
  else rightAxis->setColorMap(d_spectrogram->data().range(),d_spectrogram->mutableColorMap());
  d_plot->setAxisScale(QwtPlot::yRight,
      d_spectrogram->data().range().minValue(),
      d_spectrogram->data().range().maxValue());


  d_plot->setAxisScaleDiv(QwtPlot::yRight, *d_plot->axisScaleDiv(QwtPlot::yRight));

  for (int i=0; i < QwtPlot::axisCnt; i++)
  {updatedaxis.push_back(0);  }

  enableFixedAspectRatio(multiLayer()->applicationWindow()->fixedAspectRatio2DPlots);
  return d_spectrogram;
}

void Graph::restoreCurveLabels(int curveID, const QStringList& lst)
{
  DataCurve *c = dynamic_cast<DataCurve *>(curve(curveID));
  if (!c)
    return;

  QString labelsColumn = QString();
  int xoffset = 0, yoffset = 0;
  QStringList::const_iterator line = lst.begin();
  QString s = *line;
  if (s.contains("<column>"))
    labelsColumn = s.remove("<column>").remove("</column>").trimmed();

  for (line++; line != lst.end(); ++line){
    s = *line;
    if (s.contains("<color>"))
      c->setLabelsColor(QColor(s.remove("<color>").remove("</color>").trimmed()));
    else if (s.contains("<whiteOut>"))
      c->setLabelsWhiteOut(s.remove("<whiteOut>").remove("</whiteOut>").toInt());
    else if (s.contains("<font>")){
      QStringList fontList = s.remove("<font>").remove("</font>").trimmed().split("\t");
      QFont font = QFont(fontList[0], fontList[1].toInt());
      if (fontList.count() >= 3)
        font.setBold(fontList[2].toInt());
      if (fontList.count() >= 4)
        font.setItalic(fontList[3].toInt());
      if (fontList.count() >= 5)
        font.setUnderline(fontList[4].toInt());
      c->setLabelsFont(font);
    } else if (s.contains("<angle>"))
      c->setLabelsRotation(s.remove("<angle>").remove("</angle>").toDouble());
    else if (s.contains("<justify>"))
      c->setLabelsAlignment(s.remove("<justify>").remove("</justify>").toInt());
    else if (s.contains("<xoffset>"))
      xoffset = s.remove("<xoffset>").remove("</xoffset>").toInt();
    else if (s.contains("<yoffset>"))
      yoffset = s.remove("<yoffset>").remove("</yoffset>").toInt();
  }
  c->setLabelsOffset(xoffset, yoffset);
  c->setLabelsColumnName(labelsColumn);
}

bool Graph::validCurvesDataSize()
{
  if (!n_curves){
    QMessageBox::warning(this, tr("MantidPlot - Warning"), tr("There are no curves available on this plot!"));
    return false;
  } else {
    for (int i=0; i < n_curves; i++){
      QwtPlotItem *item = curve(i);
      if(item && item->rtti() != QwtPlotItem::Rtti_PlotSpectrogram){
        QwtPlotCurve *c = dynamic_cast<QwtPlotCurve*>(item);
        if (c->dataSize() >= 2)
          return true;
      }
    }
    QMessageBox::warning(this, tr("MantidPlot - Error"),
        tr("There are no curves with more than two points on this plot. Operation aborted!"));
    return false;
  }
}

Graph::~Graph()
{
  setActiveTool(NULL);
  if (d_range_selector)
    delete d_range_selector;
  if(d_peak_fit_tool)
    delete d_peak_fit_tool;
  if (d_magnifier)
    delete d_magnifier;
  if (d_panner)
    delete d_panner;
  delete titlePicker;
  delete scalePicker;
  delete cp;
  delete d_plot;
}

void Graph::setAntialiasing(bool on, bool update)
{
  if (d_antialiasing == on)
    return;

  d_antialiasing = on;

  if (update){
    QList<int> curve_keys = d_plot->curveKeys();
    for (int i=0; i<(int)curve_keys.count(); i++)
    {
      QwtPlotItem *c = d_plot->curve(curve_keys[i]);
      if (c)
        c->setRenderHint(QwtPlotItem::RenderAntialiased, d_antialiasing);
    }
    QList<int> marker_keys = d_plot->markerKeys();
    for (int i=0; i<(int)marker_keys.count(); i++)
    {
      QwtPlotMarker *m = d_plot->marker(marker_keys[i]);
      if (m)
        m->setRenderHint(QwtPlotItem::RenderAntialiased, d_antialiasing);
    }

    d_plot->replot();
  }
}

bool Graph::focusNextPrevChild ( bool )
{
  QList<int> mrkKeys = d_plot->markerKeys();
  int n = mrkKeys.size();
  if (n < 2)
    return false;

  int min_key = mrkKeys[0], max_key = mrkKeys[0];
  for (int i = 0; i<n; i++ )
  {
    if (mrkKeys[i] >= max_key)
      max_key = mrkKeys[i];
    if (mrkKeys[i] <= min_key)
      min_key = mrkKeys[i];
  }

  int key = selectedMarker;
  if (key >= 0)
  {
    key++;
    if ( key > max_key )
      key = min_key;
  } else
    key = min_key;

  cp->disableEditing();

  setSelectedMarker(key);
  return true;
}

QString Graph::axisFormatInfo(int axis)
{
  if (axis < 0 || axis > QwtPlot::axisCnt)
    return QString();

  ScaleDraw *sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis));
  if (sd)
    return sd->formatString();
  else
    return "Not available!";
}

void Graph::updateCurveNames(const QString& oldName, const QString& newName, bool updateTableName)
{
  //update plotted curves list
  QList<int> keys = d_plot->curveKeys();
  for (int i=0; i<(int)keys.count(); i++){
    QwtPlotItem *it = d_plot->plotItem(keys[i]);
    if (!it)
      continue;
    if (it->rtti() != QwtPlotItem::Rtti_PlotCurve)
      continue;

    DataCurve *c = dynamic_cast<DataCurve *>(it);
    if (c && c->type() != Function && c->plotAssociation().contains(oldName))
      c->updateColumnNames(oldName, newName, updateTableName);
  }
  d_plot->replot();
}

void Graph::setCurveFullRange(int curveIndex)
{
  DataCurve *c = dynamic_cast<DataCurve *>(curve(curveIndex));
  if (c)
  {
    c->setFullRange();
    updatePlot();
    emit modifiedGraph();
  }
}

void Graph::setCurveLineColor(int curveIndex, int colorIndex)
{
  QwtPlotCurve *c = curve(curveIndex);
  if (c){
    QPen pen = c->pen();
    pen.setColor(ColorBox::defaultColor(colorIndex));
    c->setPen(pen);
    replot();
    emit modifiedGraph();
  }
}

void Graph::setCurveLineColor(int curveIndex, QColor qColor)
{
  QwtPlotCurve *c = curve(curveIndex);
  if (c){
    QPen pen = c->pen();
    pen.setColor(qColor);
    c->setPen(pen);
    replot();
    emit modifiedGraph();
  }
}

void Graph::setCurveLineStyle(int curveIndex, Qt::PenStyle style)
{
  QwtPlotCurve *c = curve(curveIndex);
  if (c){
    QPen pen = c->pen();
    pen.setStyle(style);
    c->setPen(pen);
    replot();
    emit modifiedGraph();
  }
}

void Graph::setCurveLineWidth(int curveIndex, double width)
{
  QwtPlotCurve *c = curve(curveIndex);
  if (c){
    QPen pen = c->pen();
    pen.setWidthF(width);
    c->setPen(pen);
    replot();
    emit modifiedGraph();
  }
}

void Graph::setGrayScale()
{
  if (isPiePlot())
    return;

  int curves = d_plot->curvesList().size(); //QtiPlot: d_curves.size();
  int dv = int(255 / double(curves));
  // QtiPlot: int i = 0;
  QColor color = Qt::black;
  int hue = color.hue();
  for (int i = 0; i < curves; i++) // QtiPlot: foreach (QwtPlotItem *it, d_curves)
  {
    QwtPlotItem * it = plotItem(i);
    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
    {
      Spectrogram *spec = dynamic_cast<Spectrogram*>(it);
      if (spec)
        spec->setGrayScale();
      continue;
    }

    PlotCurve *c = dynamic_cast<PlotCurve*>(it);
    if (!c || c->type() == ErrorBars)
      continue;

    QPen pen = c->pen();
    if (i)
    {
      int v = i * dv;
      if (v > 255)
        v = 0;
      color = QColor::fromHsv(hue, 0, v);
    }
    pen.setColor(color);
    c->setPen(pen);

    QBrush brush = c->brush();
    if (brush.style() != Qt::NoBrush)
    {
      brush.setColor(color);
      c->setBrush(brush);
    }

    QwtSymbol symbol = c->symbol();
    pen = symbol.pen();
    pen.setColor(color);
    symbol.setPen(pen);
    if (symbol.brush().style() != Qt::NoBrush)
      symbol.setBrush(QBrush(color));
    c->setSymbol(symbol);
    // QtiPlot: i++;
  }

  for (int i = 0; i < curves; i++) // QtiPlot: foreach (QwtPlotItem *it, d_curves)
  {
    QwtPlotItem * it = plotItem(i);
    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
      continue;

    PlotCurve *c = dynamic_cast<PlotCurve*>(it);
    if (c && c->type() == ErrorBars)
    {
      // QtiPlot: ErrorBarsCurve *er = (ErrorBarsCurve *) it;
      QwtErrorPlotCurve *er = dynamic_cast<QwtErrorPlotCurve*>(it);
      if (!er)
        continue;
      DataCurve* mc = er->masterCurve();
      if (mc)
        er->setColor(mc->pen().color());
    }
  }

  replot();
  emit modifiedGraph();
}

void Graph::setIndexedColors()
{
  QList<QColor> colors(ColorBox::defaultColors());
// QtiPlot:
//  MultiLayer *ml = multiLayer();
//  if (ml && ml->applicationWindow())
//    colors = ml->applicationWindow()->indexedColors();
//  else
//    colors = ColorBox::defaultColors();

  // QtiPlot: int i = 0;
  int curves = d_plot->curvesList().size();
  for (int i = 0; i < curves; i++) // QtiPlot: foreach (QwtPlotItem *it, d_curves)
  {
    QwtPlotItem * it = plotItem(i);
    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
      continue;

    PlotCurve *c = dynamic_cast<PlotCurve*>(it);
    if (!c || c->type() == ErrorBars)
      continue;

    QPen pen = c->pen();
    QColor color = colors[i];
    pen.setColor(color);
    c->setPen(pen);

    QBrush brush = c->brush();
    if (brush.style() != Qt::NoBrush)
    {
      brush.setColor(color);
      c->setBrush(brush);
    }

    QwtSymbol symbol = c->symbol();
    pen = symbol.pen();
    pen.setColor(color);
    symbol.setPen(pen);
    if (symbol.brush().style() != Qt::NoBrush)
      symbol.setBrush(QBrush(color));
    c->setSymbol(symbol);
    // QtiPlot: i++;
  }

  for (int i = 0; i < curves; i++) // QtiPlot: foreach (QwtPlotItem *it, d_curves)
  {
    QwtPlotItem * it = plotItem(i);
    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
      continue;

    PlotCurve *c = dynamic_cast<PlotCurve*>(it);
    if (c->type() == ErrorBars)
    {
      QwtErrorPlotCurve *er = dynamic_cast<QwtErrorPlotCurve*>(it); // QtiPlot: ErrorBarsCurve *er = (ErrorBarsCurve *) it;
      DataCurve* mc = er->masterCurve();
      if (mc)
        er->setColor(mc->pen().color());
    }
  }

  replot();
  emit modifiedGraph();
}

DataCurve* Graph::masterCurve(QwtErrorPlotCurve *er)
{
  QList<int> keys = d_plot->curveKeys();
  for (int i=0; i<(int)keys.count(); i++)
  {
    QwtPlotItem *it = d_plot->plotItem(keys[i]);
    if (!it)
      continue;
    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
      continue;
    PlotCurve *pc = dynamic_cast<PlotCurve *>(it);
    if (!pc || pc->type() == Function)
      continue;

    DataCurve* dc = dynamic_cast<DataCurve *>(it);
    if (!dc) return 0;
    if (dc->plotAssociation() == er->masterCurve()->plotAssociation())
      return dc;
  }
  return 0;
}

DataCurve* Graph::masterCurve(const QString& xColName, const QString& yColName)
{
  QString master_curve = xColName + "(X)," + yColName + "(Y)";
  QList<int> keys = d_plot->curveKeys();
  for (int i=0; i<(int)keys.count(); i++)
  {
    QwtPlotItem *it = d_plot->plotItem(keys[i]);
    if (!it)
      continue;
    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
      continue;
    PlotCurve *pc = dynamic_cast<PlotCurve *>(it);
    if (!pc || pc->type() == Function)
      continue;

    DataCurve* dc = dynamic_cast<DataCurve *>(it);
    if (!dc) return 0;
    if (dc->plotAssociation() == master_curve)
      return dc;
  }
  return 0;
}

void Graph::showCurve(int index, bool visible)
{
  QwtPlotItem *it = plotItem(index);
  if (it)
    it->setVisible(visible);

  emit modifiedGraph();
}

int Graph::visibleCurves()
{
  int c = 0;
  QList<int> keys = d_plot->curveKeys();
  for (int i=0; i<(int)keys.count(); i++)
  {
    QwtPlotItem *it = d_plot->plotItem(keys[i]);
    if (it && it->isVisible())
      c++;
  }
  return c;
}

QPrinter::PageSize Graph::minPageSize(const QPrinter& printer, const QRect& r)
{
  double x_margin = 0.2/2.54*printer.logicalDpiX(); // 2 mm margins
  double y_margin = 0.2/2.54*printer.logicalDpiY();
  double w_mm = 2*x_margin + (double)(r.width())/(double)printer.logicalDpiX() * 25.4;
  double h_mm = 2*y_margin + (double)(r.height())/(double)printer.logicalDpiY() * 25.4;

  int w, h;
  if (w_mm/h_mm > 1)
  {
    w = (int)ceil(w_mm);
    h = (int)ceil(h_mm);
  }
  else
  {
    h = (int)ceil(w_mm);
    w = (int)ceil(h_mm);
  }

  QPrinter::PageSize size = QPrinter::A5;
  if (w < 45 && h < 32)
    size =  QPrinter::B10;
  else if (w < 52 && h < 37)
    size =  QPrinter::A9;
  else if (w < 64 && h < 45)
    size =  QPrinter::B9;
  else if (w < 74 && h < 52)
    size =  QPrinter::A8;
  else if (w < 91 && h < 64)
    size =  QPrinter::B8;
  else if (w < 105 && h < 74)
    size =  QPrinter::A7;
  else if (w < 128 && h < 91)
    size =  QPrinter::B7;
  else if (w < 148 && h < 105)
    size =  QPrinter::A6;
  else if (w < 182 && h < 128)
    size =  QPrinter::B6;
  else if (w < 210 && h < 148)
    size =  QPrinter::A5;
  else if (w < 220 && h < 110)
    size =  QPrinter::DLE;
  else if (w < 229 && h < 163)
    size =  QPrinter::C5E;
  else if (w < 241 && h < 105)
    size =  QPrinter::Comm10E;
  else if (w < 257 && h < 182)
    size =  QPrinter::B5;
  else if (w < 279 && h < 216)
    size =  QPrinter::Letter;
  else if (w < 297 && h < 210)
    size =  QPrinter::A4;
  else if (w < 330 && h < 210)
    size =  QPrinter::Folio;
  else if (w < 356 && h < 216)
    size =  QPrinter::Legal;
  else if (w < 364 && h < 257)
    size =  QPrinter::B4;
  else if (w < 420 && h < 297)
    size =  QPrinter::A3;
  else if (w < 515 && h < 364)
    size =  QPrinter::B3;
  else if (w < 594 && h < 420)
    size =  QPrinter::A2;
  else if (w < 728 && h < 515)
    size =  QPrinter::B2;
  else if (w < 841 && h < 594)
    size =  QPrinter::A1;
  else if (w < 1030 && h < 728)
    size =  QPrinter::B1;
  else if (w < 1189 && h < 841)
    size =  QPrinter::A0;
  else if (w < 1456 && h < 1030)
    size =  QPrinter::B0;

  return size;
}

QwtScaleWidget* Graph::selectedScale()
{
  return scalePicker->selectedAxis();
}

QwtScaleWidget* Graph::currentScale()
{
  return scalePicker->currentAxis();
}

QRect Graph::axisTitleRect(const QwtScaleWidget *scale)
{
  if (!scale)
    return QRect();

  return scalePicker->titleRect(scale);
}

void Graph::setCurrentFont(const QFont& f)
{
  QwtScaleWidget *axis = scalePicker->selectedAxis();
  if (axis){
    if (scalePicker->titleSelected()){
      QwtText title = axis->title();
      title.setFont(f);
      axis->setTitle(title);
    } else if (scalePicker->labelsSelected())
      axis->setFont(f);
    emit modifiedGraph();
  } else if (d_selected_text){
    d_selected_text->setFont(f);
    d_selected_text->repaint();
    emit modifiedGraph();
  } else if (titlePicker->selected()){
    QwtText title = d_plot->title();
    title.setFont(f);
    d_plot->setTitle(title);
    emit modifiedGraph();
  } else {
    QList<QwtPlotItem *> curves = d_plot->curvesList();
    foreach(QwtPlotItem *i, curves){
      DataCurve* dc = dynamic_cast<DataCurve *>(i);
      PlotCurve* pc = dynamic_cast<PlotCurve*>(i);
      if(pc && dc
         && i->rtti() != QwtPlotItem::Rtti_PlotSpectrogram &&
         pc->type() != Graph::Function){
        if(dc->hasSelectedLabels()){
          dc->setLabelsFont(f);
          d_plot->replot();
          emit modifiedGraph();
          return;
        }
      }
    }
  }
}

QString Graph::axisFormula(int axis)
{
  ScaleDraw *sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis));
  if (sd)
    return sd->formula();

  return QString();
}

void Graph::setAxisFormula(int axis, const QString &formula)
{
  ScaleDraw *sd = dynamic_cast<ScaleDraw *>(d_plot->axisScaleDraw(axis));
  if (sd)
    sd->setFormula(formula);
}
/*
     Sets the spectrogram intensity changed boolean flag
 */
void Graph::changeIntensity(bool bIntensityChanged)
{
  /*if(m_spectrogram)
	{
		m_bIntensityChanged=bIntensityChanged;
	}*/
  for (int i=0; i<n_curves; i++){
    QwtPlotItem *it = plotItem(i);
    if (!it)
      continue;
    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
      Spectrogram *sp = dynamic_cast<Spectrogram *>(it);
      if(sp)
      {	sp->setIntensityChange(bIntensityChanged);
      }

    }
  }
}
/* This method zooms the selected graph using using zoom tool and mouse drag
 * @param on :: boolean parameter to switch on zooming
 */
void Graph::enablePanningMagnifier(bool on)
{
  if (d_magnifier)
    delete d_magnifier;
  if (d_panner)
    delete d_panner;

  QwtPlotCanvas *cnvs =d_plot->canvas(); //canvas();
  if (on){
    cnvs->setCursor(Qt::pointingHandCursor);
    d_magnifier = new QwtPlotMagnifier(cnvs);
    d_magnifier->setAxisEnabled(QwtPlot::yRight,false);
    d_magnifier->setZoomInKey(Qt::Key_Plus, Qt::ShiftModifier);

    d_panner = new QwtPlotPanner(cnvs);
    d_panner->setAxisEnabled(QwtPlot::yRight,false);
    connect(d_panner, SIGNAL(panned(int, int)), multiLayer(), SLOT(notifyChanges()));
  } else {
    cnvs->setCursor(Qt::arrowCursor);
    d_magnifier = NULL;
    d_panner = NULL;
  }
}

/* Get the fixed aspect ratio option
 */
bool Graph::isFixedAspectRatioEnabled()
{
#if QWT_VERSION >= 0x050200
  if (d_rescaler) return true;
#endif
  return false;
}
/* Enable or disable fixing the aspect ratio of plots
 * @param on :: If true, aspect ratio will be fixed
 */
void Graph::enableFixedAspectRatio(bool on)
{
#if QWT_VERSION >= 0x050200
  if (d_rescaler)
    delete d_rescaler;

  QwtPlotCanvas *cnvs =d_plot->canvas();
  if (on){
    d_rescaler = new QwtPlotRescaler(cnvs, QwtPlot::xBottom, QwtPlotRescaler::Fixed);
    d_rescaler->setExpandingDirection(QwtPlotRescaler::ExpandBoth);
    // prevent the colormap axis from rescaling
    d_rescaler->setAspectRatio(QwtPlot::yRight,0);
  } else {
    d_rescaler = NULL;
  }
#else
  UNUSED_ARG(on)
    #endif
}

/**
 * Turn off any normalization
 */
void Graph::noNormalization()
{
  if(!m_isDistribution) return; // Nothing to do

  m_isDistribution = false;
  updateDataCurves();
  d_plot->updateAxes();
  setYAxisTitle(yAxisTitleFromFirstCurve());
  notifyChanges();
}

/**
 * Turn on normalization by bin width if it is appropriate
 */
void Graph::binWidthNormalization()
{
  if(m_isDistribution) return; // Nothing to do

  m_isDistribution = true;
  updateDataCurves();
  d_plot->updateAxes();
  setYAxisTitle(yAxisTitleFromFirstCurve());

  notifyChanges();
}

void Graph::setWaterfallXOffset(int offset)
{
  if (offset == d_waterfall_offset_x)
    return;

  if ( offset >= 0 ) d_waterfall_offset_x = offset;
  updateDataCurves();
  replot();
  emit modifiedGraph();
}

void Graph::setWaterfallYOffset(int offset)
{
  if (offset == d_waterfall_offset_y)
    return;

  if ( offset >= 0 ) d_waterfall_offset_y = offset;
  updateDataCurves();
  replot();
  emit modifiedGraph();
}

void Graph::setWaterfallOffset(int x, int y, bool update)
{
  if ( x >= 0 ) d_waterfall_offset_x = x;
  if ( y >= 0 ) d_waterfall_offset_y = y;

  if (update){
    updateDataCurves();
    replot();
    emit modifiedGraph();
  }
}

void Graph::updateWaterfallFill(bool on)
{
  int n = d_plot->curvesList().size(); //d_curves.size();
  if (!n)
    return;

  for (int i = 0; i < n; i++){
    PlotCurve *cv = dynamic_cast<PlotCurve *>(curve(i));
    if (!cv)
      continue;

    if (on && multiLayer())
      cv->setBrush(QBrush(cv->pen().color())); // Default is that each fill color matches line color
    else
      cv->setBrush(QBrush());
  }
  replot();
  emit modifiedGraph();
}

void Graph::setWaterfallSideLines(bool on)
{
  int n = d_plot->curvesList().size(); //d_curves.size();
  if (!n)
    return;

  PlotCurve *cv = dynamic_cast<PlotCurve*>(curve(0));
  if ( cv && cv->sideLinesEnabled() == on)
    return;

  for (int i = 0; i < n; i++){
    cv = dynamic_cast<PlotCurve*>(curve(i));
    if (cv)
      cv->enableSideLines(on);
  }
  replot();
  emit modifiedGraph();
}

void Graph::setWaterfallFillColor(const QColor& c)
{
  int n = d_plot->curvesList().size(); //d_curves.size();
  if (!n)
    return;

  for (int i = 0; i < n; i++){
    PlotCurve *cv = dynamic_cast<PlotCurve*>(curve(i));
    if (cv)
      cv->setBrush(QBrush(c));
  }
  replot();
  emit modifiedGraph();
}

void Graph::reverseCurveOrder()
{
  // RJT: Just call through to Plot - functionality placed there
  d_plot->reverseCurveOrder();

  emit modifiedGraph();
}

void Graph::updateDataCurves()
{
  int n = d_plot->curvesList().size(); //d_curves.size();
  if (!n)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  for (int i = 0; i < n; i++){
    PlotCurve *pc = dynamic_cast<PlotCurve*>(curve(i));
    if (DataCurve *c = dynamic_cast<DataCurve*>(pc))
      c->loadData();
    else if (MantidMatrixCurve *mc = dynamic_cast<MantidMatrixCurve*>(pc))
    {
      mc->setDrawAsDistribution(m_isDistribution);
      mc->invalidateBoundingRect();
      mc->loadData();
    }
  }
  QApplication::restoreOverrideCursor();
}

void Graph::checkValuesInAxisRange(MantidMatrixCurve* mc)
{
  auto* data = mc->mantidData();
  double xMin(data->x(0)); // Needs to be min of current graph (x-axis)
  double xMax(data->x(data->size()-1)); // Needs to be max of current graph (x-axis)
  bool changed(false);
  for (size_t i = 1; i<data->size(); ++i)
  {
    // Sort X
    if(data->x(i) < xMin)
    {
      xMin = data->x(i);
      changed = true;
    }
    else if (data->x(i) > xMax)
    {
      xMax = data->x(i);
      changed = true;
    }
  }

  // If there were values outside the range of the axis then update the axis with the new positions.
  if(changed)
  {
    d_plot->setAxisScale(QwtPlot::Axis(QwtPlot::xTop), xMin, xMax);
    d_plot->setAxisScale(QwtPlot::Axis(QwtPlot::xBottom), xMin, xMax);
  }
}

/**
  * Process dragMousePress signal from d_plot.
  * @param pos :: Mouse position.
  */
void Graph::slotDragMousePress(QPoint pos)
{
  if ( hasActiveTool() ) return;
  emit dragMousePress(pos);
}

/**
  * Process dragMouseRelease signal from d_plot.
  * @param pos :: Mouse position.
  */
void Graph::slotDragMouseRelease(QPoint pos)
{
  if ( hasActiveTool() ) return;
  emit dragMouseRelease(pos);
}

/**
  * Process dragMouseMove signal from d_plot.
  * @param pos :: Mouse position.
  */
void Graph::slotDragMouseMove(QPoint pos)
{
  if ( hasActiveTool() ) return;
  emit dragMouseMove(pos);
}

void Graph::loadFromProject(const std::string& lines, ApplicationWindow* app, const int fileVersion)
{
  blockSignals(true);

  enableAutoscaling(app->autoscale2DPlots);

  TSVSerialiser tsv(lines);

  if(tsv.selectSection("Antialiasing"))
  {
    int aa;
    tsv >> aa;
    setAntialiasing(aa);
  }

  if(tsv.selectSection("Autoscaling"))
  {
    int as;
    tsv >> as;
    enableAutoscaling(as);
  }

  if(tsv.selectLine("AxesColors"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("AxesColors").c_str()).split("\t");
    sl.pop_front();
    for(int i = 0; i < sl.count(); ++i)
      setAxisColor(i, QColor(sl[i]));
  }

  if(tsv.selectLine("AxesNumberColors"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("AxesNumberColors").c_str()).split("\t");
    sl.pop_front();
    for(int i = 0; i < sl.count(); ++i)
      setAxisLabelsColor(i, QColor(sl[i]));
  }

  if(tsv.selectLine("AxesTitleColors"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("AxesTitleColors").c_str()).split("\t");
    sl.pop_front();
    for(int i = 0; i < sl.count(); ++i)
      setAxisTitleColor(i, QColor(sl[i]));
  }

  if(tsv.selectLine("AxesTitleAlignment"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("AxesTitleAlignment").c_str()).split("\t");
    sl.pop_front();
    for(int i = 0; i < sl.count(); ++i)
      setAxisTitleAlignment(i, sl[i].toInt());
  }

  if(tsv.selectLine("AxesBaseline"))
  {
    size_t n = tsv.values("AxesBaseline").size();
    for(size_t i = 0; i < n - 1; ++i)
    {
      setAxisMargin((int)i, tsv.asInt(i+1));
    }
  }

  if(tsv.selectLine("AxesTitles"))
  {
    std::vector<std::string> values = tsv.values("AxesTitles");
    for(size_t i = 1; i < values.size(); ++i)
      setScaleTitle((int)(i - 1), QString::fromUtf8(values[i].c_str()));
  }

  if(tsv.selectLine("AxisType"))
  {
    std::vector<std::string> values = tsv.values("AxisType");

    if(values.size() >= 4)
    {
      for(int i = 0; i < 4; ++i)
      {
        QStringList sl = QString::fromUtf8(values[i].c_str()).split(";");
        int format = sl[0].toInt();
        if(format == ScaleDraw::Numeric)
          continue;
        if(format == ScaleDraw::Day)
          setLabelsDayFormat(i, sl[1].toInt());
        else if(format == ScaleDraw::Month)
          setLabelsMonthFormat(i, sl[1].toInt());
        else if(format == ScaleDraw::Time || format == ScaleDraw::Date)
          setLabelsDateTimeFormat(i, format, sl[1]+";"+sl[2]);
        else if(sl.size() > 1)
          setLabelsTextFormat(i, format, sl[1], app->table(sl[1]));
      }
    }
  }

  for(int i = 0; i < 4; ++i)
  {
    std::stringstream ss;
    ss << "AxisFont" << i;
    if(tsv.selectLine(ss.str()))
    {
      QString font;
      int pointSize, weight, italic, underline, strikeout;
      tsv >> font >> pointSize >> weight >> italic >> underline >> strikeout;

      QFont fnt(font, pointSize, weight, italic);
      fnt.setUnderline(underline);
      fnt.setStrikeOut(strikeout);
      setAxisFont(i, fnt);
    }
  }

  for(int i = 0; i < 4; ++i)
  {
    std::stringstream ss;
    ss << "AxisFormula " << i;
    auto afSections = tsv.sections(ss.str());
    if(afSections.size() > 0)
      setAxisFormula(i, QString::fromUtf8(afSections.front().c_str()));
  }

  if(tsv.selectLine("AxesLineWidth"))
  {
    int lineWidth;
    tsv >> lineWidth;
    loadAxesLinewidth(lineWidth);
  }

  if(tsv.selectLine("Background"))
  {
    QString color;
    int alpha;
    tsv >> color >> alpha;

    QColor c(color);
    if(alpha > 0)
      c.setAlpha(alpha);

    setBackgroundColor(c);
  }

  if(tsv.selectLine("Border"))
  {
    int border;
    QString color;
    tsv >> border >> color;
    setFrame(border, QColor(color));
  }

  if(tsv.selectLine("CanvasFrame"))
  {
    int lineWidth;
    QString color;
    tsv >> lineWidth >> color;
    setCanvasFrame(lineWidth, QColor(color));
  }

  if(tsv.selectLine("CanvasBackground"))
  {
    QString color;
    int alpha;
    tsv >> color >> alpha;
    QColor c = QColor(color);
    if(alpha > 0)
      c.setAlpha(alpha);
    setCanvasBackground(c);
  }

  if(tsv.selectLine("DrawAxesBackbone"))
  {
    QString axesOptions;
    tsv >> axesOptions;
    loadAxesOptions(axesOptions);
  }

  if(tsv.selectLine("EnabledAxes"))
  {
    size_t n = tsv.values("EnabledAxes").size();
    for(size_t i = 0; i < n - 1; ++i)
      enableAxis((int)i, tsv.asInt(i+1));
  }

  if(tsv.selectLine("EnabledTicks"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("EnabledTicks").c_str()).split("\t");
    sl.pop_front();
    sl.replaceInStrings("-1", "3");
    setMajorTicksType(sl);
    setMinorTicksType(sl);
  }

  if(tsv.selectLine("EnabledTickLabels"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("EnabledTickLabels").c_str()).split("\t");
    sl.pop_front();
    for(int i = 0; i < sl.count(); ++i)
      enableAxisLabels(i, sl[i].toInt());
  }

  if(tsv.selectLine("grid"))
  {
    plotWidget()->grid()->load(QString::fromUtf8(tsv.lineAsString("grid").c_str()).split("\t"));
  }

  for(int i = 0; tsv.selectLine("ImageMarker", i); ++i)
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("ImageMarker", i).c_str()).split("\t");
    insertImageMarker(sl, fileVersion);
  }

  std::vector<std::string> imageSections = tsv.sections("image");
  for(auto it = imageSections.begin(); it != imageSections.end(); ++it)
  {
    QStringList sl = QString::fromUtf8((*it).c_str()).split("\t");
    insertImageMarker(sl, fileVersion);
  }

  if(tsv.selectLine("LabelsFormat"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("LabelsFormat").c_str()).split("\t");
    sl.pop_front();
    setLabelsNumericFormat(sl);
  }

  if(tsv.selectLine("LabelsRotation"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("LabelsRotation").c_str()).split("\t");
    setAxisLabelRotation(QwtPlot::xBottom, sl[1].toInt());
    setAxisLabelRotation(QwtPlot::xTop, sl[2].toInt());
  }

  std::vector<std::string> legendSections = tsv.sections("legend");
  for(auto it = legendSections.begin(); it != legendSections.end(); ++it)
    insertText("legend", *it);

  std::vector<std::string> lineSections = tsv.sections("line");
  for(auto it = lineSections.begin(); it != lineSections.end(); ++it)
  {
    QStringList sl = QString::fromUtf8((*it).c_str()).split("\t");
    addArrow(sl, fileVersion);
  }

  if(tsv.selectLine("Margin"))
  {
    int margin;
    tsv >> margin;
    plotWidget()->setMargin(margin);
  }

  if(tsv.selectLine("MajorTicks"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("MajorTicks").c_str()).split("\t");
    sl.pop_front();
    setMajorTicksType(sl);
  }

  if(tsv.selectLine("MinorTicks"))
  {
    QStringList sl = QString::fromUtf8(tsv.lineAsString("MinorTicks").c_str()).split("\t");
    sl.pop_front();
    setMinorTicksType(sl);
  }

  for(int i = 0; tsv.selectLine("PieCurve", i); ++i)
  {
    QString pieName;
    tsv >> pieName;

    if(!app->renamedTables.isEmpty())
    {
      QString caption = pieName.left(pieName.find("_", 0));
      if(app->renamedTables.contains(caption))
      {
        int index = app->renamedTables.findIndex(caption);
        QString newCaption = app->renamedTables[++index];
        pieName.replace(caption+"_", newCaption+"_");
      }
    }

    QString penColor, penStyle;
    double penThickness;
    tsv >> penThickness >> penColor >> penStyle;
    QPen pen(QColor(penColor), penThickness, Graph::getPenStyle(penStyle));

    Table* table = app->table(pieName);
    if(!table)
      continue;

    int startRow = 0;
    int endRow = table->numRows() - 1;
    int visible = 1;

    int firstColor, brush, brushSize, antiClockwise, autoLabelling;
    int values, percentages, categories, fixedLabels;
    double startAzi, viewAngle, thickness, horOffset, edgeDist;

    tsv >> brush >> brushSize >> firstColor;
    tsv >> startRow >> endRow >> visible;
    tsv >> startAzi >> viewAngle >> thickness >> horOffset;
    tsv >> edgeDist >> antiClockwise >> autoLabelling >> values;
    tsv >> percentages >> categories >> fixedLabels;

    plotPie(table, pieName, pen, brush,
        brushSize, firstColor, startRow, endRow, visible,
        startAzi, viewAngle, thickness, horOffset, edgeDist,
        antiClockwise, autoLabelling, values, percentages,
        categories, fixedLabels);
  }

  std::vector<std::string> pieLabelSections = tsv.sections("PieLabel");
  for(auto it = pieLabelSections.begin(); it != pieLabelSections.end(); ++it)
    insertText("PieLabel", *it);

  if(tsv.selectLine("PlotTitle"))
  {
    QString title, color;
    int alignment;
    tsv >> title >> color >> alignment;
    setTitle(title);
    setTitleColor(QColor(color));
    setTitleAlignment((Qt::AlignmentFlag)alignment);
  }

  for(int i = 0; tsv.selectLine("scale", i); ++i)
  {
    QStringList scl = QString::fromUtf8(tsv.lineAsString("scale", i).c_str()).split("\t");
    scl.pop_front();
    if(scl.count() >= 8)
    {
      setScale(scl[0].toInt(), scl[1].toDouble(), scl[2].toDouble(), scl[3].toDouble(),
        scl[4].toInt(), scl[5].toInt(),  scl[6].toInt(), bool(scl[7].toInt()));
    }
  }

  for(int i = 0; i < 4; ++i)
  {
    std::stringstream ss;
    ss << "ScaleFont" << i;
    if(tsv.selectLine(ss.str()))
    {
      QString font;
      int pointSize, weight, italic, underline, strikeout;
      tsv >> font >> pointSize >> weight >> italic >> underline >> strikeout;

      QFont fnt(font, pointSize, weight, italic);
      fnt.setUnderline(underline);
      fnt.setStrikeOut(strikeout);
      setAxisTitleFont(i, fnt);
    }
  }

  if(tsv.selectSection("SyncScales"))
  {
    int ss;
    tsv >> ss;
    setSynchronizedScaleDivisions(ss);
  }

  std::vector<std::string> textSections = tsv.sections("text");
  for(auto it = textSections.begin(); it != textSections.end(); ++it)
    insertText("text", *it);

  if(tsv.selectLine("TitleFont"))
  {
    QString font;
    int pointSize, weight, italic, underline, strikeout;
    tsv >> font >> pointSize >> weight >> italic >> underline >> strikeout;
    QFont fnt(font, pointSize, weight, italic);
    fnt.setUnderline(underline);
    fnt.setStrikeOut(strikeout);
    setTitleFont(fnt);
  }

  if(tsv.selectLine("TicksLength"))
  {
    setTicksLength(tsv.asInt(1), tsv.asInt(2));
  }

  //curveID section
  {
    //All of the lines and sections are assigned curveIDs based on the order in which they are loaded.
    int curveID = 0;

    for(int i = 0; tsv.selectLine("MantidMatrixCurve", i); ++i)
    {
      std::vector<std::string> values = tsv.values("MantidMatrixCurve", i);

      if(values.size() < 5)
        continue;

      QString wsName = tsv.asString(1).c_str();
      int index = tsv.asInt(3);
      int skipSymbolsCount = tsv.asInt(5);

      if(values.size() < 7) //Pre 29 Feb 2012
      {
        PlotCurve* c = new MantidMatrixCurve(wsName, this, index,
            MantidMatrixCurve::Spectrum, tsv.asInt(4));

        if(values.size() == 6 && values[5].length() > 0)
          c->setSkipSymbolsCount(skipSymbolsCount);
      }
      else //Post 29 Feb 2012
      {
        PlotCurve* c = new MantidMatrixCurve(wsName, this, index,
            MantidMatrixCurve::Spectrum, tsv.asInt(4), tsv.asInt(5));
        setCurveType(curveID, tsv.asInt(6));

        QStringList sl = QString::fromUtf8(tsv.lineAsString("MantidMatrixCurve", i).c_str()).split("\t");
        CurveLayout cl = fillCurveSettings(sl, fileVersion, 3);
        updateCurveLayout(c,&cl);
      }
      curveID++;
    }

    for(int i = 0; tsv.selectLine("curve", i); ++i)
    {
      QStringList curveValues = QString::fromUtf8(tsv.lineAsString("curve",i).c_str()).split("\t");
      CurveLayout cl = fillCurveSettings(curveValues, fileVersion, 0);

      QString tableName;
      int plotType;

      tsv >> tableName >> plotType;

      Table* table = app->table(tableName);
      if(table)
      {
        PlotCurve* c = NULL;
        if(plotType == Graph::VectXYXY || plotType == Graph::VectXYAM)
        {
          QStringList colsList;
          colsList << curveValues[1] << curveValues[2];
          colsList << curveValues[20] << curveValues[21];

          int startRow = curveValues[curveValues.count()-3].toInt();
          int endRow = curveValues[curveValues.count()-2].toInt();

          c = reinterpret_cast<PlotCurve*>(plotVectorCurve(
              table, colsList, plotType, startRow, endRow));

          if(plotType == Graph::VectXYXY)
          {
            updateVectorsLayout(curveID, curveValues[15],
                curveValues[16].toDouble(), curveValues[17].toInt(),
                curveValues[18].toInt(), curveValues[19].toInt(), 0);
          }
          else
          {
            updateVectorsLayout(curveID, curveValues[15],
                curveValues[16].toDouble(), curveValues[17].toInt(),
                curveValues[18].toInt(), curveValues[19].toInt(),
                curveValues[22].toInt());
          }
        }
        else if(plotType == Graph::Box)
        {
          c = reinterpret_cast<PlotCurve*>(openBoxDiagram(table, curveValues, fileVersion));
        }
        else
        {
          int startRow = curveValues[curveValues.count()-3].toInt();
          int endRow = curveValues[curveValues.count()-2].toInt();
          c = dynamic_cast<PlotCurve*>(insertCurve(table, curveValues[1], curveValues[2], plotType, startRow, endRow));
        }

        if(plotType == Graph::Histogram)
        {
          QwtHistogram* h = dynamic_cast<QwtHistogram*>(curve(curveID));
          h->setBinning(curveValues[17].toInt(),curveValues[18].toDouble(),curveValues[19].toDouble(),curveValues[20].toDouble());
          h->loadData();
        }

        if(plotType == Graph::VerticalBars
            || plotType == Graph::HorizontalBars
            || plotType == Graph::Histogram)
        {
          setBarsGap(curveID, curveValues[15].toInt(), curveValues[16].toInt());
        }

        updateCurveLayout(c, &cl);

        if(c && c->rtti() == QwtPlotItem::Rtti_PlotCurve)
        {
          c->setAxis(curveValues[curveValues.count()-5].toInt(), curveValues[curveValues.count()-4].toInt());
          c->setVisible(curveValues.last().toInt());
        }
      }
      else if(plotType == Graph::Histogram)
      {
        Matrix* m = app->matrix(tableName);
        QwtHistogram* h = restoreHistogram(m, curveValues);
        updateCurveLayout(h, &cl);
      }
      curveID++;
    }

    std::vector<std::string> functionSections = tsv.sections("Function");
    for(auto it = functionSections.begin(); it != functionSections.end(); ++it)
    {
      curveID++;
      QStringList sl = QString::fromUtf8((*it).c_str()).split("\n");
      restoreFunction(sl);
    }

    for(int i = 0; tsv.selectLine("FunctionCurve", i); ++i)
    {
      CurveLayout cl;
      QString formula, discarded;
      int points, curveStyle, axis1, axis2, visible;

      //CurveLayout members
      int connectType, lCol, lStyle, sSize, sType, symCol, fillCol, filledArea, aCol, aStyle;
      float lWidth;

      tsv >> formula >> points >> discarded >> discarded;
      tsv >> curveStyle >> connectType >> lCol >> lStyle;
      tsv >> lWidth >> sSize >> sType >> symCol;
      tsv >> fillCol >> filledArea >> aCol >> aStyle;
      tsv >> axis1 >> axis2 >> visible;

      cl.connectType = connectType;
      cl.lCol = lCol;
      cl.lStyle = lStyle;
      cl.lWidth = lWidth;
      cl.sSize = sSize;
      cl.sType = sType;
      cl.symCol = symCol;
      cl.fillCol = fillCol;
      cl.filledArea = filledArea;
      cl.aCol = aCol;
      cl.aStyle = aStyle;

      if(curveStyle == Graph::Box)
      {
        float penWidth;
        tsv >> penWidth;
        cl.penWidth = penWidth;
      }
      else if(curveStyle <= Graph::LineSymbols)
      {
        float penWidth;
        tsv >> penWidth;
        cl.penWidth = penWidth;
      }
      else
      {
        cl.penWidth = cl.lWidth;
      }

      PlotCurve* c = dynamic_cast<PlotCurve*>(insertFunctionCurve(formula, points, fileVersion));

      setCurveType(curveID, curveStyle);
      updateCurveLayout(c, &cl);
      QwtPlotCurve* qc = curve(curveID);
      if(qc)
      {
        qc->setAxis(axis1, axis2);
        qc->setVisible(visible);
      }

      curveID++;
    }

    if(tsv.selectLine("ErrorBars"))
    {
      QStringList sl = QString::fromUtf8(tsv.lineAsString("ErrorBars").c_str()).split("\t");
      if(!app->renamedTables.isEmpty())
      {
        QString caption = sl[4].left(sl[4].find("_",0));
        if(app->renamedTables.contains(caption))
        {
          //modify the name of the curve according to the new table name
          int index = app->renamedTables.findIndex(caption);
          QString newCaption = app->renamedTables[++index];
          sl.replaceInStrings(caption+"_", newCaption+"_");
        }
      }
      Table* w = app->table(sl[3]);
      Table* errTable = app->table(sl[4]);
      if(w && errTable)
      {
        addErrorBars(sl[2], sl[3], errTable, sl[4], sl[1].toInt(),
            sl[5].toDouble(), sl[6].toInt(), QColor(sl[7]),
            sl[8].toInt(), sl[10].toInt(), sl[9].toInt());
      }
      curveID++;
    }

    std::vector<std::string> specSections = tsv.sections("spectrogram");
    for(auto it = specSections.begin(); it != specSections.end(); ++it)
    {
      TSVSerialiser specTSV(*it);

      if(specTSV.selectLine("workspace"))
      {
        std::string wsName;
        specTSV >> wsName;

        Mantid::API::IMDWorkspace_const_sptr wsPtr = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IMDWorkspace>(wsName);

        //Check the pointer
        if(!wsPtr.get())
          continue;

        /* You may notice we plot the spectrogram before loading it.
        * Why? Because plotSpectrogram overrides the spectrograms' settings
        * based off the second parameter (which has been chosen arbitrarily
        * in this case). We're just use plotSpectrogram to add the spectrogram
        * to the graph for us, and then loading the settings into the spectrogram.
        */
        Spectrogram* s = new Spectrogram(QString::fromUtf8(wsName.c_str()), wsPtr);
        plotSpectrogram(s, Graph::ColorMap);
        s->loadFromProject(*it);
        curveID++;
      }
      else if(specTSV.selectLine("matrix"))
      {
        std::string matrixName;
        specTSV >> matrixName;

        Matrix* m = app->matrix(QString::fromStdString(matrixName));

        if(!m)
          continue;

        Spectrogram* s = new Spectrogram(m);
        plotSpectrogram(s, Graph::ColorMap);
        s->loadFromProject(*it);
        curveID++;
      }
    }

    //<SkipPoints>, <CurveLabels>, and <MantidYErrors> all apply to the
    //MantidMatrixCurve that was before it in the file. This is an annoying
    //edge case, but not too difficult to solve.

    //Because we load all the MantidMatrixCurves from the file first (and in order),
    //we can simply iterate through the file, counting them and using the count as
    //the curveID for the section we're loading.

    std::vector<std::string> lineVec;
    boost::split(lineVec, lines, boost::is_any_of("\n"));

    int lastCurveID = -1;
    for(auto lineIt = lineVec.begin(); lineIt != lineVec.end(); ++lineIt)
    {
      const std::string line = *lineIt;

      if(line.find("MantidMatrixCurve") == 0)
      {
        //Moving onto the next MantidMatrixCurve.
        lastCurveID++;
        continue;
      }

      //Handle sections as appropriate.
      if(line.find("<SkipPoints>") == 0)
      {
        PlotCurve* c = dynamic_cast<PlotCurve*>(curve(lastCurveID));
        if(!c)
          continue;

        //Remove surrounding tags.
        const std::string contents = line.substr(12, line.length() - 25);

        int value = 0;
        Mantid::Kernel::Strings::convert<int>(contents, value);
        c->setSkipSymbolsCount(value);
      }
      else if(line.find("<CurveLabels>") == 0)
      {
        //Start reading from next line
        lineIt++;
        if(lineIt == lineVec.end())
          break;

        QStringList lst;
        while(*lineIt != "</CurveLabels")
        {
          lst << QString::fromUtf8((*(lineIt++)).c_str());

          if(lineIt == lineVec.end())
            break;
        }

        //We now have StringList of the lines we want.
        restoreCurveLabels(lastCurveID, lst);
      }
      else if(line.find("<MantidYErrors>") == 0)
      {
        MantidCurve *c = dynamic_cast<MantidCurve*>(curve(lastCurveID));
        if(!c)
          continue;

        //Remove surrounding tags.
        const std::string contents = line.substr(15, line.length() - 31);

        c->errorBarSettingsList().front()->fromString(QString::fromUtf8(contents.c_str()));
      }
    }
  }//end of curveID section

  if(tsv.hasSection("waterfall"))
  {
    std::string contents = tsv.sections("waterfall").front();
    QStringList sl = QString::fromUtf8(contents.c_str()).split(",");
    if(sl.size() >= 2)
      setWaterfallOffset(sl[0].toInt(), sl[1].toInt());
    if(sl.size() >= 3)
      setWaterfallSideLines(sl[2].toInt());
    updateDataCurves();
  }

  replot();
  blockSignals(false);

  setIgnoreResizeEvents(!app->autoResizeLayers);
  setAutoscaleFonts(app->autoScaleFonts);
}

std::string Graph::saveToProject()
{
  TSVSerialiser tsv;

  tsv.writeLine("ggeometry") << pos().x() << pos().y() << frameGeometry().width() << frameGeometry().height();

  tsv.writeLine("PlotTitle");
  tsv << d_plot->title().text().replace("\n", "<br>");
  tsv << d_plot->title().color().name();
  tsv << d_plot->title().renderFlags();

  tsv.writeInlineSection("Antialiasing", d_antialiasing       ? "1" : "0");
  tsv.writeInlineSection("SyncScales",   d_synchronize_scales ? "1" : "0");

  tsv.writeLine("Background");
  tsv << d_plot->paletteBackgroundColor().name();
  tsv << d_plot->paletteBackgroundColor().alpha();

  tsv.writeLine("Margin") << d_plot->margin();
  tsv.writeLine("Border") << d_plot->lineWidth() << d_plot->frameColor().name();

  tsv.writeRaw(grid()->saveToString());

  tsv.writeLine("EnabledAxes");
  for(int i = 0; i < 4; ++i)
    tsv << d_plot->axisEnabled(i);

  tsv.writeLine("AxesTitles");
  tsv << d_plot->axisTitle(2).text().replace("\n", "<br>");
  tsv << d_plot->axisTitle(0).text().replace("\n", "<br>");
  tsv << d_plot->axisTitle(3).text().replace("\n", "<br>");
  tsv << d_plot->axisTitle(1).text().replace("\n", "<br>");

  tsv.writeLine("AxesTitleColors");
  for(int i = 0; i < 4; ++i)
  {
    QwtScaleWidget* scale = dynamic_cast<QwtScaleWidget*>(d_plot->axisWidget(i));
    QColor color = scale ? scale->title().color() : QColor(Qt::black);
    tsv << color.name();
  }

  tsv.writeLine("AxesTitleAlignment");
  for(int i = 0; i < 4; ++i)
    if(d_plot->axisEnabled(i))
      tsv << d_plot->axisTitle(i).renderFlags();
    else
      tsv << Qt::AlignHCenter;

  tsv.writeLine("TitleFont");
  {
    QFont f = d_plot->title().font();
    tsv << f.family().toStdString() << f.pointSize() << f.weight() << f.italic() << f.underline() << f.strikeOut();
  }

  for(int i = 0; i < 4; ++i)
  {
    std::stringstream ss;
    ss << "ScaleFont" << i;
    tsv.writeLine(ss.str());
    QFont f = d_plot->axisTitle(i).font();
    tsv << f.family().toStdString() << f.pointSize() << f.weight() << f.italic() << f.underline() << f.strikeOut();
  }

  for(int i = 0; i < 4; ++i)
  {
    std::stringstream ss;
    ss << "AxisFont" << i;
    tsv.writeLine(ss.str());
    QFont f = d_plot->axisFont(i);
    tsv << f.family().toStdString() << f.pointSize() << f.weight() << f.italic() << f.underline() << f.strikeOut();
  }

  tsv.writeLine("EnabledTickLabels");
  for(int i = 0; i < 4; ++i)
    tsv << d_plot->axisScaleDraw(i)->hasComponent(QwtAbstractScaleDraw::Labels);

  tsv.writeLine("AxesColors");
  for(int i = 0; i < 4; ++i)
  {
    QwtScaleWidget* scale = dynamic_cast<QwtScaleWidget*>(d_plot->axisWidget(i));
    QColor col = scale ? scale->palette().color(QPalette::Active, QColorGroup::Foreground) : QColor(Qt::black);
    tsv << col.name();
  }

  tsv.writeLine("AxesNumberColors");
  for(int i = 0; i < 4; ++i)
  {
    QwtScaleWidget* scale = dynamic_cast<QwtScaleWidget*>(d_plot->axisWidget(i));
    QColor col = scale ? scale->palette().color(QPalette::Active, QColorGroup::Text) : QColor(Qt::black);
    tsv << col.name();
  }

  tsv.writeLine("AxesBaseline");
  for(int i = 0; i < 4; ++i)
  {
    QwtScaleWidget* scale = dynamic_cast<QwtScaleWidget*>(d_plot->axisWidget(i));
    if(scale)
      tsv << scale->margin();
    else
      tsv << 0;
  }

  if(d_plot->canvas()->lineWidth() > 0)
    tsv.writeLine("CanvasFrame") << d_plot->canvas()->lineWidth() << canvasFrameColor().name();

  tsv.writeLine("CanvasBackground");
  tsv << d_plot->canvasBackground().name();
  tsv << d_plot->canvasBackground().alpha();

  if(isPiePlot())
  {
    tsv.writeRaw(savePieCurveLayout().toStdString());
  }
  else
  {
    for(int i = 0; i < n_curves; ++i)
      tsv.writeRaw(saveCurve(i));
  }

  tsv.writeRaw(saveScale());

  //Axis formulae
  for(int i = 0; i < 4; ++i)
  {
    auto sd = dynamic_cast<ScaleDraw*>(d_plot->axisScaleDraw(i));
    if(!sd)
      continue;

    if(sd->formula().isEmpty())
      continue;

    std::stringstream ss;
    ss << "<AxisFormula pos=\"" << i << "\">" << "\n";
    ss << sd->formula().toStdString()         << "\n";
    ss << "</AxisFormula>"                    << "\n";
    tsv.writeRaw(ss.str());
  }

  tsv.writeLine("LabelsFormat");
  for(int i = 0; i < 4; ++i)
    tsv << d_plot->axisLabelFormat(i) << d_plot->axisLabelPrecision(i);

  tsv.writeLine("AxisType");
  for(int i = 0; i < 4; ++i)
  {
    if(!d_plot->axisEnabled(i))
    {
      tsv << ScaleDraw::Numeric;
      continue;
    }

    auto sd = dynamic_cast<ScaleDraw*>(d_plot->axisScaleDraw(i));
    if(!sd)
      continue;

    const int type = sd->scaleType();

    std::stringstream ss;
    ss << type;

    if(type == ScaleDraw::Time  || type == ScaleDraw::Date ||
       type == ScaleDraw::Text  || type == ScaleDraw::Day  ||
       type == ScaleDraw::Month || type == ScaleDraw::ColHeader)
      ss << ";" << sd->formatString().toUtf8().constData();

    tsv << ss.str();
  }

  tsv.writeLine("MajorTicks");
  QList<int> majorTicksTypeList = d_plot->getMajorTicksType();
  for(int i = 0; i < 4; ++i)
    tsv << majorTicksTypeList[i];

  tsv.writeLine("MinorTicks");
  QList<int> minorTicksTypeList = d_plot->getMinorTicksType();
  for(int i = 0; i < 4; ++i)
    tsv << minorTicksTypeList[i];

  tsv.writeLine("TicksLength") << minorTickLength() << majorTickLength();
  tsv.writeLine("DrawAxesBackbone") << drawAxesBackbone;
  tsv.writeLine("AxesLineWidth") << d_plot->axesLinewidth();

  tsv.writeLine("LabelsRotation");
  tsv << labelsRotation(QwtPlot::xBottom) << labelsRotation(QwtPlot::xTop);

  tsv.writeRaw(saveMarkers());

  if(isWaterfallPlot())
  {
    QString s = "<waterfall>" + QString::number(d_waterfall_offset_x) + ",";
    s += QString::number(d_waterfall_offset_y) + ",";
    bool sideLines = false;
    if(d_plot->curvesList().size() > 0)
    {
      auto cv = dynamic_cast<PlotCurve*>(curve(0));
      if(cv && cv->sideLinesEnabled())
        sideLines = true;
    }
    s += QString::number(sideLines) + "</waterfall>\n";

    tsv.writeRaw(s.toStdString());
  }

  return tsv.outputLines();
}

/** A method to populate the CurveLayout struct on loading a project
 *  @param curve  The list of numbers corresponding to settings loaded from the project file
 *  @param fileVersion  The version number of the project file being loaded
 *  @param offset An offset to add to each index. Used when loading a MantidMatrixCurve
 *  @return The filled in CurveLayout struct
 */
CurveLayout Graph::fillCurveSettings(const QStringList & curve, int fileVersion, unsigned int offset)
{
  CurveLayout cl;
  cl.connectType=curve[4+offset].toInt();
  cl.lCol=curve[5+offset].toInt();
  cl.lStyle=curve[6+offset].toInt();
  cl.lWidth=curve[7+offset].toFloat();
  cl.sSize=curve[8+offset].toInt();
  cl.sType=curve[9+offset].toInt();
  cl.symCol=curve[10+offset].toInt();
  cl.fillCol=curve[11+offset].toInt();
  cl.filledArea=curve[12+offset].toInt();
  cl.aCol=curve[13+offset].toInt();
  cl.aStyle=curve[14+offset].toInt();
  if(curve.count() < 16)
    cl.penWidth = cl.lWidth;
  else if ((fileVersion >= 79) && (curve[3+offset].toInt() == Graph::Box))
    cl.penWidth = curve[15+offset].toFloat();
  else if ((fileVersion >= 78) && (curve[3+offset].toInt() <= Graph::LineSymbols))
    cl.penWidth = curve[15+offset].toFloat();
  else
    cl.penWidth = cl.lWidth;

  return cl;
}

std::string Graph::saveCurve(int i)
{
  QwtPlotItem* it = plotItem(i);
  if(!it)
    return "";

  if(it->rtti() == QwtPlotItem::Rtti_PlotUserItem)
  {
    auto mmc = dynamic_cast<MantidMatrixCurve*>(it);
    if(!mmc)
      return "";

    QString s = mmc->saveToString();
    s += saveCurveLayout(i);
    s += "\n";

    if(mmc->hasErrorBars())
      s += "<MantidYErrors>" + mmc->errorBarSettingsList().front()->toString() + "</MantidYErrors>\n";

    if(mmc->skipSymbolsCount() > 1)
      s += "<SkipPoints>" + QString::number(mmc->skipSymbolsCount()) + "</SkipPoints>\n";

    return s.toUtf8().constData();
  }

  if(it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
  {
    auto spec = dynamic_cast<Spectrogram*>(it);
    if(spec)
      return spec->saveToProject();
    return "";
  }

  auto funcCurve = dynamic_cast<FunctionCurve*>(it);
  if(funcCurve)
    return funcCurve->saveToString().toUtf8().constData();

  auto er = dynamic_cast<QwtErrorPlotCurve *>(it);
  if(er)
  {
    QString s = "ErrorBars\t";
    s += QString::number(er->direction())+"\t";
    s += er->masterCurve()->xColumnName() + "\t";
    s += er->masterCurve()->title().text() + "\t";
    s += er->title().text() + "\t";
    s += er->toString() + "\n";
    return s.toUtf8().constData();
  }

  //If we're none of the above...
  auto c = dynamic_cast<DataCurve*>(it);
  if(c)
  {
    QString s;
    if(c->type() == Box)
      s += "curve\t" + QString::number(c->x(0)) + "\t" + c->title().text() + "\t";
    else
      s += "curve\t" + c->xColumnName() + "\t" + c->title().text() + "\t";

    s += saveCurveLayout(i);
    s += QString::number(c->xAxis())+"\t"+QString::number(c->yAxis())+"\t";
    s += QString::number(c->startRow())+"\t"+QString::number(c->endRow())+"\t";
    s += QString::number(c->isVisible())+"\n";
    s += c->saveToString();
    return s.toUtf8().constData();
  }

  return "";
}

std::string Graph::saveScale()
{
  TSVSerialiser tsv;
  for(int i = 0; i < 4; i++)
  {
    tsv.writeLine("scale") << i;

    const QwtScaleDiv* scDiv = d_plot->axisScaleDiv(i);
    if(!scDiv)
      return "";

    tsv << QString::number(QMIN(scDiv->lBound(), scDiv->hBound()), 'g', 15);
    tsv << QString::number(QMAX(scDiv->lBound(), scDiv->hBound()), 'g', 15);

    tsv << QString::number(d_user_step[i], 'g', 15);
    tsv << d_plot->axisMaxMajor(i);
    tsv << d_plot->axisMaxMinor(i);

    auto se = dynamic_cast<ScaleEngine*>(d_plot->axisScaleEngine(i));
    if(!se)
      return "";

    tsv << (int)se->type();
    tsv << se->testAttribute(QwtScaleEngine::Inverted);
    if (se->hasBreak())
    {
      tsv << QString::number(se->axisBreakLeft(), 'g', 15);
      tsv << QString::number(se->axisBreakRight(), 'g', 15);
      tsv << se->breakPosition();
      tsv << QString::number(se->stepBeforeBreak(), 'g', 15);
      tsv << QString::number(se->stepAfterBreak(), 'g', 15);
      tsv << se->minTicksBeforeBreak();
      tsv << se->minTicksAfterBreak();
      tsv << se->log10ScaleAfterBreak();
      tsv << se->breakWidth();
      tsv << se->hasBreakDecoration();
    }

    for(int j = 0; j < n_curves; j++)
    {
      QwtPlotItem* it = plotItem(j);
      if(it && it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
        tsv << updatedaxis[i];
    }
  }
  return tsv.outputLines();
}

std::string Graph::saveMarkers()
{
  TSVSerialiser tsv;
  for(int i = 0; i < d_images.size(); ++i)
  {
    auto mrkI = dynamic_cast<ImageMarker*>(d_plot->marker(d_images[i]));
    if(!mrkI)
      continue;

    QString s = "<image>";
    s += "\t" + mrkI->fileName();
    s += "\t" + QString::number(mrkI->xValue(), 'g', 15);
    s += "\t" + QString::number(mrkI->yValue(), 'g', 15);
    s += "\t" + QString::number(mrkI->right(),  'g', 15);
    s += "\t" + QString::number(mrkI->bottom(), 'g', 15);
    s += "</image>\n";
    tsv.writeRaw(s.toStdString());
  }

  for(int i = 0; i < d_lines.size(); ++i)
  {
    auto mrkL = dynamic_cast<ArrowMarker*>(d_plot->marker(d_lines[i]));
    if(!mrkL)
      continue;

    QwtDoublePoint sp = mrkL->startPointCoord();
    QwtDoublePoint ep = mrkL->endPointCoord();

    QString s ="<line>";
    s += "\t" + (QString::number(sp.x(), 'g', 15));
    s += "\t" + (QString::number(sp.y(), 'g', 15));
    s += "\t" + (QString::number(ep.x(), 'g', 15));
    s += "\t" + (QString::number(ep.y(), 'g', 15));
    s += "\t" + QString::number(mrkL->width());
    s += "\t" + mrkL->color().name();
    s += "\t" + penStyleName(mrkL->style());
    s += "\t" + QString::number(mrkL->hasEndArrow());
    s += "\t" + QString::number(mrkL->hasStartArrow());
    s += "\t" + QString::number(mrkL->headLength());
    s += "\t" + QString::number(mrkL->headAngle());
    s += "\t" + QString::number(mrkL->filledArrowHead());
    s += "</line>\n";
    tsv.writeRaw(s.toStdString());
  }

  QObjectList lst = d_plot->children();
  foreach(QObject* o, lst)
  {
    auto l = dynamic_cast<LegendWidget*>(o);
    if(!l)
      continue;

    QString s;

    if(l == d_legend)
      s += "<legend>";
    else if(l->isA("PieLabel"))
    {
      if (l->text().isEmpty())
        continue;

      s += "<PieLabel>";
    }
    else
      s += "<text>";

    s += "\t" + QString::number(l->x());
    s += "\t" + QString::number(l->y());

    QFont f = l->font();
    s += "\t" + f.family();
    s += "\t" + QString::number(f.pointSize());
    s += "\t" + QString::number(f.weight());
    s += "\t" + QString::number(f.italic());
    s += "\t" + QString::number(f.underline());
    s += "\t" + QString::number(f.strikeOut());
    s += "\t" + l->textColor().name();
    s += "\t" + QString::number(l->frameStyle());
    s += "\t" + QString::number(l->angle());
    s += "\t" + l->backgroundColor().name();
    s += "\t" + QString::number(l->backgroundColor().alpha());

    QStringList textList = l->text().split("\n", QString::KeepEmptyParts);
    s += "\t" + textList.join("\t");
    if(l == d_legend)
      s += "</legend>\n";
    else if (l->isA("PieLabel"))
      s += "</PieLabel>\n";
    else
      s += "</text>\n";

    tsv.writeRaw(s.toStdString());
  }
  return tsv.outputLines();
}
