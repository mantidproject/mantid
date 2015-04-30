#include "MantidQtCustomInterfaces/MultiDatasetFit/MultiDatasetFit.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFDataController.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFPlotController.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFEditLocalParameterDialog.h"

#include "MantidAPI/AlgorithmManager.h"

#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtMantidWidgets/FitOptionsBrowser.h"
#include "MantidQtMantidWidgets/FunctionBrowser.h"

#include <QMessageBox>
#include <QToolBar>
#include <QSettings>

namespace{
  // tool options pages
  const int zoomToolPage  = 0;
  const int panToolPage   = 1;
  const int rangeToolPage = 2;
}

namespace MantidQt
{
namespace CustomInterfaces
{

//Register the class with the factory
DECLARE_SUBWINDOW(MultiDatasetFit)

/// Constructor
/// @param parent :: The parent widget
MultiDatasetFit::MultiDatasetFit(QWidget *parent)
:UserSubWindow(parent)
{
}

MultiDatasetFit::~MultiDatasetFit()
{
  saveSettings();
  m_plotController->clear();
}

/// Initilize the layout. 
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

  m_dataController = new MDF::DataController(this, m_uiForm.dataTable);
  connect(m_dataController,SIGNAL(hasSelection(bool)),  m_uiForm.btnRemove, SLOT(setEnabled(bool)));
  connect(m_uiForm.btnAddWorkspace,SIGNAL(clicked()),m_dataController,SLOT(addWorkspace()));
  connect(m_uiForm.btnRemove,SIGNAL(clicked()),m_dataController,SLOT(removeSelectedSpectra()));
  connect(m_uiForm.cb_applyRangeToAll,SIGNAL(toggled(bool)),m_dataController,SLOT(setFittingRangeGlobal(bool)));

  m_plotController = new MDF::PlotController(this,
                                        m_uiForm.plot,
                                        m_uiForm.dataTable,
                                        m_uiForm.cbPlotSelector,
                                        m_uiForm.btnPrev,
                                        m_uiForm.btnNext);
  connect(m_dataController,SIGNAL(dataTableUpdated()),m_plotController,SLOT(tableUpdated()));
  connect(m_dataController,SIGNAL(dataSetUpdated(int)),m_plotController,SLOT(updateRange(int)));
  connect(m_plotController,SIGNAL(fittingRangeChanged(int, double, double)),m_dataController,SLOT(setFittingRange(int, double, double)));

  QSplitter* splitter = new QSplitter(Qt::Vertical,this);

