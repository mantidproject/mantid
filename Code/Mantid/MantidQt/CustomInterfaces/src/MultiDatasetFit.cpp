#include "MantidQtCustomInterfaces/MultiDatasetFit.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/Exception.h"

#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtMantidWidgets/FitOptionsBrowser.h"
#include "MantidQtMantidWidgets/FunctionBrowser.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "qtpropertybrowser.h"

#include <QDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QToolBar>
#include <QActionGroup>
#include <QSplitter>
#include <QSettings>

#include <boost/make_shared.hpp>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_scale_div.h>
#include <Poco/ActiveResult.h>

#include <vector>
#include <algorithm>

namespace{
  // columns in the data table
  const int wsColumn      = 0;
  const int wsIndexColumn = 1;
  const int startXColumn  = 2;
  const int endXColumn    = 3;
  // tool options pages
  const int zoomToolPage  = 0;
  const int panToolPage   = 1;
  const int rangeToolPage = 2;
  // colours of the fitting range selector
  QColor rangeSelectorDisabledColor(Qt::darkGray);
  QColor rangeSelectorEnabledColor(Qt::blue);
}

namespace MantidQt
{
namespace CustomInterfaces
{

/*==========================================================================================*/
/*                              AddWorkspaceDialog                                          */
/*==========================================================================================*/
AddWorkspaceDialog::AddWorkspaceDialog(QWidget *parent):QDialog(parent)
{
  m_uiForm.setupUi(this);
  // populate the combo box with names of eligible workspaces
  QStringList workspaceNames;
  auto wsNames = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  for(auto name = wsNames.begin(); name != wsNames.end(); ++name)
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( *name );
    if ( ws )
    {
      workspaceNames << QString::fromStdString( *name );
    }
  }
  connect(m_uiForm.cbWorkspaceName,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(workspaceNameChanged(const QString&)));
  m_uiForm.cbWorkspaceName->addItems( workspaceNames );

