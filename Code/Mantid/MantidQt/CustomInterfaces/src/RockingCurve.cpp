//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/RockingCurve.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <QFileInfo>

namespace MantidQt
{
namespace CustomInterfaces
{

//Register the class with the factory
//DECLARE_SUBWINDOW(RockingCurve);

using namespace Mantid::Kernel;
using namespace Mantid::API;

/// Constructor
RockingCurve::RockingCurve(QWidget *parent) : UserSubWindow(parent)
{
}

RockingCurve::~RockingCurve()
{
  // Clean up any hidden workspaces created
  cleanupWorkspaces();
}

/// Set up the dialog layout
void RockingCurve::initLayout()
{
  m_uiForm.setupUi(this);

  connect( m_uiForm.mWRunFiles, SIGNAL(filesFound()), SLOT(loadFile()), Qt::QueuedConnection );
  connect( this, SIGNAL(logsAvailable(const Mantid::API::MatrixWorkspace_const_sptr &)),
           SLOT(fillPlotVarCombobox(const Mantid::API::MatrixWorkspace_const_sptr &)) );

  //connect( m_uiForm.startButton, SIGNAL(clicked()), SLOT(generatePlot()) );
  connect( m_uiForm.startButton, SIGNAL(clicked()), SLOT(runRockingCurveAlg()) );
  connect( m_uiForm.closeButton, SIGNAL(clicked()), this->parent(), SLOT(close()) );
}

void RockingCurve::cleanupWorkspaces()
{
  if ( ! m_inputWSName.empty() )
  {
    AnalysisDataService::Instance().remove( m_inputWSName );
    AnalysisDataService::Instance().remove( m_inputWSName + "_monitors" );
    m_inputWSName.clear();
    AnalysisDataService::Instance().remove( m_plotWSName );
    m_plotWSName.clear();
  }
  // Disable start button
  m_uiForm.startButton->setEnabled(false);
  // Disconnect anything listening to the comboboxes
  m_uiForm.plotVariable->disconnect(SIGNAL(currentIndexChanged(const QString &)));
  m_uiForm.normalization->disconnect(SIGNAL(currentIndexChanged(const QString &)));
}

void RockingCurve::loadFile()
{
  // Remove any previously-loaded workspaces
  cleanupWorkspaces();

  // TODO: Run entirely asynchronously (see AlgorithmRunner)
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadEventNexus");
  const QString filename = m_uiForm.mWRunFiles->getFirstFilename();
  alg->setPropertyValue("Filename", filename.toStdString());
  m_inputWSName = "__" + QFileInfo(filename).baseName().toStdString();
  alg->setPropertyValue("OutputWorkspace", m_inputWSName);
  alg->setProperty("LoadMonitors", true);
  if ( alg->execute() )  // executeAsync???
  {
    const auto& ADS = AnalysisDataService::Instance();
    MatrixWorkspace_const_sptr outWS = ADS.retrieveWS<MatrixWorkspace>(m_inputWSName);
    emit logsAvailable( outWS );
    // Add the monitors to the normalization combobox
    MatrixWorkspace_const_sptr monWS = ADS.retrieveWS<MatrixWorkspace>(m_inputWSName+"_monitors");
    fillNormalizationCombobox( monWS );
  }
  else
  {
    QMessageBox::warning(this,"File loading failed","Is this an event nexus file?");
  }
}

void RockingCurve::fillPlotVarCombobox( const MatrixWorkspace_const_sptr & ws )
{
  // Hold the name of the scan index log in a common place
  const std::string scan_index("scan_index");
  // Clear the combobox and immediately re-insert 'scan_index' (so it's the first entry)
  m_uiForm.plotVariable->clear();
  m_uiForm.plotVariable->addItem( QString::fromStdString(scan_index) );

  // First check that the provided workspace has the scan_index - complain if it doesn't
  try {
    auto scan_index_prop = ws->run().getTimeSeriesProperty<int>(scan_index);
    if ( scan_index_prop->realSize() < 2 )
    {
      // TODO: This might be mistakenly triggered for live datasets.
      QMessageBox::warning(this,"scan_index log empty","This data does not appear to be an alignment scan");
      return;
    }
  } catch ( std::exception& ) {
  // Old way: ws->run().hasProperty(scan_index)
    QMessageBox::warning(this,"scan_index log not found","Is this an ADARA-style dataset?");
    return;
  }

  // This is unfortunately more or less a copy of SumEventsByLogValue::getNumberSeriesLogs
  // but I want to populate the box before running the algorithm
  const auto & logs = ws->run().getLogData();
  for ( auto log = logs.begin(); log != logs.end(); ++log )
  {
    const std::string logName = (*log)->name();
    // Don't add scan_index - that's already there
    if ( logName == scan_index ) continue;
    // Try to cast to an ITimeSeriesProperty
    auto tsp = dynamic_cast<const ITimeSeriesProperty*>(*log);
    // Move on to the next one if this is not a TSP
    if ( tsp == NULL ) continue;
    // Don't keep ones with only one entry
    if ( tsp->realSize() < 2 ) continue;
    // Now make sure it's either an int or double tsp, and if so add log to the list
    if ( dynamic_cast<TimeSeriesProperty<double>* >(*log) || dynamic_cast<TimeSeriesProperty<int>* >(*log))
    {
      m_uiForm.plotVariable->addItem( QString::fromStdString( logName ) );
    }
  }

  // Now that this has been populated, allow the user to select from it
  m_uiForm.plotVariable->setEnabled(true);
  // Now's the time to enable the start button as well
  m_uiForm.startButton->setEnabled(true);
}

void RockingCurve::fillNormalizationCombobox( const Mantid::API::MatrixWorkspace_const_sptr & ws )
{
  // If there are more than 3 entries in the combobox (nothing, time, proton_charge) then
  // remove any stale ones
  while ( m_uiForm.normalization->count() > 3 )
  {
    m_uiForm.normalization->removeItem(m_uiForm.normalization->count()-1);
  }

  for ( std::size_t i = 0; i < ws->getNumberHistograms(); ++i )
  {
    const std::string monitorName = ws->getDetector(i)->getName();
    m_uiForm.normalization->addItem( QString::fromStdString( monitorName ) );
  }
}

void RockingCurve::runRockingCurveAlg()
{
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("RockingCurve");
  alg->setPropertyValue("InputWorkspace", m_inputWSName);
  // The table should not be hidden, so leave off the prefix
  m_tableWSName = m_inputWSName.substr(2) + "_RockingCurve";
  alg->setPropertyValue("OutputWorkspace", m_tableWSName);
  // TODO: Get other properties when implemented in the form
  alg->execute();

  // Now that the algorithm's been run, connect up the signal to change the plot variable
  connect( m_uiForm.plotVariable, SIGNAL(currentIndexChanged(const QString &)),
           SLOT(generateCurve(const QString &)) );
  // and the one if the normalisation's been changed
  connect( m_uiForm.normalization, SIGNAL(currentIndexChanged(const QString &)),
           SLOT(updateForNormalizationChange()) );
  // Create the plot for the first time
  generateCurve( m_uiForm.plotVariable->currentText() );
}

void RockingCurve::updateForNormalizationChange()
{
  generateCurve( m_uiForm.plotVariable->currentText() );
}

void RockingCurve::generateCurve( const QString & var )
{
  // Create a matrix workspace out of the variable that's asked for
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ConvertTableToMatrixWorkspace");
  alg->setLogging(false); // Don't log this algorithm
  alg->setPropertyValue("InputWorkspace", m_tableWSName);
  m_plotWSName = "__plot_" + m_tableWSName;
  alg->setPropertyValue("OutputWorkspace", m_plotWSName);
  alg->setPropertyValue("ColumnX", var.toStdString() );
  alg->setPropertyValue("ColumnY", "Counts" );
  alg->execute();

  // Now create one for the normalisation, if required
  if ( m_uiForm.normalization->currentIndex() !=  0 )
  {
    IAlgorithm_sptr norm = AlgorithmManager::Instance().create("ConvertTableToMatrixWorkspace");
    norm->setChild(true);
    norm->setLogging(false); // Don't log this algorithm
    norm->setPropertyValue("InputWorkspace", m_tableWSName);
    norm->setPropertyValue("OutputWorkspace", "dummyName");
    norm->setPropertyValue("ColumnX", var.toStdString() );
    // TODO: Protect against column being missing (e.g. if monitor not found in data)
    norm->setPropertyValue("ColumnY", m_uiForm.normalization->currentText().toStdString() );
    norm->execute();

    MatrixWorkspace_sptr top = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_plotWSName);
    MatrixWorkspace_sptr bottom = norm->getProperty("OutputWorkspace");
    top /= bottom;
  }

