#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFPlotController.h"

#include "MantidQtCustomInterfaces/MultiDatasetFit/MultiDatasetFit.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFDataController.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFDatasetPlotData.h"

#include "MantidQtMantidWidgets/RangeSelector.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <boost/make_shared.hpp>

#include <QMessageBox>

#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>

namespace{
  // columns in the data table
  const int wsColumn      = 0;
  const int wsIndexColumn = 1;
  const int startXColumn  = 2;
  const int endXColumn    = 3;
  QColor rangeSelectorDisabledColor(Qt::darkGray);
  QColor rangeSelectorEnabledColor(Qt::blue);
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{

/// Constructor
PlotController::PlotController(MultiDatasetFit *parent, QwtPlot *plot,
                               QTableWidget *table, QComboBox *plotSelector,
                               QPushButton *prev, QPushButton *next)
    : QObject(parent), m_plot(plot), m_table(table),
      m_plotSelector(plotSelector), m_prevPlot(prev), m_nextPlot(next),
      m_currentIndex(-1), m_showDataErrors(false)
{
  connect(prev,SIGNAL(clicked()),this,SLOT(prevPlot()));
  connect(next,SIGNAL(clicked()),this,SLOT(nextPlot()));
  connect(plotSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(plotDataSet(int)));

  m_zoomer = new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
      QwtPicker::DragSelection | QwtPicker::CornerToCorner, QwtPicker::AlwaysOff, plot->canvas());

  m_panner = new QwtPlotPanner( plot->canvas() );

  m_magnifier = new QwtPlotMagnifier( plot->canvas() );

  m_rangeSelector = new MantidWidgets::RangeSelector(m_plot);
  m_rangeSelector->setRange( -1e30, 1e30 );
  m_rangeSelector->setMinimum(10);
  m_rangeSelector->setMaximum(990);
  connect(m_rangeSelector,SIGNAL(selectionChanged(double, double)),this,SLOT(updateFittingRange(double, double)));

  disableAllTools();
}

/// Destructor.
PlotController::~PlotController()
{
  m_plotData.clear();
}

/// Slot. Respond to changes in the data table.
void PlotController::tableUpdated()
{
  m_plotSelector->blockSignals(true);
  m_plotSelector->clear();
  int rowCount = m_table->rowCount();
  for(int row = 0; row < rowCount; ++row)
  {
    QString itemText = QString("%1 (%2)").arg(m_table->item(row,wsColumn)->text(),m_table->item(row,wsIndexColumn)->text());
    m_plotSelector->insertItem( itemText );
  }
  m_plotData.clear();
  m_currentIndex = -1;
  m_plotSelector->blockSignals(false);
  plotDataSet( m_plotSelector->currentIndex() );
}

/// Display the previous plot if there is one.
void PlotController::prevPlot()
{
  int index = m_plotSelector->currentIndex();
  if ( index > 0 )
  {
    --index;
    m_plotSelector->setCurrentIndex( index );
  }
}

/// Display the next plot if there is one.
void PlotController::nextPlot()
{
  int index = m_plotSelector->currentIndex();
  if ( index < m_plotSelector->count() - 1 )
  {
    ++index;
    m_plotSelector->setCurrentIndex( index );
  }
}

/// Get a pointer to a dataset data.
/// @param index :: Index of a dataset.
boost::shared_ptr<DatasetPlotData> PlotController::getData(int index)
{
  auto data = boost::shared_ptr<DatasetPlotData>();
  if (index < 0) return data;
  if ( !m_plotData.contains(index) )
  {
    QString wsName = m_table->item( index, wsColumn )->text();
    int wsIndex = m_table->item( index, wsIndexColumn )->text().toInt();
    QString outputWorkspaceName = owner()->getOutputWorkspaceName();
    std::string outName = outputWorkspaceName.toStdString();
    if (!outputWorkspaceName.isEmpty() &&
        Mantid::API::AnalysisDataService::Instance().doesExist(outName)) {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(outName);
      if (auto  group = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws)) {
        outputWorkspaceName = QString::fromStdString(group->getItem(index)->name());
      }
    }
    try {
      data = boost::make_shared<DatasetPlotData>( wsName, wsIndex, outputWorkspaceName );
      m_plotData.insert(index, data );
    }
    catch(std::exception& e)
    {
      QMessageBox::critical(owner(),"MantidPlot - Error",e.what());
      clear();
      owner()->checkSpectra();
      m_plot->replot();
    }
  }
  else
  {
    data = m_plotData[index];
  }

  if (data)
  {
    data->showDataErrorBars(m_showDataErrors);
  }

  return data;
}

