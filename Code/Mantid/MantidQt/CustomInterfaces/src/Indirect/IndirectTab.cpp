#include "MantidQtCustomInterfaces/Indirect/IndirectTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Logger.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

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
   * Sets the edge bounds of plot to prevent the user inputting invalid values
   * Also sets limits for range selector movement
   *
   * @param rs :: Pointer to the RangeSelector
   * @param min :: The lower bound property in the property browser
   * @param max :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void IndirectTab::setPlotPropertyRange(RangeSelector * rs, QtProperty* min, QtProperty* max,
      const QPair<double, double> & bounds)
  {
    m_dblManager->setMinimum(min, bounds.first);
    m_dblManager->setMaximum(min, bounds.second);
    m_dblManager->setMinimum(max, bounds.first);
    m_dblManager->setMaximum(max, bounds.second);
    rs->setRange(bounds.first, bounds.second);
  }

  /**
   * Set the position of the range selectors on the mini plot
   *
   * @param rs :: Pointer to the RangeSelector
   * @param lower :: The lower bound property in the property browser
   * @param upper :: The upper bound property in the property browser
   * @param bounds :: The upper and lower bounds to be set
   */
  void IndirectTab::setRangeSelector(RangeSelector * rs, QtProperty* lower, QtProperty* upper,
      const QPair<double, double> & bounds)
  {
    m_dblManager->setValue(lower, bounds.first);
    m_dblManager->setValue(upper, bounds.second);
    rs->setMinimum(bounds.first);
    rs->setMaximum(bounds.second);
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