  plotCurve();
}

void RockingCurve::plotCurve()
{
  // Get the name of the dataset to produce the plot title
  std::string title = m_inputWSName.substr(2);
  // qtiplot may unhelpfully change '_' to '-' so I need to as well
  std::replace(title.begin(), title.end(), '_', '-');

  // Figure out the axis titles
  const std::string xAxisTitle = m_uiForm.plotVariable->currentText().toStdString();
  std::string yAxisTitle = "Counts";
  const std::string normalization = m_uiForm.normalization->currentText().toStdString();
  if ( normalization == "nothing") /* Do nothing */;
  else if ( normalization == "time" ) yAxisTitle += " / second";
  else if ( normalization == "proton_charge" ) yAxisTitle += " / picocoulomb";
  else yAxisTitle += " / " + normalization;

  // Has to be done via python
  std::string pyCode = "g = graph('" + title + "')\n"
                       "if g is None:\n"
                       "    g = plotSpectrum('" + m_plotWSName + "',0,type=Layer.Scatter)\n"
                       "    l = g.activeLayer()\n"
                       "    l.legend().hide()\n"
                       "    l.removeTitle()\n"
                       "    setWindowName(g,'" + title + "')\n"
                       "    g.setWindowLabel('Rocking Curve')\n"
                       "l = g.activeLayer()\n"
                       "l.setAxisTitle(Layer.Bottom,'" + xAxisTitle + "')\n"
                       "l.setAxisTitle(Layer.Left,'" + yAxisTitle + "')";

  runPythonCode( QString::fromStdString(pyCode) );
}

} // namespace CustomInterfaces
} // namespace MantidQt