  connect(m_uiForm.cbAllSpectra,SIGNAL(stateChanged(int)),this,SLOT(selectAllSpectra(int)));
}

/**
 * Slot. Reacts on change of workspace name in the selection combo box.
 * @param wsName :: Name of newly selected workspace.
 */
void AddWorkspaceDialog::workspaceNameChanged(const QString& wsName)
{
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( wsName.toStdString() );
  if ( ws )
  {
    int maxValue = static_cast<int>(ws->getNumberHistograms()) - 1;
    if ( maxValue < 0 ) maxValue = 0;
    m_maxIndex = maxValue;
    if ( m_uiForm.cbAllSpectra->isChecked() )
    {
      m_uiForm.leWSIndices->setText(QString("0-%1").arg(m_maxIndex));
    }
    else
    {
      m_uiForm.leWSIndices->clear();
    }
  }
  else
  {
    m_maxIndex = 0;
    m_uiForm.leWSIndices->clear();
    m_uiForm.cbAllSpectra->setChecked(false);
  }
}

/**
 * Slot. Called when "All Spectra" check box changes its state
 */
void AddWorkspaceDialog::selectAllSpectra(int state)
{
  if ( state == Qt::Checked )
  {
    m_uiForm.leWSIndices->setText(QString("0-%1").arg(m_maxIndex));
    m_uiForm.leWSIndices->setEnabled(false);
  }
  else
  {
    m_uiForm.leWSIndices->setEnabled(true);
  }

}

/**
 * Called on close if selection accepted.
 */
void AddWorkspaceDialog::accept()
{
  m_workspaceName = m_uiForm.cbWorkspaceName->currentText();
  m_wsIndices.clear();
  QString indexInput = m_uiForm.leWSIndices->text();
  if ( !m_workspaceName.isEmpty() && !indexInput.isEmpty() )
  {
    auto validator = boost::make_shared<Mantid::Kernel::ArrayBoundedValidator<int>>(0,m_maxIndex);
    Mantid::Kernel::ArrayProperty<int> prop("Indices",validator);
    std::string err = prop.setValue( indexInput.toStdString() );
    if ( err.empty() )
    {
      m_wsIndices = prop;
    }
    else
    {
      QMessageBox::warning(this, "MantidPlot - Error", QString("Some of the indices are outside the allowed range [0,%1]").arg(m_maxIndex));
    }
  }
  QDialog::accept();
}

/**
 * Called on close if selection rejected.
 */
void AddWorkspaceDialog::reject()
{
  m_workspaceName.clear();
  m_wsIndices.clear();
  QDialog::reject();
}

/*==========================================================================================*/
/*                                DatasetPlotData                                           */
/*==========================================================================================*/

/**
 * Contains graphics for a single data set: fitting data, claculated result, difference.
 */
class DatasetPlotData
{
public:
  DatasetPlotData(const QString& wsName, int wsIndex, const QString& outputWSName);
  ~DatasetPlotData();
  void show(QwtPlot *plot);
  void hide();
  QwtDoubleRect boundingRect() const;
private:
  // no copying
  DatasetPlotData(const DatasetPlotData&);
  DatasetPlotData& operator=(const DatasetPlotData&);
  void setData(const Mantid::API::MatrixWorkspace *ws, int wsIndex, const Mantid::API::MatrixWorkspace *outputWS);
  QwtPlotCurve *m_dataCurve;
  QwtPlotCurve *m_calcCurve;
  QwtPlotCurve *m_diffCurve;
};

/**
 * Constructor.
 * @param wsName :: Name of a MatrixWorkspace with the data for fitting.
 * @param wsIndex :: Workspace index of a spectrum in wsName to plot.
 * @param outputWSName :: Name of the Fit's output workspace containing at least 3 spectra:
 *    #0 - original data (the same as in wsName[wsIndex]), #1 - calculated data, #3 - difference.
 *    If empty - ignore this workspace.
 */
DatasetPlotData::DatasetPlotData(const QString& wsName, int wsIndex, const QString& outputWSName):
  m_dataCurve(new QwtPlotCurve(wsName + QString(" (%1)").arg(wsIndex))),
  m_calcCurve(NULL),
  m_diffCurve(NULL)
{
  // get the data workspace
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( wsName.toStdString() );
  if ( !ws )
  {
    QString mess = QString("Workspace %1 either doesn't exist or isn't a MatrixWorkspace").arg(wsName);
    throw std::runtime_error( mess.toStdString() );
  }
  // check that the index is in range
  if ( static_cast<size_t>(wsIndex) >= ws->getNumberHistograms() )
  {
    QString mess = QString("Spectrum %1 doesn't exist in workspace %2").arg(wsIndex).arg(wsName);
    throw std::runtime_error( mess.toStdString() );
  }

  // get the data workspace
  Mantid::API::MatrixWorkspace_sptr outputWS;
  if ( !outputWSName.isEmpty() )
  {
    std::string stdOutputWSName = outputWSName.toStdString();
    if ( Mantid::API::AnalysisDataService::Instance().doesExist(stdOutputWSName) )
    {
      try
      {
        outputWS = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( stdOutputWSName );
      }
      catch ( Mantid::Kernel::Exception::NotFoundError& )
      {
        QString mess = QString("Workspace %1 either doesn't exist or isn't a MatrixWorkspace").arg(outputWSName);
        throw std::runtime_error( mess.toStdString() );
      }
    }
  }

  // create the curves
  setData( ws.get(), wsIndex, outputWS.get() );

}

/**
 * Destructor.
 */
DatasetPlotData::~DatasetPlotData()
{
  m_dataCurve->detach();
  delete m_dataCurve;
  if ( m_calcCurve )
  {
    m_calcCurve->detach();
    delete m_calcCurve;
  }
  if ( m_diffCurve )
  {
    m_diffCurve->detach();
    delete m_diffCurve;
  }
}

/**
 * Set the data to the curves.
 * @param ws :: A Fit's input workspace.
 * @param wsIndex :: Workspace index of a spectrum to costruct the plot data for.
 * @param outputWS :: The output workspace from Fit containing the calculated spectrum.
 */
void DatasetPlotData::setData(const Mantid::API::MatrixWorkspace *ws, int wsIndex, const Mantid::API::MatrixWorkspace *outputWS)
{
  bool haveFitCurves = outputWS && outputWS->getNumberHistograms() >= 3;
  std::vector<double> xValues = ws->readX(wsIndex);
  if ( ws->isHistogramData() )
  {
    auto xend = xValues.end() - 1;
    for(auto x = xValues.begin(); x != xend; ++x)
    {
      *x = (*x + *(x+1))/2;
    }
    xValues.pop_back();
  }
  m_dataCurve->setData( xValues.data(), ws->readY(wsIndex).data(), static_cast<int>(xValues.size()) );

  if ( haveFitCurves )
  {
    auto xBegin = std::lower_bound(xValues.begin(),xValues.end(), outputWS->readX(1).front());
    if ( xBegin == xValues.end() ) return;
    int i0 = static_cast<int>(std::distance(xValues.begin(),xBegin));
    int n = static_cast<int>(outputWS->readY(1).size());
    if ( i0 + n > static_cast<int>(xValues.size()) ) return;
    m_calcCurve = new QwtPlotCurve("calc");
    m_calcCurve->setData( xValues.data() + i0, outputWS->readY(1).data(), n );
    QPen penCalc("red");
    m_calcCurve->setPen(penCalc);
    m_diffCurve = new QwtPlotCurve("diff");
    m_diffCurve->setData( xValues.data() + i0, outputWS->readY(2).data(), n );
    QPen penDiff("green");
    m_diffCurve->setPen(penDiff);
  }
}

/**
 * Show the curves on a plot.
 */
void DatasetPlotData::show(QwtPlot *plot)
{
  m_dataCurve->attach(plot);
  if ( m_calcCurve )
  {
    m_calcCurve->attach(plot);
  }
  if ( m_diffCurve )
  {
    m_diffCurve->attach(plot);
  }
}

/**
 * Hide the curves from any plot.
 */
void DatasetPlotData::hide()
{
  m_dataCurve->detach();
  if ( m_calcCurve )
  {
    m_calcCurve->detach();
  }
  if ( m_diffCurve )
  {
    m_diffCurve->detach();
  }
}

/**
 * Get the bounding rect including all plotted data.
 */
QwtDoubleRect DatasetPlotData::boundingRect() const
{
  QwtDoubleRect rect = m_dataCurve->boundingRect();
  if ( m_calcCurve )
  {
    rect = rect.united( m_calcCurve->boundingRect() );
  }
  if ( m_diffCurve )
  {
    rect = rect.united( m_diffCurve->boundingRect() );
  }
  return rect;
}

/*==========================================================================================*/
/*                                PlotController                                            */
/*==========================================================================================*/

PlotController::PlotController(MultiDatasetFit *parent, 
                               QwtPlot *plot, 
                               QTableWidget *table, 
                               QComboBox *plotSelector, 
                               QPushButton *prev, 
                               QPushButton *next):
  QObject(parent),m_plot(plot),m_table(table),m_plotSelector(plotSelector),m_prevPlot(prev),m_nextPlot(next),m_currentIndex(-1)
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