  m_functionBrowser = new MantidQt::MantidWidgets::FunctionBrowser(NULL, true);
  splitter->addWidget( m_functionBrowser );
  connect(m_functionBrowser,SIGNAL(localParameterButtonClicked(const QString&)),this,SLOT(editLocalParameterValues(const QString&)));
  connect(m_functionBrowser,SIGNAL(functionStructureChanged()),this,SLOT(reset()));
  connect(m_plotController,SIGNAL(currentIndexChanged(int)),m_functionBrowser,SLOT(setCurrentDataset(int)));
  connect(m_dataController,SIGNAL(spectraRemoved(QList<int>)),m_functionBrowser,SLOT(removeDatasets(QList<int>)));
  connect(m_dataController,SIGNAL(spectraAdded(int)),m_functionBrowser,SLOT(addDatasets(int)));

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

/// Create the tool bar for the plot widget.
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

/// Create a multi-domain function to fit all the spectra in the data table.
boost::shared_ptr<Mantid::API::IFunction> MultiDatasetFit::createFunction() const
{
  return m_functionBrowser->getGlobalFunction();
}

/// Run the fitting algorithm.
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

/// Get the workspace name of the i-th spectrum.
/// @param i :: Index of a spectrum in the data table.
std::string MultiDatasetFit::getWorkspaceName(int i) const
{
  return m_dataController->getWorkspaceName(i);
}

/// Get the workspace index of the i-th spectrum.
/// @param i :: Index of a spectrum in the data table.
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

/// Get the number of spectra to fit to.
int MultiDatasetFit::getNumberOfSpectra() const
{
  return m_dataController->getNumberOfSpectra();
}

/// Start an editor to display and edit individual local parameter values.
/// @param parName :: Fully qualified name for a local parameter (Global unchecked).
void MultiDatasetFit::editLocalParameterValues(const QString& parName)
{
  MDF::EditLocalParameterDialog dialog(this,parName);
  if ( dialog.exec() == QDialog::Accepted )
  {
    auto values = dialog.getValues();
    auto fixes = dialog.getFixes();
    assert( values.size() == getNumberOfSpectra() );
    for(int i = 0; i < values.size(); ++i)
    {
      setLocalParameterValue(parName, i, values[i]);
      setLocalParameterFixed(parName, i, fixes[i]);
    }
  }
}

/// Slot called on completion of the Fit algorithm.
/// @param error :: Set to true if Fit finishes with an error.
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

/// Update the interface to have the same parameter values as in a function.
/// @param fun :: A function from which to take the parameters.
void MultiDatasetFit::updateParameters(const Mantid::API::IFunction& fun)
{
  m_functionBrowser->updateMultiDatasetParameters( fun );
}

/// Show a message in the info bar at the bottom of the interface.
void MultiDatasetFit::showInfo(const QString& text)
{
  m_uiForm.infoBar->setText(text);
}

/// Intersept mouse-enter events to display context-specific info
/// in the "status bar".
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

/// Show info about the function browser.
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

/// Show info about the Fit options browser.
void MultiDatasetFit::showFitOptionsBrowserInfo()
{
  showInfo( "Set Fit properties." );
}

/// Show info / tips on the plot widget.
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

/// Show info / tips on the dataset table.
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

/// Check that the data sets in the table are valid and remove invalid ones.
void MultiDatasetFit::checkSpectra()
{
  m_dataController->checkSpectra();
}

/// Enable the zoom tool.
void MultiDatasetFit::enableZoom()
{
  m_plotController->enableZoom();
  m_uiForm.toolOptions->setCurrentIndex(zoomToolPage);
}

/// Enable the panning tool.
void MultiDatasetFit::enablePan()
{
  m_plotController->enablePan();
  m_uiForm.toolOptions->setCurrentIndex(panToolPage);
}

/// Enable the fitting range selection tool.
void MultiDatasetFit::enableRange()
{
  m_plotController->enableRange();
  m_uiForm.toolOptions->setCurrentIndex(rangeToolPage);
}

/// Set value of a local parameter
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
/// @param value :: New value for the parameter.
void MultiDatasetFit::setLocalParameterValue(const QString& parName, int i, double value)
{
  m_functionBrowser->setLocalParameterValue(parName, i, value);
}

/// Get value of a local parameter
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
double MultiDatasetFit::getLocalParameterValue(const QString& parName, int i) const
{
  return m_functionBrowser->getLocalParameterValue(parName, i);
}

/// Reset the caches. Prepare to fill them in lazily.
void MultiDatasetFit::reset()
{
  m_functionBrowser->resetLocalParameters();
  m_functionBrowser->setNumberOfDatasets( getNumberOfSpectra() );
}

/// Check if a local parameter is fixed
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
bool MultiDatasetFit::isLocalParameterFixed(const QString& parName, int i) const
{
  return m_functionBrowser->isLocalParameterFixed(parName, i);
}

/// Fix/unfix local parameter
/// @param parName : Name of a local parameter.
/// @param i :: Index of the dataset (spectrum).
/// @param fixed :: Should the parameter be fixed (true) or unfixed (false).
void MultiDatasetFit::setLocalParameterFixed(const QString& parName, int i, bool fixed)
{
  m_functionBrowser->setLocalParameterFixed(parName, i, fixed);
}

/// Load settings
void MultiDatasetFit::loadSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  m_fitOptionsBrowser->loadSettings( settings );
}

/// Save settings
void MultiDatasetFit::saveSettings() const
{
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  m_fitOptionsBrowser->saveSettings( settings );
}

} // CustomInterfaces
} // MantidQt
