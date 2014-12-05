#include "MantidQtCustomInterfaces/IndirectTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Logger.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/InterfaceManager.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace
{
  Mantid::Kernel::Logger g_log("IndirectTab");
}

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectTab::IndirectTab(QObject* parent) : QObject(parent),
      m_plots(), m_curves(), m_rangeSelectors(),
      m_properties(),
      m_dblManager(new QtDoublePropertyManager()), m_blnManager(new QtBoolPropertyManager()), m_grpManager(new QtGroupPropertyManager()),
      m_dblEdFac(new DoubleEditorFactory()),
      m_pythonRunner(),
      m_tabStartTime(DateAndTime::getCurrentTime()), m_tabEndTime(DateAndTime::maximum())
  {
    m_parentWidget = dynamic_cast<QWidget *>(parent);

    m_batchAlgoRunner = new MantidQt::API::BatchAlgorithmRunner(m_parentWidget);
    m_valInt = new QIntValidator(m_parentWidget);
    m_valDbl = new QDoubleValidator(m_parentWidget);
    m_valPosDbl = new QDoubleValidator(m_parentWidget);

    const double tolerance = 0.00001;
    m_valPosDbl->setBottom(tolerance);

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmFinished(bool)));
    connect(&m_pythonRunner, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectTab::~IndirectTab()
  {
  }

  void IndirectTab::runTab()
  {
    if(validate())
    {
      m_tabStartTime = DateAndTime::getCurrentTime();
      run();
    }
    else
    {
      g_log.warning("Failed to validate indirect tab input!");
    }
  }

  void IndirectTab::setupTab()
  {
    setup();
  }

  bool IndirectTab::validateTab()
  {
    return validate();
  }

  /**
   * Handles generating a Python script for the algorithms run on the current tab.
   */
  void IndirectTab::exportPythonScript()
  {
    g_log.information() << "Python export for workspace: " << m_pythonExportWsName <<
      ", between " << m_tabStartTime << " and " << m_tabEndTime << std::endl;

    // Take the search times to be a second either side of the actual times, just in case
    DateAndTime startSearchTime = m_tabStartTime - 1.0;
    DateAndTime endSearchTime = m_tabEndTime + 1.0;

    // Don't let the user change the time range
    QStringList enabled;
    enabled << "Filename" << "InputWorkspace" << "UnrollAll" << "SpecifyAlgorithmVersions";

    // Give some indication to the user that they will have to specify the workspace
    if(m_pythonExportWsName.empty())
      g_log.warning("This tab has not specified a result workspace name.");

    // Set default properties
    QHash<QString, QString> props;
    props["Filename"] = "IndirectInterfacePythonExport.py";
    props["InputWorkspace"] = QString::fromStdString(m_pythonExportWsName);
    props["SpecifyAlgorithmVersions"] = "Specify All";
    props["UnrollAll"] = "1";
    props["StartTimestamp"] = QString::fromStdString(startSearchTime.toISO8601String());
    props["EndTimestamp"] = QString::fromStdString(endSearchTime.toISO8601String());

    // Create an algorithm dialog for the script export algorithm
    MantidQt::API::InterfaceManager interfaceManager;
    MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialogFromName("GeneratePythonScript", -1,
        NULL, false, props, "", enabled);

    // Show the dialog
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
  }

  /**
   * Run the load algorithm with the supplied filename and spectrum range
   *
   * @param filename :: The name of the file to load
   * @param outputName :: The name of the output workspace
   * @param specMin :: Lower spectra bound
   * @param specMax :: Upper spectra bound
   * @return If the algorithm was successful
   */
  bool IndirectTab::loadFile(const QString& filename, const QString& outputName,
      const int specMin, const int specMax)
  {
    Algorithm_sptr load = AlgorithmManager::Instance().createUnmanaged("Load", -1);
    load->initialize();

    load->setProperty("Filename", filename.toStdString());
    load->setProperty("OutputWorkspace", outputName.toStdString());

    if(specMin != -1)
      load->setPropertyValue("SpectrumMin", boost::lexical_cast<std::string>(specMin));

    if(specMax != -1)
      load->setPropertyValue("SpectrumMax", boost::lexical_cast<std::string>(specMax));

    load->execute();

    //If reloading fails we're out of options
    return load->isExecuted();
  }

  /**
   * Gets the range of the curve plotted in the mini plot
   *
   * @param curveID :: The string index of the curve in the m_curves map
   * @return A pair containing the maximum and minimum points of the curve
   */
  std::pair<double, double> IndirectTab::getCurveRange(const QString& curveID)
  {
    size_t npts = m_curves[curveID]->data().size();

    if( npts < 2 )
      throw std::invalid_argument("Too few points on data curve to determine range.");

    return std::make_pair(m_curves[curveID]->data().x(0), m_curves[curveID]->data().x(npts-1));
  }

  /**
   * Set the range of an axis on a miniplot
   *
   * @param plotID :: Index of plot in m_plots map
   * @param axis :: ID of axis to set range of
   * @param range :: Pair of double values specifying range
   */
  void IndirectTab::setAxisRange(const QString& plotID, QwtPlot::Axis axis,
      std::pair<double, double> range)
  {
    m_plots[plotID]->setAxisScale(axis, range.first, range.second);
  }

  /**
   * Sets the X axis of a plot to match the range of x values on a curve
   *
   * @param plotID :: Index of plot in m_plots map
   * @param curveID :: Index of curve in m_curves map
   */
  void IndirectTab::setXAxisToCurve(const QString& plotID, const QString& curveID)
  {
    auto range = getCurveRange(curveID);
    setAxisRange(plotID, QwtPlot::xBottom, range);
  }

  /**
   * Plot a workspace to the miniplot given a workspace name and
   * a specturm index.
   *
   * This method uses the analysis data service to retrieve the workspace.
   *
   * @param workspace :: The name of the workspace
   * @param index :: The spectrum index of the workspace
   * @param plotID :: String index of the plot in the m_plots map
   * @param curveID :: String index of the curve in the m_curves map, defaults to plot ID
   */
  void IndirectTab::plotMiniPlot(const QString& workspace, size_t index,
      const QString& plotID, const QString& curveID)
  {
    auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(workspace.toStdString());
    plotMiniPlot(ws, index, plotID, curveID);
  }

  /**
   * Replot a given mini plot
   *
   * @param plotID :: ID of plot in m_plots map
   */
  void IndirectTab::replot(const QString& plotID)
  {
    m_plots[plotID]->replot();
  }

  /**
   * Removes a curve from a mini plot and deletes it.
   *
   * @param curveID :: ID of plot in m_plots map
   */
  void IndirectTab::removeCurve(const QString& curveID)
  {
    if(m_curves[curveID] == NULL)
      return;

    m_curves[curveID]->attach(0);
    delete m_curves[curveID];
    m_curves[curveID] = NULL;
  }

  /**
   * Plot a workspace to the miniplot given a workspace pointer and
   * a specturm index.
   *
   * @param workspace :: Pointer to the workspace
   * @param wsIndex :: The spectrum index of the workspace
   * @param plotID :: String index of the plot in the m_plots map
   * @param curveID :: String index of the curve in the m_curves map, defaults to plot ID
   */
  void IndirectTab::plotMiniPlot(const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex,
      const QString& plotID, const QString& curveID)
  {
    using Mantid::MantidVec;

    QString cID = curveID;
    if(cID == "")
      cID = plotID;

    //check if we can plot
    if( wsIndex >= workspace->getNumberHistograms() || workspace->readX(0).size() < 2 )
      return;

    const bool logScale(false), distribution(false);
    QwtWorkspaceSpectrumData wsData(*workspace, static_cast<int>(wsIndex), logScale, distribution);

    removeCurve(cID);

    size_t nhist = workspace->getNumberHistograms();
    if ( wsIndex >= nhist )
    {
      emit showMessageBox("Error: Workspace index out of range.");
    }
    else
    {
      m_curves[cID] = new QwtPlotCurve();
      m_curves[cID]->setData(wsData);
      m_curves[cID]->attach(m_plots[plotID]);

      m_plots[plotID]->replot();
    }
  }

  /**
   * Sets the edge bounds of plot to prevent the user inputting invalid values
   * Also sets limits for range selector movement
   *
   * @param rsID :: The string index of the range selector in the map m_rangeSelectors
   * @param min :: The lower bound property in the property browser
   * @param max :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void IndirectTab::setPlotRange(const QString& rsID, QtProperty* min, QtProperty* max,
      const std::pair<double, double>& bounds)
  {
    m_dblManager->setMinimum(min, bounds.first);
    m_dblManager->setMaximum(min, bounds.second);
    m_dblManager->setMinimum(max, bounds.first);
    m_dblManager->setMaximum(max, bounds.second);
    m_rangeSelectors[rsID]->setRange(bounds.first, bounds.second);
  }

  /**
   * Set the position of the guides on the mini plot
   *
   * @param rsID :: The string index of the range selector in the map m_rangeSelectors
   * @param lower :: The lower bound property in the property browser
   * @param upper :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void IndirectTab::setMiniPlotGuides(const QString& rsID, QtProperty* lower, QtProperty* upper,
      const std::pair<double, double>& bounds)
  {
    m_dblManager->setValue(lower, bounds.first);
    m_dblManager->setValue(upper, bounds.second);
    m_rangeSelectors[rsID]->setMinimum(bounds.first);
    m_rangeSelectors[rsID]->setMaximum(bounds.second);
  }

  /**
   * Runs an algorithm async
   *
   * @param algorithm :: The algorithm to be run
   */
  void IndirectTab::runAlgorithm(const Mantid::API::IAlgorithm_sptr algorithm)
  {
    algorithm->setRethrows(true);

    // There should never really be unexecuted algorithms in the queue, but it is worth warning in case of possible weirdness
    size_t batchQueueLength = m_batchAlgoRunner->queueLength();
    if(batchQueueLength > 0)
      g_log.warning() << "Batch queue already contains " << batchQueueLength << " algorithms!" << std::endl;

    m_batchAlgoRunner->addAlgorithm(algorithm);
    m_batchAlgoRunner->executeBatchAsync();
  }

  /**
   * Handles getting the results of an algorithm running async
   *
   * @param error :: True if execution failed, false otherwise
   */
  void IndirectTab::algorithmFinished(bool error)
  {
    m_tabEndTime = DateAndTime::getCurrentTime();

    if(error)
    {
      emit showMessageBox("Error running algorithm. \nSee results log for details.");
    }
  }

  /**
   * Run Python code and return anything printed to stdout.
   *
   * @param code Python code the execute
   * @param no_output Enable to ignore any output
   * @returns What was printed to stdout
   */
  QString IndirectTab::runPythonCode(QString code, bool no_output)
  {
    return m_pythonRunner.runPythonCode(code, no_output);
  }

} // namespace CustomInterfaces
} // namespace Mantid