  m_plot->canvas()->installEventFilter(this);
  
}

PlotController::~PlotController()
{
  m_plotData.clear();
}

bool PlotController::eventFilter(QObject *, QEvent *evn)
{
  if ( evn->type() == QEvent::MouseButtonDblClick )
  {
    if ( isRangeSelectorEnabled() )
    {
      resetRange();
    }
    else if ( isZoomEnabled() )
    {
      zoomToRange();
    }
  }
  return false;
}

/**
 * Slot. Respond to changes in the data table.
 */
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

/**
 * Display the previous plot if there is one.
 */
void PlotController::prevPlot()
{
  int index = m_plotSelector->currentIndex();
  if ( index > 0 )
  {
    --index;
    m_plotSelector->setCurrentIndex( index );
  }
}

/**
 * Display the next plot if there is one.
 */
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
  if ( !m_plotData.contains(index) )
  {
    QString wsName = m_table->item( index, wsColumn )->text();
    int wsIndex = m_table->item( index, wsIndexColumn )->text().toInt();
    QString outputWorkspaceName = owner()->getOutputWorkspaceName();
    if ( !outputWorkspaceName.isEmpty() )
    {
      outputWorkspaceName += QString("_%1").arg(index);
    }
    try
    {
      auto value = boost::make_shared<DatasetPlotData>( wsName, wsIndex, outputWorkspaceName );
      m_plotData.insert(index, value );
      return value;
    }
    catch(std::exception& e)
    {
      QMessageBox::critical(owner(),"MantidPlot - Error",e.what());
      clear();
      owner()->checkDataSets();
      m_plot->replot();
      return boost::shared_ptr<DatasetPlotData>();
    }
  }

  return m_plotData[index];
}

/**
 * Plot a data set.
 * @param index :: Index (row) of the data set in the table.
 */