/// Plot a data set.
/// @param index :: Index (row) of the data set in the table.
void PlotController::plotDataSet(int index)
{
  if ( index < 0 || index >= m_table->rowCount() )
  {
    clear();
    owner()->checkSpectra();
    m_plot->replot();
    return;
  }

  bool resetZoom = m_plotData.isEmpty();

  auto plotData = getData(index);

  // hide the previously shown data
  if ( m_currentIndex > -1 ) 
  {
    m_plotData[m_currentIndex]->hide();
  }

  // try to keep the zooming from the previous view
  // but if zoom rect doesn't show any data reset zoom base to show all
  auto dataRect = m_plotData[index]->boundingRect();
  auto zoomRect = m_zoomer->zoomRect();
  if ( !zoomRect.intersects( dataRect ) )
  {
    m_plot->setAxisAutoScale(QwtPlot::xBottom);
    m_plot->setAxisAutoScale(QwtPlot::yLeft);
  }
  // change the current data set index
  m_currentIndex = index;
  updateRange(index);
  
  // show the new data
  plotData->show( m_plot );
  m_plot->replot();
  // the idea is to set the zoom base (the largest view) to the data's bounding rect
  // but it looks like the base is set to the union of dataRect and current zoomRect
  m_zoomer->setZoomBase( dataRect );
  // if it's first data set ever set the zoomer's base
  // if it's not done the base is set to some default rect that has nothing to do with the data
  if ( resetZoom ) 
  {
    m_zoomer->setZoomBase(true);
  }
  emit currentIndexChanged( index );
}

/// Clear all plot data.
void PlotController::clear()
{
  m_plotData.clear();
}

/// Update the plot.
void PlotController::update()
{
  plotDataSet( m_currentIndex );
}

/// Reset the fitting range to the current limits on the x axis.
void PlotController::resetRange()
{
  QwtScaleMap xMap = m_plot->canvasMap(QwtPlot::xBottom);
  double startX = xMap.s1();
  double endX = xMap.s2();
  m_rangeSelector->setMinimum( startX );
  m_rangeSelector->setMaximum( endX );
}

/// Set zooming to the current fitting range.
void PlotController::zoomToRange()
{
  QwtDoubleRect rect = m_zoomer->zoomRect();
  rect.setX( m_rangeSelector->getMinimum() );
  rect.setRight( m_rangeSelector->getMaximum() );
  // In case the scales were set by the panning tool we need to
  // reset the zoomer first.
  m_zoomer->zoom(-1);
  // Set new zoom level.
  m_zoomer->zoom(rect);
}

/// Disable all plot tools. It is a helper method 
/// to simplify switchig between tools.
void PlotController::disableAllTools()
{
  m_zoomer->setEnabled(false);
  m_panner->setEnabled(false);
  m_magnifier->setEnabled(false);
  m_rangeSelector->setEnabled(false);
  m_rangeSelector->setColour(rangeSelectorDisabledColor);
}

/// Generic tool enabler.
template<class Tool>
void PlotController::enableTool(Tool* tool, int cursor)
{
  disableAllTools();
  tool->setEnabled(true);
  m_plot->canvas()->setCursor(QCursor(static_cast<Qt::CursorShape>(cursor)));
  m_plot->replot();
  owner()->showPlotInfo();
}


/// Enable zooming tool.
void PlotController::enableZoom()
{
  enableTool(m_zoomer,Qt::CrossCursor);
}

/// Enable panning tool.
void PlotController::enablePan()
{
  enableTool(m_panner,Qt::pointingHandCursor);
  m_magnifier->setEnabled(true);
}

/// Enable range selector tool.
void PlotController::enableRange()
{
  enableTool(m_rangeSelector,Qt::pointingHandCursor);
  m_rangeSelector->setColour(rangeSelectorEnabledColor);
  m_plot->replot();
}

/// Check if zooming tool is on.
bool PlotController::isZoomEnabled() const
{
  return m_zoomer->isEnabled();
}

/// Check if panning tool is on.
bool PlotController::isPanEnabled() const
{
  return m_panner->isEnabled();
}

/// Check if range seletcor is on.
bool PlotController::isRangeSelectorEnabled() const
{
  return m_rangeSelector->isEnabled();
}

/// Signal others that fitting range has been updated.
void PlotController::updateFittingRange(double startX, double endX)
{
  emit fittingRangeChanged(m_currentIndex, startX, endX);
}

/// Sync the range selector with the data in the data table.
/// @param index :: Index of a spectrum that has been updated.
void PlotController::updateRange(int index)
{
  if ( index >= 0 && index == m_currentIndex )
  {
    const double startX = m_table->item(index,startXColumn)->text().toDouble();
    const double endX = m_table->item(index,endXColumn)->text().toDouble();
    m_rangeSelector->blockSignals(true);
    m_rangeSelector->setMinimum(startX);
    m_rangeSelector->setMaximum(endX);
    m_rangeSelector->blockSignals(false);
  }
}

MultiDatasetFit *PlotController::owner() const {return static_cast<MultiDatasetFit*>(parent());}

/// Toggle display of the data error bars.
void PlotController::showDataErrors(bool on)
{
  m_showDataErrors = on;
  if (auto data = getData(m_currentIndex))
  {
    data->show(m_plot);
    m_plot->replot();
  }
}

} // MDF
} // CustomInterfaces
} // MantidQt
