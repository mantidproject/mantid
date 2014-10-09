#include "MantidQtCustomInterfaces/MultiDatasetFit.h"
#include "MantidQtMantidWidgets/FunctionBrowser.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"

#include "qtpropertybrowser.h"

#include <QDialog>
#include <QHeaderView>
#include <QMessageBox>

#include <boost/make_shared.hpp>
#include <qwt_plot_curve.h>

#include <vector>
#include <algorithm>

namespace{
  const int wsColumn = 0;
  const int wsIndexColumn = 1;
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
  if ( !indexInput.isEmpty() )
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
 * Contains graphics for a single data set.
 */
class DatasetPlotData
{
public:
  DatasetPlotData(const QString& wsName, int wsIndex, const QString& outputWSName);
  ~DatasetPlotData();
  void show(QwtPlot *plot);
  void hide();
private:
  void setData(const Mantid::API::MatrixWorkspace *ws, int wsIndex, const Mantid::API::MatrixWorkspace *outputWS);
  QwtPlotCurve *m_dataCurve;
  QwtPlotCurve *m_calcCurve;
  QwtPlotCurve *m_diffCurve;
};

/**
 * Creator.
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
    outputWS = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( outputWSName.toStdString() );
    if ( !outputWS )
    {
      QString mess = QString("Workspace %1 either doesn't exist or isn't a MatrixWorkspace").arg(outputWSName);
      throw std::runtime_error( mess.toStdString() );
    }
  }

  // create the curves
  setData( ws.get(), wsIndex, outputWS.get() );

}

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

void DatasetPlotData::setData(const Mantid::API::MatrixWorkspace *ws, int wsIndex, const Mantid::API::MatrixWorkspace *outputWS)
{
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

  if ( outputWS && outputWS->getNumberHistograms() >= 3 )
  {
    m_calcCurve = new QwtPlotCurve("calc");
    m_calcCurve->setData( xValues.data(), outputWS->readY(1).data(), static_cast<int>(xValues.size()) );
    QPen penCalc("red");
    m_calcCurve->setPen(penCalc);
    m_diffCurve = new QwtPlotCurve("diff");
    m_diffCurve->setData( xValues.data(), outputWS->readY(2).data(), static_cast<int>(xValues.size()) );
    QPen penDiff("green");
    m_diffCurve->setPen(penDiff);
  }
}

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

/*==========================================================================================*/
/*                                PlotController                                            */
/*==========================================================================================*/

PlotController::PlotController(MultiDatasetFit *parent,QwtPlot *plot, QTableWidget *table, QComboBox *plotSelector, QPushButton *prev, QPushButton *next):
  QObject(parent),m_plot(plot),m_table(table),m_plotSelector(plotSelector),m_prevPlot(prev),m_nextPlot(next),m_currentIndex(-1)
{
  connect(parent,SIGNAL(dataTableUpdated()),this,SLOT(tableUpdated()));
  connect(prev,SIGNAL(clicked()),this,SLOT(prevPlot()));
  connect(next,SIGNAL(clicked()),this,SLOT(nextPlot()));
  connect(plotSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(plotDataSet(int)));
}

PlotController::~PlotController()
{
  m_plotData.clear();
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

/**
 * Plot a data set.
 * @param index :: Index (row) of the data set in the table.
 */
void PlotController::plotDataSet(int index)
{
  if ( index < 0 || index >= m_table->rowCount() ) return;
  if ( !m_plotData.contains(index) )
  {
    QString wsName = m_table->item( index, wsColumn )->text();
    int wsIndex = m_table->item( index, wsIndexColumn )->text().toInt();
    QString outputWorkspaceName = owner()->getOutputWorkspaceName();
    if ( !outputWorkspaceName.isEmpty() )
    {
      outputWorkspaceName += QString("_%1").arg(index);
    }
    auto value = boost::make_shared<DatasetPlotData>( wsName, wsIndex, outputWorkspaceName );
    m_plotData.insert(index, value );
  }
  if ( m_currentIndex > -1 ) 
  {
    m_plotData[m_currentIndex]->hide();
  }
  m_plotData[index]->show( m_plot );
  m_plot->replot();
  m_currentIndex = index;
}

void PlotController::clear()
{
  m_plotData.clear();
}

void PlotController::update()
{
  plotDataSet( m_currentIndex );
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

  connect(m_uiForm.btnAddWorkspace,SIGNAL(clicked()),this,SLOT(addWorkspace()));
  connect(m_uiForm.btnRemove,SIGNAL(clicked()),this,SLOT(removeSelectedSpectra()));
  connect(m_uiForm.dataTable,SIGNAL(itemSelectionChanged()), this,SLOT(workspaceSelectionChanged()));
  connect(m_uiForm.btnFit,SIGNAL(clicked()),this,SLOT(fit()));
  connect(this,SIGNAL(dataTableUpdated()),this,SLOT(reset()));

  m_plotController = new PlotController(this,
                                        m_uiForm.plot,
                                        m_uiForm.dataTable,
                                        m_uiForm.cbPlotSelector,
                                        m_uiForm.btnPrev,
                                        m_uiForm.btnNext);

  m_functionBrowser = new MantidQt::MantidWidgets::FunctionBrowser(NULL, true);
  m_uiForm.browserLayout->addWidget( m_functionBrowser );
  connect(m_functionBrowser,SIGNAL(localParameterButtonClicked(const QString&)),this,SLOT(editLocalParameterValues(const QString&)));
  connect(m_functionBrowser,SIGNAL(functionStructureChanged()),this,SLOT(reset()));
}

/**
 * Show a dialog to select a workspace.
 */
void MultiDatasetFit::addWorkspace()
{
  AddWorkspaceDialog dialog(this);
  if ( dialog.exec() == QDialog::Accepted )
  {
    QString wsName = dialog.workspaceName().stripWhiteSpace();
    if ( Mantid::API::AnalysisDataService::Instance().doesExist( wsName.toStdString()) )
    {
      auto indices = dialog.workspaceIndices();
      for(auto i = indices.begin(); i != indices.end(); ++i)
      {
        addWorkspaceSpectrum( wsName, *i );
      }
      emit dataTableUpdated();
    }
    else
    {
      QMessageBox::warning(this,"MantidPlot - Warning",QString("Workspace \"%1\" doesn't exist.").arg(wsName));
    }
  }
}

/**
 * Add a spectrum from a workspace to the table.
 * @param wsName :: Name of a workspace.
 * @param wsIndex :: Index of a spectrum in the workspace (workspace index).
 */
void MultiDatasetFit::addWorkspaceSpectrum(const QString &wsName, int wsIndex)
{
  int row = m_uiForm.dataTable->rowCount();
  m_uiForm.dataTable->insertRow(row);

  auto cell = new QTableWidgetItem( wsName );
  m_uiForm.dataTable->setItem( row, wsColumn, cell );
  cell = new QTableWidgetItem( QString::number(wsIndex) );
  m_uiForm.dataTable->setItem( row, wsIndexColumn, cell );
}

/**
 * Slot. Called when selection in the data table changes.
 */
void MultiDatasetFit::workspaceSelectionChanged()
{
  auto selection = m_uiForm.dataTable->selectionModel();
  bool enableRemoveButton = selection->hasSelection();
  if ( enableRemoveButton )
  {
    enableRemoveButton = selection->selectedRows().size() > 0;
  }

  m_uiForm.btnRemove->setEnabled( enableRemoveButton );
}

/**
 * Slot. Called when "Remove" button is pressed.
 */
void MultiDatasetFit::removeSelectedSpectra()
{
  auto ranges = m_uiForm.dataTable->selectedRanges();
  if ( ranges.isEmpty() ) return;
  std::vector<int> rows;
  for(auto range = ranges.begin(); range != ranges.end(); ++range)
  {
    for(int row = range->topRow(); row <= range->bottomRow(); ++row)
    {
      rows.push_back( row );
    }
  }
  std::sort( rows.begin(), rows.end() );
  for(auto row = rows.rbegin(); row != rows.rend(); ++row)
  {
    m_uiForm.dataTable->removeRow( *row );
  }
  emit dataTableUpdated();
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
  try
  {
    auto fun = createFunction();
    auto fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();
    fit->setProperty("Function", fun );
    fit->setPropertyValue("InputWorkspace", getWorkspaceName(0));
    fit->setProperty("WorkspaceIndex", getWorkspaceIndex(0));

    m_outputWorkspaceName = "out";
    fit->setPropertyValue("Output",m_outputWorkspaceName);
    m_outputWorkspaceName += "_Workspace";

    int n = getNumberOfSpectra();
    for(int ispec = 1; ispec < n; ++ispec)
    {
      std::string suffix = boost::lexical_cast<std::string>(ispec);
      fit->setPropertyValue( "InputWorkspace_" + suffix, getWorkspaceName(ispec) );
      fit->setProperty( "WorkspaceIndex_" + suffix, getWorkspaceIndex(ispec) );
    }

    fit->execute();

    m_plotController->clear();
    m_plotController->update();
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
  return m_uiForm.dataTable->item(i, wsColumn)->text().toStdString();
}

/**
 * Get the workspace index of the i-th spectrum.
 * @param i :: Index of a spectrum in the data table.
 */
int MultiDatasetFit::getWorkspaceIndex(int i) const
{
  return m_uiForm.dataTable->item(i, wsIndexColumn)->text().toInt();
}

/**
 * Get the number of spectra to fit to.
 */
int MultiDatasetFit::getNumberOfSpectra() const
{
  return m_uiForm.dataTable->rowCount();
}

/**
 * Start an editor to display and edit individual local parameter values.
 * @param parName :: Fully qualified name for a local parameter (Global unchecked).
 */
void MultiDatasetFit::editLocalParameterValues(const QString& parName)
{
  EditLocalParameterDialog dialog(this,parName);
  if ( dialog.exec() == QDialog::Accepted )
  {
    throw std::runtime_error("Edit local parameters.");
  }
}

/**
 * Get value of a local parameter
 * @param parName :: Name of a parameter.
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
  QString functionIndex;
  QString parameterName = parName;
  int j = parName.lastIndexOf('.');
  if ( j > 0 )
  {
    ++j;
    functionIndex = parName.mid(0,j);
    parameterName = parName.mid(j);
  }
  double value = m_functionBrowser->getParameter(functionIndex,parameterName);
  QVector<double> values( static_cast<int>(getNumberOfSpectra()), value );
  m_localParameterValues[parName] = values;
}

void MultiDatasetFit::reset()
{
  m_localParameterValues.clear();
}

/*==========================================================================================*/
} // CustomInterfaces
} // MantidQt