void PlotController::plotDataSet(int index)
{
  if ( index < 0 || index >= m_table->rowCount() )
  {
    clear();
    owner()->checkDataSets();
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

void PlotController::clear()
{
  m_plotData.clear();
}

void PlotController::update()
{
  plotDataSet( m_currentIndex );
}

/**
 * Reset the fitting range to the current limits on the x axis.
 */
void PlotController::resetRange()
{
  QwtScaleMap xMap = m_plot->canvasMap(QwtPlot::xBottom);
  double startX = xMap.s1();
  double endX = xMap.s2();
  m_rangeSelector->setMinimum( startX );
  m_rangeSelector->setMaximum( endX );
}

/**
 * Set zooming to the current fitting range.
 */
void PlotController::zoomToRange()
{
  QwtDoubleRect rect = m_zoomer->zoomRect();
  rect.setX( m_rangeSelector->getMinimum() );
  rect.setRight( m_rangeSelector->getMaximum() );
  m_zoomer->zoom( rect );
}

/**
 * Disable all plot tools. It is a helper method 
 * to simplify switchig between tools.
 */
void PlotController::disableAllTools()
{
  m_zoomer->setEnabled(false);
  m_panner->setEnabled(false);
  m_magnifier->setEnabled(false);
  m_rangeSelector->setEnabled(false);
  m_rangeSelector->setColour(rangeSelectorDisabledColor);
}

template<class Tool>
void PlotController::enableTool(Tool* tool, int cursor)
{
  disableAllTools();
  tool->setEnabled(true);
  m_plot->canvas()->setCursor(QCursor(static_cast<Qt::CursorShape>(cursor)));
  m_plot->replot();
  owner()->showPlotInfo();
}


/**
 * Enable zooming tool.
 */
void PlotController::enableZoom()
{
  enableTool(m_zoomer,Qt::CrossCursor);
}

/**
 * Enable panning tool.
 */
void PlotController::enablePan()
{
  enableTool(m_panner,Qt::pointingHandCursor);
  m_magnifier->setEnabled(true);
}

/**
 * Enable range selector tool.
 */
void PlotController::enableRange()
{
  enableTool(m_rangeSelector,Qt::pointingHandCursor);
  m_rangeSelector->setColour(rangeSelectorEnabledColor);
  m_plot->replot();
}

bool PlotController::isZoomEnabled() const
{
  return m_zoomer->isEnabled();
}

bool PlotController::isPanEnabled() const
{
  return m_panner->isEnabled();
}

bool PlotController::isRangeSelectorEnabled() const
{
  return m_rangeSelector->isEnabled();
}

/// Signal others that fitting range has been updated.
void PlotController::updateFittingRange(double startX, double endX)
{
  emit fittingRangeChanged(m_currentIndex, startX, endX);
}

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


/*==========================================================================================*/
/*                               EditLocalParameterDialog                                   */
/*==========================================================================================*/

EditLocalParameterDialog::EditLocalParameterDialog(MultiDatasetFit *parent, const QString &parName):
  QDialog(parent),m_parName(parName)
{
  m_uiForm.setupUi(this);
  QHeaderView *header = m_uiForm.tableWidget->horizontalHeader();
  header->setResizeMode(0,QHeaderView::Stretch);
  header->setResizeMode(1,QHeaderView::Stretch);
  connect(m_uiForm.tableWidget,SIGNAL(cellChanged(int,int)),this,SLOT(valueChanged(int,int)));

  auto multifit = owner();
  auto n = static_cast<int>( multifit->getNumberOfSpectra() );
  for(int i = 0; i < n; ++i)
  {
    m_uiForm.tableWidget->insertRow(i);
    auto cell = new QTableWidgetItem( QString("f%1.").arg(i) + parName );
    m_uiForm.tableWidget->setItem( i, 0, cell );
    cell = new QTableWidgetItem( QString::number(multifit->getLocalParameterValue(parName,i)) );
    m_uiForm.tableWidget->setItem( i, 1, cell );
  }
}

/**
 * Slot. Called when a value changes.
 * @param row :: Row index of the changed cell.
 * @param col :: Column index of the changed cell.
 */
void EditLocalParameterDialog::valueChanged(int row, int col)
{
  if ( col == 1 )
  {
    QString text = m_uiForm.tableWidget->item(row,col)->text();
    try
    {
      double value = text.toDouble();
      owner()->setLocalParameterValue(m_parName,row,value);
    }
    catch(std::exception&)
    {
      // restore old value
      m_uiForm.tableWidget->item(row,col)->setText( QString::number(owner()->getLocalParameterValue(m_parName,row)) );
    }
  }
}

/*==========================================================================================*/
/*                                    MultiDatasetFit                                       */
/*==========================================================================================*/

//Register the class with the factory
DECLARE_SUBWINDOW(MultiDatasetFit);

/**
 * Constructor
 * @param parent :: The parent widget
 */
MultiDatasetFit::MultiDatasetFit(QWidget *parent)
:UserSubWindow(parent)
{
}

MultiDatasetFit::~MultiDatasetFit()
{
  saveSettings();
  m_plotController->clear();
}

/**
 * Initilize the layout.
 */
void MultiDatasetFit::initLayout()
{
  m_uiForm.setupUi(this);
  m_uiForm.hSplitter->setStretchFactor(0,0);
  m_uiForm.hSplitter->setStretchFactor(1,1);
  m_uiForm.vSplitter->setStretchFactor(0,0);
  m_uiForm.vSplitter->setStretchFactor(1,1);

  QHeaderView *header = m_uiForm.dataTable->horizontalHeader();
  header->setResizeMode(0,QHeaderView::Stretch);
  header->setResizeMode(1,QHeaderView::Fixed);

  m_uiForm.btnRemove->setEnabled( false );

  connect(m_uiForm.btnFit,SIGNAL(clicked()),this,SLOT(fit()));

  m_dataController = new DataController(this, m_uiForm.dataTable);
  connect(m_dataController,SIGNAL(dataTableUpdated()),this,SLOT(reset()));
  connect(m_dataController,SIGNAL(hasSelection(bool)),  m_uiForm.btnRemove, SLOT(setEnabled(bool)));
  connect(m_uiForm.btnAddWorkspace,SIGNAL(clicked()),m_dataController,SLOT(addWorkspace()));
  connect(m_uiForm.btnRemove,SIGNAL(clicked()),m_dataController,SLOT(removeSelectedSpectra()));
  connect(m_uiForm.cb_applyRangeToAll,SIGNAL(toggled(bool)),m_dataController,SLOT(setFittingRangeGlobal(bool)));

  m_plotController = new PlotController(this,
                                        m_uiForm.plot,
                                        m_uiForm.dataTable,
                                        m_uiForm.cbPlotSelector,
                                        m_uiForm.btnPrev,
                                        m_uiForm.btnNext);
  connect(m_dataController,SIGNAL(dataTableUpdated()),m_plotController,SLOT(tableUpdated()));
  connect(m_dataController,SIGNAL(dataSetUpdated(int)),m_plotController,SLOT(updateRange(int)));
  connect(m_plotController,SIGNAL(fittingRangeChanged(int, double, double)),m_dataController,SLOT(setFittingRange(int, double, double)));
  connect(m_plotController,SIGNAL(currentIndexChanged(int)),this,SLOT(updateLocalParameters(int)));

  QSplitter* splitter = new QSplitter(Qt::Vertical,this);

  m_functionBrowser = new MantidQt::MantidWidgets::FunctionBrowser(NULL, true);
  splitter->addWidget( m_functionBrowser );
  connect(m_functionBrowser,SIGNAL(localParameterButtonClicked(const QString&)),this,SLOT(editLocalParameterValues(const QString&)));
  connect(m_functionBrowser,SIGNAL(functionStructureChanged()),this,SLOT(reset()));

  m_fitOptionsBrowser = new MantidQt::MantidWidgets::FitOptionsBrowser(NULL);
  splitter->addWidget( m_fitOptionsBrowser );

  m_uiForm.browserLayout->addWidget( splitter );

  createPlotToolbar();

  // filters
  m_functionBrowser->installEventFilter( this );
  m_fitOptionsBrowser->installEventFilter( this );
  m_uiForm.plot->installEventFilter( this );
  m_uiForm.dataTable->installEventFilter( this );

  m_plotController->enableZoom();
  showInfo( "Add some data, define fitting function" );

  loadSettings();
}

void MultiDatasetFit::createPlotToolbar()
{
  // ----- Main tool bar --------
  auto toolBar = new QToolBar(this);
  toolBar->setIconSize(QSize(16,16));
  auto group = new QActionGroup(this);
 
  auto action = new QAction(this);
  action->setIcon(QIcon(":/MultiDatasetFit/icons/zoom.png"));
  action->setCheckable(true);
  action->setChecked(true);
  action->setToolTip("Zooming tool");
  connect(action,SIGNAL(triggered()),this,SLOT(enableZoom()));
  group->addAction(action);

  action = new QAction(this);
  action->setIcon(QIcon(":/MultiDatasetFit/icons/panning.png"));
  action->setCheckable(true);
  action->setToolTip("Panning tool");
  connect(action,SIGNAL(triggered()),this,SLOT(enablePan()));
  group->addAction(action);

  action = new QAction(this);
  action->setIcon(QIcon(":/MultiDatasetFit/icons/range.png"));
  action->setCheckable(true);
  action->setToolTip("Set fitting range");
  connect(action,SIGNAL(triggered()),this,SLOT(enableRange()));
  group->addAction(action);

  toolBar->addActions(group->actions());

  m_uiForm.horizontalLayout->insertWidget(3,toolBar);

}

/**
 * Create a multi-domain function to fit all the spectra in the data table.
 */
boost::shared_ptr<Mantid::API::IFunction> MultiDatasetFit::createFunction() const
{
  // number of spectra to fit == size of the multi-domain function
  size_t nOfDataSets = getNumberOfSpectra();
  if ( nOfDataSets == 0 )
  {
    throw std::runtime_error("There are no data sets specified.");
  }

  // description of a single function
  QString funStr = m_functionBrowser->getFunctionString();

  if ( nOfDataSets == 1 )
  {
    return Mantid::API::FunctionFactory::Instance().createInitialized( funStr.toStdString() );
  }

  bool isComposite = (std::find(funStr.begin(),funStr.end(),';') != funStr.end());
  if ( isComposite )
  {
    funStr = ";(" + funStr + ")";
  }
  else
  {
    funStr = ";" + funStr;
  }

  QString multiFunStr = "composite=MultiDomainFunction,NumDeriv=1";
  for(size_t i = 0; i < nOfDataSets; ++i)
  {
    multiFunStr += funStr;
  }

  // add the global ties
  QStringList globals = m_functionBrowser->getGlobalParameters();
  QString globalTies;
  if ( !globals.isEmpty() )
  {
    globalTies = "ties=(";
    bool isFirst = true;
    foreach(QString par, globals)
    {
      if ( !isFirst ) globalTies += ",";
      else
        isFirst = false;

      for(size_t i = 1; i < nOfDataSets; ++i)
      {
        globalTies += QString("f%1.").arg(i) + par + "=";
      }
      globalTies += QString("f0.%1").arg(par);
    }
    globalTies += ")";
    multiFunStr += ";" + globalTies;
  }

  // create the multi-domain function
  std::string tmpStr = multiFunStr.toStdString();
  auto fun = Mantid::API::FunctionFactory::Instance().createInitialized( tmpStr );
  boost::shared_ptr<Mantid::API::MultiDomainFunction> multiFun = boost::dynamic_pointer_cast<Mantid::API::MultiDomainFunction>( fun );
  if ( !multiFun )
  {
    throw std::runtime_error("Failed to create the MultiDomainFunction");
  }
  
  auto globalParams = m_functionBrowser->getGlobalParameters();

  // set the domain indices, initial local parameter values and ties
  for(size_t i = 0; i < nOfDataSets; ++i)
  {
    multiFun->setDomainIndex(i,i);
    auto fun1 = multiFun->getFunction(i);
    for(size_t j = 0; j < fun1->nParams(); ++j)
    {
      auto tie = fun1->getTie(j);
      if ( tie )
      {
        // if a local parameter has a constant tie (is fixed) set tie's value to 
        // the value of the local parameter
        if ( tie->isConstant() )
        {
          QString parName = QString::fromStdString(fun1->parameterName(j));
          if ( !globalParams.contains(parName) )
          {
            std::string expr = boost::lexical_cast<std::string>( getLocalParameterValue(parName,static_cast<int>(i)) );
            tie->set( expr );
          }
        }
      }
      else
      {
        // if local parameter isn't tied set its local value
        QString parName = QString::fromStdString(fun1->parameterName(j));
        if ( !globalParams.contains(parName) )
        {
          fun1->setParameter(j, getLocalParameterValue(parName,static_cast<int>(i)));
        }
      }
    }
  }
  assert( multiFun->nFunctions() == nOfDataSets );

  return fun;
}

/**
 * Run the fitting algorithm.
 */
void MultiDatasetFit::fit()
{
  if ( !m_functionBrowser->hasFunction() )
  {
    QMessageBox::warning( this, "MantidPlot - Warning","Function wasn't set." );
    return;
  }

  try
  {
    auto fun = createFunction();
    auto fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();
    fit->setProperty("Function", fun );
    fit->setPropertyValue("InputWorkspace", getWorkspaceName(0));
    fit->setProperty("WorkspaceIndex", getWorkspaceIndex(0));
    auto range = getFittingRange(0);
    fit->setProperty( "StartX", range.first );
    fit->setProperty( "EndX", range.second );

    int n = getNumberOfSpectra();
    for(int ispec = 1; ispec < n; ++ispec)
    {
      std::string suffix = boost::lexical_cast<std::string>(ispec);
      fit->setPropertyValue( "InputWorkspace_" + suffix, getWorkspaceName(ispec) );
      fit->setProperty( "WorkspaceIndex_" + suffix, getWorkspaceIndex(ispec) );
      auto range = getFittingRange(ispec);
      fit->setProperty( "StartX_" + suffix, range.first );
      fit->setProperty( "EndX_" + suffix, range.second );
    }

    m_fitOptionsBrowser->copyPropertiesToAlgorithm(*fit);

    m_outputWorkspaceName = m_fitOptionsBrowser->getProperty("Output").toStdString();
    if ( m_outputWorkspaceName.empty() )
    {
      m_outputWorkspaceName = "out";
      fit->setPropertyValue("Output",m_outputWorkspaceName);
      m_fitOptionsBrowser->setProperty("Output","out");
    }
    m_outputWorkspaceName += "_Workspace";

    m_fitRunner.reset( new API::AlgorithmRunner() );
    connect( m_fitRunner.get(),SIGNAL(algorithmComplete(bool)), this, SLOT(finishFit(bool)), Qt::QueuedConnection );

    m_fitRunner->startAlgorithm(fit);

  }
  catch(std::exception& e)
  {
    QString mess(e.what());
    const int maxSize = 500;
    if ( mess.size() > maxSize )
    {
      mess = mess.mid(0,maxSize);
      mess += "...";
    }
    QMessageBox::critical( this, "MantidPlot - Error", QString("Fit failed:\n\n  %1").arg(mess) );
  }
}

/**
 * Get the workspace name of the i-th spectrum.
 * @param i :: Index of a spectrum in the data table.
 */
std::string MultiDatasetFit::getWorkspaceName(int i) const
{
  return m_dataController->getWorkspaceName(i);
}

/**
 * Get the workspace index of the i-th spectrum.
 * @param i :: Index of a spectrum in the data table.
 */
int MultiDatasetFit::getWorkspaceIndex(int i) const
{
  return m_dataController->getWorkspaceIndex(i);
}

/// Get the fitting range for the i-th spectrum
/// @param i :: Index of a spectrum in the data table.
std::pair<double,double> MultiDatasetFit::getFittingRange(int i) const
{
  return m_dataController->getFittingRange(i);
}

/**
 * Get the number of spectra to fit to.
 */
int MultiDatasetFit::getNumberOfSpectra() const
{
  return m_dataController->getNumberOfSpectra();
}

/**
 * Start an editor to display and edit individual local parameter values.
 * @param parName :: Fully qualified name for a local parameter (Global unchecked).
 */
void MultiDatasetFit::editLocalParameterValues(const QString& parName)
{
  EditLocalParameterDialog dialog(this,parName);
  dialog.exec();
}

/**
 * Get value of a local parameter
 * @param parName :: Name of a parameter.
 * @param i :: Data set index.
 */
double MultiDatasetFit::getLocalParameterValue(const QString& parName, int i) const
{
  if ( !m_localParameterValues.contains(parName) || m_localParameterValues[parName].size() != getNumberOfSpectra() )
  {
    initLocalParameter(parName);
  }
  return m_localParameterValues[parName][i];
}

void MultiDatasetFit::setLocalParameterValue(const QString& parName, int i, double value)
{
  if ( !m_localParameterValues.contains(parName) || m_localParameterValues[parName].size() != getNumberOfSpectra() )
  {
    initLocalParameter(parName);
  }
  m_localParameterValues[parName][i] = value;
}

/**
 * Init a local parameter. Define initial values for all datasets.
 * @param parName :: Name of parametere to init.
 */
void MultiDatasetFit::initLocalParameter(const QString& parName)const
{
  double value = m_functionBrowser->getParameter(parName);
  QVector<double> values( static_cast<int>(getNumberOfSpectra()), value );
  m_localParameterValues[parName] = values;
}

void MultiDatasetFit::reset()
{
  m_localParameterValues.clear();
}

/**
 * Slot called on completion of the Fit algorithm.
 * @param error :: Set to true if Fit finishes with an error.
 */
void MultiDatasetFit::finishFit(bool error)
{
  if ( !error )
  {
    m_plotController->clear();
    m_plotController->update();
    Mantid::API::IFunction_sptr fun = m_fitRunner->getAlgorithm()->getProperty("Function");
    updateParameters( *fun );
  }
}

/**
 * Update the interface to have the sametparameter values as in a function.
 */
void MultiDatasetFit::updateParameters(const Mantid::API::IFunction& fun)
{
  m_localParameterValues.clear();
  auto cfun = dynamic_cast<const Mantid::API::CompositeFunction*>( &fun );
  if ( cfun && cfun->nFunctions() > 0 )
  {
    auto qLocalParameters = m_functionBrowser->getLocalParameters();
    std::vector<std::string> localParameters;
    foreach(QString par, qLocalParameters)
    {
      localParameters.push_back( par.toStdString() );
    }
    size_t currentIndex = static_cast<size_t>( m_plotController->getCurrentIndex() );
    for(size_t i = 0; i < cfun->nFunctions(); ++i)
    {
      auto sfun = cfun->getFunction(i);
      if ( i == currentIndex )
      {
        m_functionBrowser->updateParameters( *sfun );
      }
      for(int j = 0; j < qLocalParameters.size(); ++j)
      {
        setLocalParameterValue( qLocalParameters[j], static_cast<int>(i), sfun->getParameter(localParameters[j]) );
      }
    }
  }
  else
  {
    m_functionBrowser->updateParameters( fun );
  }
}

/**
 * Update the local parameters in the function browser to show values corresponding
 * to a particular dataset.
 * @param index :: Index of a dataset.
 */
void MultiDatasetFit::updateLocalParameters(int index)
{
  auto localParameters = m_functionBrowser->getLocalParameters();
  foreach(QString par, localParameters)
  {
    m_functionBrowser->setParameter( par, getLocalParameterValue( par, index ) );
  }
}

/**
 * Show a message in the info bar at the bottom of the interface.
 */
void MultiDatasetFit::showInfo(const QString& text)
{
  m_uiForm.infoBar->setText(text);
}

bool MultiDatasetFit::eventFilter(QObject *widget, QEvent *evn)
{
  if ( evn->type() == QEvent::Enter )
  {
    if ( qobject_cast<QObject*>( m_functionBrowser ) == widget )
    {
      showFunctionBrowserInfo();
    }
    else if ( qobject_cast<QObject*>( m_fitOptionsBrowser ) == widget )
    {
      showFitOptionsBrowserInfo();
    }
    else if ( qobject_cast<QObject*>( m_uiForm.plot ) == widget )
    {
      showPlotInfo();
    }
    else if ( qobject_cast<QObject*>( m_uiForm.dataTable ) == widget )
    {
      showTableInfo();
    }
    else
    {
      showInfo("");
    }
  }
  return false;
}

/**
 * Show info about the function browser.
 */
void MultiDatasetFit::showFunctionBrowserInfo()
{
  if ( m_functionBrowser->hasFunction() )
  {
    showInfo( "Use context menu to add more functions. Set parameters and attributes." );
  }
  else
  {
    showInfo( "Use context menu to add a function." );
  }
}

/**
 * Show info about the Fit options browser.
 */
void MultiDatasetFit::showFitOptionsBrowserInfo()
{
  showInfo( "Set Fit properties." );
}

/**
 * Show info about the plot.
 */
void MultiDatasetFit::showPlotInfo()
{
  QString text = "Use Alt+. and Alt+, to change the data set. ";

  if ( m_plotController->isZoomEnabled() )
  {
    text += "Click and drag to zoom in. Use middle or right button to zoom out";
  }
  else if ( m_plotController->isPanEnabled() )
  {
    text += "Click and drag to move. Use mouse wheel to zoom in and out.";
  }
  else if ( m_plotController->isRangeSelectorEnabled() )
  {
    text += "Drag the vertical dashed lines to adjust the fitting range.";
  }
  
  showInfo( text );
}

void MultiDatasetFit::showTableInfo()
{
  if ( getNumberOfSpectra() > 0 )
  {
    showInfo("Select spectra by selecting rows. For multiple selection use Shift or Ctrl keys.");
  }
  else
  {
    showInfo("Add some data sets. Click \"Add Workspace\" button.");
  }
}

/**
 * Check that the data sets in the table are valid and remove invalid ones.
 */
void MultiDatasetFit::checkDataSets()
{
  m_dataController->checkDataSets();
}

void MultiDatasetFit::enableZoom()
{
  m_plotController->enableZoom();
  m_uiForm.toolOptions->setCurrentIndex(zoomToolPage);
}

void MultiDatasetFit::enablePan()
{
  m_plotController->enablePan();
  m_uiForm.toolOptions->setCurrentIndex(panToolPage);
}

void MultiDatasetFit::enableRange()
{
  m_plotController->enableRange();
  m_uiForm.toolOptions->setCurrentIndex(rangeToolPage);
}

/*==========================================================================================*/
/*                                    DataController                                        */
/*==========================================================================================*/

DataController::DataController(MultiDatasetFit *parent, QTableWidget *dataTable):
  QObject(parent),m_dataTable(dataTable),m_isFittingRangeGlobal(false)
{
  connect(dataTable,SIGNAL(itemSelectionChanged()), this,SLOT(workspaceSelectionChanged()));
  connect(dataTable,SIGNAL(cellChanged(int,int)),this,SLOT(updateDataset(int,int)));
}

/**
 * Show a dialog to select a workspace from the ADS.
 */
void DataController::addWorkspace()
{
  AddWorkspaceDialog dialog(owner());
  if ( dialog.exec() == QDialog::Accepted )
  {
    QString wsName = dialog.workspaceName().stripWhiteSpace();
    // if name is empty assume that there are no workspaces in the ADS
    if ( wsName.isEmpty() ) return;
    if ( Mantid::API::AnalysisDataService::Instance().doesExist( wsName.toStdString()) )
    {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( wsName.toStdString() );
      auto indices = dialog.workspaceIndices();
      for(auto i = indices.begin(); i != indices.end(); ++i)
      {
        addWorkspaceSpectrum( wsName, *i, *ws );
      }
      emit dataTableUpdated();
    }
    else
    {
      QMessageBox::warning(owner(),"MantidPlot - Warning",QString("Workspace \"%1\" doesn't exist.").arg(wsName));
    }
  }
}

/**
 * Add a spectrum from a workspace to the table.
 * @param wsName :: Name of a workspace.
 * @param wsIndex :: Index of a spectrum in the workspace (workspace index).
 * @param ws :: The workspace.
 */
void DataController::addWorkspaceSpectrum(const QString &wsName, int wsIndex, const Mantid::API::MatrixWorkspace& ws)
{
  int row = m_dataTable->rowCount();
  m_dataTable->insertRow(row);

  auto cell = new QTableWidgetItem( wsName );
  m_dataTable->setItem( row, wsColumn, cell );
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);

  cell = new QTableWidgetItem( QString::number(wsIndex) );
  m_dataTable->setItem( row, wsIndexColumn, cell );
  flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);

  const double startX = ws.readX(wsIndex).front();
  cell = new QTableWidgetItem( QString::number(startX) );
  m_dataTable->setItem( row, startXColumn, cell );

  const double endX = ws.readX(wsIndex).back();
  cell = new QTableWidgetItem( QString::number(endX) );
  m_dataTable->setItem( row, endXColumn, cell );
}

/**
 * Slot. Called when selection in the data table changes.
 */
void DataController::workspaceSelectionChanged()
{
  auto selection = m_dataTable->selectionModel();
  bool enableRemoveButton = selection->hasSelection();
  if ( enableRemoveButton )
  {
    enableRemoveButton = selection->selectedRows().size() > 0;
  }

  emit hasSelection(enableRemoveButton);
}

/**
 * Slot. Called when "Remove" button is pressed.
 */
void DataController::removeSelectedSpectra()
{
  auto ranges = m_dataTable->selectedRanges();
  if ( ranges.isEmpty() ) return;
  std::vector<int> rows;
  for(auto range = ranges.begin(); range != ranges.end(); ++range)
  {
    for(int row = range->topRow(); row <= range->bottomRow(); ++row)
    {
      rows.push_back( row );
    }
  }
  removeDataSets( rows );
}

void DataController::removeDataSets( std::vector<int>& rows )
{
  if ( rows.empty() ) return;
  std::sort( rows.begin(), rows.end() );
  for(auto row = rows.rbegin(); row != rows.rend(); ++row)
  {
    m_dataTable->removeRow( *row );
  }
  emit dataTableUpdated();
}

/**
 * Check that the data sets in the table are valid and remove invalid ones.
 */
void DataController::checkDataSets()
{
  std::vector<int> rows;
  int nrows = getNumberOfSpectra();
  auto& ADS = Mantid::API::AnalysisDataService::Instance();
  for( int row = 0; row < nrows; ++row)
  {
    auto wsName = getWorkspaceName( row );
    auto i = getWorkspaceIndex( row );
    if ( !ADS.doesExist( wsName ) )
    {
      rows.push_back( row );
      continue;
    }
    auto ws = ADS.retrieveWS<Mantid::API::MatrixWorkspace>( wsName );
    if ( !ws || i >= static_cast<int>( ws->getNumberHistograms() ) )
    {
      rows.push_back( row );
      continue;
    }
  }

  removeDataSets( rows );
}

/**
 * Get the workspace name of the i-th spectrum.
 * @param i :: Index of a spectrum in the data table.
 */
std::string DataController::getWorkspaceName(int i) const
{
  return m_dataTable->item(i, wsColumn)->text().toStdString();
}

/**
 * Get the workspace index of the i-th spectrum.
 * @param i :: Index of a spectrum in the data table.
 */
int DataController::getWorkspaceIndex(int i) const
{
  return m_dataTable->item(i, wsIndexColumn)->text().toInt();
}

/**
 * Get the number of spectra to fit to.
 */
int DataController::getNumberOfSpectra() const
{
  return m_dataTable->rowCount();
}

/**
 * Enable global setting of fitting range (calls to setFittingRage(...)
 * will set ranges of all datasets.)
 * @param on :: True for global setting, false for individual.
 */
void DataController::setFittingRangeGlobal(bool on)
{
  m_isFittingRangeGlobal = on;
}

/**
 * Set the fitting range for a data set or all data sets.
 * @param i :: Index of a data set (spectrum). If m_isFittingRangeGlobal == true
 *   the index is ignored and fitting range is set for all spectra.
 * @param startX :: Start of the fitting range.
 * @param endX :: End of the fitting range.
 */
void DataController::setFittingRange(int i, double startX, double endX)
{
  if ( i < 0 || i >= m_dataTable->rowCount() ) return;
  auto start = QString::number(startX);
  auto end = QString::number(endX);
  if ( m_isFittingRangeGlobal )
  {
    for(int k = 0; k < getNumberOfSpectra(); ++k)
    {
      m_dataTable->item(k, startXColumn)->setText(start);
      m_dataTable->item(k, endXColumn)->setText(end);
    }
  }
  else
  {
    m_dataTable->item(i, startXColumn)->setText(start);
    m_dataTable->item(i, endXColumn)->setText(end);
  }
}

/// Get the fitting range for a i-th data set.
/// @param i :: Index of a dataset.
std::pair<double,double> DataController::getFittingRange(int i) const
{
  double startX = m_dataTable->item(i, startXColumn)->text().toDouble();
  double endX = m_dataTable->item(i, endXColumn)->text().toDouble();
  return std::make_pair(startX,endX);
}

/// Inform the others that a dataset was updated.
void DataController::updateDataset(int row, int)
{
  emit dataSetUpdated(row);
}

void MultiDatasetFit::loadSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  m_fitOptionsBrowser->loadSettings( settings );
}

void MultiDatasetFit::saveSettings() const
{
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  m_fitOptionsBrowser->saveSettings( settings );
}


/*==========================================================================================*/
} // CustomInterfaces
} // MantidQt
