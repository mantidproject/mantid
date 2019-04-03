// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QMessageBox>

#include <boost/algorithm/string/find.hpp>
#include <boost/pointer_cast.hpp>

#include "MantidKernel/Strings.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
using namespace Poco::XML;

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using Mantid::Types::Core::DateAndTime;

namespace {
Mantid::Kernel::Logger g_log("IndirectTab");

std::string castToString(int value) {
  return boost::lexical_cast<std::string>(value);
}

template <typename Predicate>
void setPropertyIf(Algorithm_sptr algorithm, std::string const &propName,
                   std::string const &value, Predicate const &condition) {
  if (condition)
    algorithm->setPropertyValue(propName, value);
}

std::string findXMLAttribute(Document const *document, XMLString const &tag,
                             XMLString const &attribute) {
  auto const root = document->documentElement();
  for (auto *node = root->firstChild(); node != 0; node = node->nextSibling()) {
    auto const element = dynamic_cast<Poco::XML::Element *>(node);
    if (element && element->tagName() == tag)
      return std::string(element->getAttribute(attribute));
  }
  throw std::runtime_error("The tag " + tag + " was not found in the file.");
}

std::string getAttributeFromFile(std::string const &filepath,
                                 XMLString const &tag,
                                 XMLString const &attribute) {
  DOMParser parser;
  Document *document;
  std::string attributeValue;
  try {
    auto const xmlText = Strings::loadFile(filepath);
    document = parser.parseString(xmlText);
    if (document)
      attributeValue = findXMLAttribute(document, tag, attribute);
  } catch (Poco::Exception const &ex) {
    g_log.warning(ex.displayText() + ". Unable to parse File:" + filepath);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
  }
  return attributeValue;
}

QStringList convertToQStringList(std::string const &str,
                                 std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));

  QStringList list;
  for (auto const &subString : subStrings)
    list << QString::fromStdString(subString);
  return list;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {

IndirectTab::IndirectTab(QObject *parent)
    : QObject(parent), m_properties(),
      m_dblManager(new QtDoublePropertyManager()),
      m_blnManager(new QtBoolPropertyManager()),
      m_grpManager(new QtGroupPropertyManager()),
      m_dblEdFac(new DoubleEditorFactory()), m_pythonRunner(),
      m_plotErrorBars(false), m_tabStartTime(DateAndTime::getCurrentTime()),
      m_tabEndTime(DateAndTime::maximum()) {
  m_parentWidget = dynamic_cast<QWidget *>(parent);

  m_batchAlgoRunner = new MantidQt::API::BatchAlgorithmRunner(m_parentWidget);
  m_valInt = new QIntValidator(m_parentWidget);
  m_valDbl = new QDoubleValidator(m_parentWidget);
  m_valPosDbl = new QDoubleValidator(m_parentWidget);

  const double tolerance = 0.00001;
  m_valPosDbl->setBottom(tolerance);

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmFinished(bool)));
  connect(&m_pythonRunner, SIGNAL(runAsPythonScript(const QString &, bool)),
          this, SIGNAL(runAsPythonScript(const QString &, bool)));
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IndirectTab::~IndirectTab() {}

void IndirectTab::runTab() {
  if (validate()) {
    m_tabStartTime = DateAndTime::getCurrentTime();
    run();
  } else {
    g_log.warning("Failed to validate indirect tab input!");
  }
}

void IndirectTab::setupTab() { setup(); }

bool IndirectTab::validateTab() { return validate(); }

/**
 * Handles generating a Python script for the algorithms run on the current tab.
 */
void IndirectTab::exportPythonScript() {
  g_log.information() << "Python export for workspace: " << m_pythonExportWsName
                      << ", between " << m_tabStartTime << " and "
                      << m_tabEndTime << '\n';

  // Take the search times to be a second either side of the actual times, just
  // in case
  DateAndTime startSearchTime = m_tabStartTime - 1.0;
  DateAndTime endSearchTime = m_tabEndTime + 1.0;

  // Don't let the user change the time range
  QStringList enabled;
  enabled << "Filename"
          << "InputWorkspace"
          << "UnrollAll"
          << "SpecifyAlgorithmVersions";

  // Give some indication to the user that they will have to specify the
  // workspace
  if (m_pythonExportWsName.empty())
    g_log.warning("This tab has not specified a result workspace name.");

  // Set default properties
  QHash<QString, QString> props;
  props["Filename"] = "IndirectInterfacePythonExport.py";
  props["InputWorkspace"] = QString::fromStdString(m_pythonExportWsName);
  props["SpecifyAlgorithmVersions"] = "Specify All";
  props["UnrollAll"] = "1";
  props["StartTimestamp"] =
      QString::fromStdString(startSearchTime.toISO8601String());
  props["EndTimestamp"] =
      QString::fromStdString(endSearchTime.toISO8601String());

  // Create an algorithm dialog for the script export algorithm
  MantidQt::API::InterfaceManager interfaceManager;
  MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialogFromName(
      "GeneratePythonScript", -1, nullptr, false, props, "", enabled);

  // Show the dialog
  dlg->show();
  dlg->raise();
  dlg->activateWindow();
}

/**
 * Returns the allowed input data file extensions
 * @return The allowed input data file extensions
 */
QStringList IndirectTab::getAllowedExtensions() const { return {".nxs"}; }

/**
 * Run the load algorithm with the supplied filename and spectrum range
 *
 * @param filename :: The name of the file to load
 * @param outputName :: The name of the output workspace
 * @param specMin :: Lower spectra bound
 * @param specMax :: Upper spectra bound
 * @return If the algorithm was successful
 */
bool IndirectTab::loadFile(const QString &filename, const QString &outputName,
                           const int specMin, const int specMax,
                           bool loadHistory) {
  const auto algName = loadHistory ? "Load" : "LoadNexusProcessed";

  auto loader = AlgorithmManager::Instance().createUnmanaged(algName, -1);
  loader->initialize();
  loader->setProperty("Filename", filename.toStdString());
  loader->setProperty("OutputWorkspace", outputName.toStdString());
  setPropertyIf(loader, "SpectrumMin", castToString(specMin), specMin != -1);
  setPropertyIf(loader, "SpectrumMax", castToString(specMax), specMax != -1);
  setPropertyIf(loader, "LoadHistory", loadHistory ? "1" : "0", !loadHistory);
  loader->execute();

  return loader->isExecuted();
}

std::string
IndirectTab::getInterfaceProperty(std::string const &interfaceName,
                                  std::string const &propertyName) const {
  auto const directory =
      ConfigService::Instance().getString("instrumentDefinition.directory");
  auto const filepath = directory + "Indirect_Interface_Properties.xml";
  return getAttributeFromFile(filepath, interfaceName, propertyName);
}

QStringList IndirectTab::getExtensions(std::string const &interfaceName) const {
  return convertToQStringList(getInterfaceProperty(interfaceName, "extensions"),
                              ",");
}

QStringList
IndirectTab::getCalibrationExtensions(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "calib-extensions"), ",");
}

QStringList
IndirectTab::getSampleFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "sample-file-suffixes"), ",");
}

QStringList
IndirectTab::getSampleWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "sample-workspace-suffixes"), ",");
}

QStringList
IndirectTab::getResolutionFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "resolution-file-suffixes"), ",");
}

QStringList
IndirectTab::getResolutionWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "resolution-workspace-suffixes"),
      ",");
}

QStringList
IndirectTab::getCalibrationFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "calib-file-suffixes"), ",");
}

QStringList
IndirectTab::getCalibrationWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "calib-workspace-suffixes"), ",");
}

/**
 * Configures the SaveNexusProcessed algorithm to save a workspace in the
 * default save directory and adds the algorithm to the batch queue.
 *
 * @param wsName Name of workspace to save
 * @param filename Name of file to save as (including extension)
 */
void IndirectTab::addSaveWorkspaceToQueue(const QString &wsName,
                                          const QString &filename) {
  addSaveWorkspaceToQueue(wsName.toStdString(), filename.toStdString());
}

void IndirectTab::addSaveWorkspaceToQueue(const std::string &wsName,
                                          const std::string &filename) {
  // Setup the input workspace property
  API::BatchAlgorithmRunner::AlgorithmRuntimeProps saveProps;
  saveProps["InputWorkspace"] = wsName;

  // Setup the algorithm
  auto saveAlgo = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlgo->initialize();

  if (filename.empty())
    saveAlgo->setProperty("Filename", wsName + ".nxs");
  else
    saveAlgo->setProperty("Filename", filename);

  // Add the save algorithm to the batch
  m_batchAlgoRunner->addAlgorithm(saveAlgo, saveProps);
}

/**
 * Gets the suffix of a workspace (i.e. part after last underscore (red, sqw)).
 *
 * @param wsName Name of workspace
 * @return Suffix, or empty string if no underscore
 */
QString IndirectTab::getWorkspaceSuffix(const QString &wsName) {
  int lastUnderscoreIndex = wsName.lastIndexOf("_");
  if (lastUnderscoreIndex == -1)
    return QString();

  return wsName.right(lastUnderscoreIndex);
}

/**
 * Returns the basename of a workspace (i.e. the part before the last
 *underscore)
 *
 * e.g. basename of irs26176_graphite002_red is irs26176_graphite002
 *
 * @param wsName Name of workspace
 * @return Base name, or wsName if no underscore
 */
QString IndirectTab::getWorkspaceBasename(const QString &wsName) {
  int lastUnderscoreIndex = wsName.lastIndexOf("_");
  if (lastUnderscoreIndex == -1)
    return QString(wsName);

  return wsName.left(lastUnderscoreIndex);
}

/**
 * Allows the user to turn the plotting of error bars off and on
 *
 * @param errorBars :: true if you want output plots to have error bars
 */
void IndirectTab::setPlotErrorBars(bool errorBars) {
  m_plotErrorBars = errorBars;
}

/**
 * Plots different spectra from multiple workspaces on the same plot
 *
 * This uses the plotSpectrum function from the Python API.
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param workspaceIndices List of indices to plot
 */
void IndirectTab::plotMultipleSpectra(
    const QStringList &workspaceNames,
    const std::vector<int> &workspaceIndices) {

  if (workspaceNames.isEmpty())
    return;
  if (workspaceNames.length() != static_cast<int>(workspaceIndices.size()))
    return;

  QString pyInput = "from mantidplot import plotSpectrum\n";
  pyInput += "current_window = plotSpectrum('";
  pyInput += workspaceNames[0];
  pyInput += "', ";
  pyInput += QString::number(workspaceIndices[0]);
  pyInput += ")\n";

  for (int i = 1; i < workspaceNames.size(); i++) {
    pyInput += "plotSpectrum('";
    pyInput += workspaceNames[i];
    pyInput += "', ";
    pyInput += QString::number(workspaceIndices[i]);
    pyInput += ", window=current_window)\n";
  }
  m_pythonRunner.runPythonCode(pyInput);
}

/**
 * Creates a spectrum plot of one or more workspaces at a given spectrum
 * index.
 *
 * This uses the plotSpectrum function from the Python API.
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param spectraIndex Index of spectrum from each workspace to plot
 */
void IndirectTab::plotSpectrum(const QStringList &workspaceNames,
                               const int &spectraIndex) {
  if (!workspaceNames.isEmpty()) {
    const QString errors = m_plotErrorBars ? "True" : "False";

    QString pyInput = "from mantidplot import plotSpectrum\n";
    pyInput += "plotSpectrum(['";
    pyInput += workspaceNames.join("','");
    pyInput += "'], ";
    pyInput += QString::number(spectraIndex);
    pyInput += ", error_bars=" + errors + ")\n";

    m_pythonRunner.runPythonCode(pyInput);
  }
}

/**
 * Creates a spectrum plot of a single workspace at a given spectrum
 * index.
 *
 * @param workspaceName Names of workspace to plot
 * @param spectraIndex Workspace Index of spectrum to plot
 * @param errorBars Is true if you want to plot the error bars
 */
void IndirectTab::plotSpectrum(const QString &workspaceName,
                               const int &spectraIndex) {
  if (!workspaceName.isEmpty()) {
    QStringList workspaceNames;
    workspaceNames << workspaceName;
    plotSpectrum(workspaceNames, spectraIndex);
  }
}

/**
 * Creates a spectrum plot of one or more workspaces with the range of
 * spectra [specStart, specEnd)
 *
 * This uses the plotSpectrum function from the Python API.
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param specStart Range start index
 * @param specEnd Range end index
 */
void IndirectTab::plotSpectrum(const QStringList &workspaceNames, int specStart,
                               int specEnd) {
  if (workspaceNames.isEmpty())
    return;
  QString const errors = m_plotErrorBars ? "True" : "False";

  QString pyInput = "from mantidplot import plotSpectrum\n";

  pyInput += "plotSpectrum(['";
  pyInput += workspaceNames.join("','");
  pyInput += "'], list(range(";
  pyInput += QString::number(specStart);
  pyInput += ",";
  pyInput += QString::number(specEnd + 1);
  pyInput += ")), error_bars=" + errors + ")\n";

  m_pythonRunner.runPythonCode(pyInput);
}

/**
 * Creates a spectrum plot of a single workspace with the range of
 * spectra [specStart, specEnd)
 *
 * This uses the plotSpectrum function from the Python API.
 *
 * @param workspaceName Names of workspace to plot
 * @param specStart Range start index
 * @param specEnd Range end index
 */
void IndirectTab::plotSpectrum(const QString &workspaceName, int specStart,
                               int specEnd) {
  if (workspaceName.isEmpty())
    return;

  QStringList workspaceNames;
  workspaceNames << workspaceName;
  plotSpectrum(workspaceNames, specStart, specEnd);
}

/**
 * Creates a spectrum plot of one or more workspaces with a set
 *  of spectra specified in a vector
 *
 * This uses the plotSpectrum function from the Python API.
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param wsIndices List of indices of spectra to plot
 */
void IndirectTab::plotSpectra(const QStringList &workspaceNames,
                              const std::vector<int> &wsIndices) {
  if (workspaceNames.isEmpty() || wsIndices.empty())
    return;

  QString const errors = m_plotErrorBars ? "True" : "False";

  QString pyInput = "from mantidplot import plotSpectrum\n";

  pyInput += "plotSpectrum(['";
  pyInput += workspaceNames.join("','");
  pyInput += "'], [";
  pyInput += QString::number(wsIndices[0]);
  for (size_t i = 1; i < wsIndices.size(); i++) {
    pyInput += " ,";
    pyInput += QString::number(wsIndices[i]);
  }
  pyInput += "]";
  pyInput += ", error_bars=" + errors + ")\n";
  m_pythonRunner.runPythonCode(pyInput);
}

/**
 * Creates a spectrum plot of a single workspace with a set
 *  of spectra specified in a vector
 *
 * @param workspaceName Name of workspace to plot
 * @param wsIndices List of indices of spectra to plot
 */
void IndirectTab::plotSpectra(const QString &workspaceName,
                              const std::vector<int> &wsIndices) {
  if (workspaceName.isEmpty()) {
    return;
  }
  if (wsIndices.empty()) {
    return;
  }
  QStringList workspaceNames;
  workspaceNames << workspaceName;
  plotSpectra(workspaceNames, wsIndices);
}

void IndirectTab::plotTiled(std::string const &workspaceName,
                            std::size_t const &fromIndex,
                            std::size_t const &toIndex) {
  auto const numberOfPlots = toIndex - fromIndex + 1;
  if (numberOfPlots != 0) {
    QString pyInput = "from mantidplot import newTiledWindow\n";
    pyInput += "newTiledWindow(sources=[";
    for (auto index = fromIndex; index <= toIndex; ++index) {
      if (index > fromIndex)
        pyInput += ",";

      std::string const pyInStr =
          "(['" + workspaceName + "'], " + std::to_string(index) + ")";
      pyInput += QString::fromStdString(pyInStr);
    }
    pyInput += QString::fromStdString("])\n");
    m_pythonRunner.runPythonCode(pyInput);
  }
}

/**
 * Plots a contour (2D) plot of a given workspace.
 *
 * This uses the plot2D function from the Python API.
 *
 * @param workspaceName Name of workspace to plot
 */
void IndirectTab::plot2D(const QString &workspaceName) {
  if (workspaceName.isEmpty())
    return;

  QString pyInput = "from mantidplot import plot2D\n";

  pyInput += "plot2D('";
  pyInput += workspaceName;
  pyInput += "')\n";

  m_pythonRunner.runPythonCode(pyInput);
}

/**
 * Creates a time bin plot of one or more workspaces at a given spectrum
 * index.
 *
 * This uses the plotTimeBin function from the Python API.
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param binIndex Index of spectrum from each workspace to plot
 */
void IndirectTab::plotTimeBin(const QStringList &workspaceNames, int binIndex) {
  if (workspaceNames.isEmpty())
    return;

  QString const errors = m_plotErrorBars ? "True" : "False";

  QString pyInput = "from mantidplot import plotTimeBin\n";

  pyInput += "plotTimeBin(['";
  pyInput += workspaceNames.join("','");
  pyInput += "'], ";
  pyInput += QString::number(binIndex);
  pyInput += ", error_bars=" + errors + ")\n";

  m_pythonRunner.runPythonCode(pyInput);
}

/**
 * Creates a time bin plot of a single workspace at a given spectrum
 * index.
 *
 * @param workspaceName Names of workspace to plot
 * @param binIndex Index of spectrum to plot
 */
void IndirectTab::plotTimeBin(const QString &workspaceName, int binIndex) {
  if (workspaceName.isEmpty())
    return;

  QStringList workspaceNames;
  workspaceNames << workspaceName;
  plotTimeBin(workspaceNames, binIndex);
}

/*
 * Resizes the range (y-axis) of the specified plot preview given the specified
 * range
 *
 * @param preview The plot preview whose range to resize.
 * @param range   The range to resize to, as a pair of minimum and maximum value
 */
void IndirectTab::resizePlotRange(MantidQt::MantidWidgets::PreviewPlot *preview,
                                  QPair<double, double> range) {
  preview->resizeX();
  preview->setAxisRange(range, QwtPlot::yLeft);
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
void IndirectTab::setPlotPropertyRange(RangeSelector *rs, QtProperty *min,
                                       QtProperty *max,
                                       const QPair<double, double> &bounds) {
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
void IndirectTab::setRangeSelector(RangeSelector *rs, QtProperty *lower,
                                   QtProperty *upper,
                                   const QPair<double, double> &bounds) {
  m_dblManager->setValue(lower, bounds.first);
  m_dblManager->setValue(upper, bounds.second);
  rs->setMinimum(bounds.first);
  rs->setMaximum(bounds.second);
}

/**
 * Gets the energy mode from a workspace based on the X unit.
 *
 * Units of dSpacing typically denote diffraction, hence Elastic.
 * All other units default to spectroscopy, therefore Indirect.
 *
 * @param ws Pointer to the workspace
 * @return Energy mode
 */
std::string IndirectTab::getEMode(Mantid::API::MatrixWorkspace_sptr ws) {
  Mantid::Kernel::Unit_sptr xUnit = ws->getAxis(0)->unit();
  std::string xUnitName = xUnit->caption();

  g_log.debug() << "X unit name is: " << xUnitName << '\n';

  if (boost::algorithm::find_first(xUnitName, "d-Spacing"))
    return "Elastic";

  return "Indirect";
}

/**
 * Gets the eFixed value from the workspace using the instrument parameters.
 *
 * @param ws Pointer to the workspace
 * @return eFixed value
 */
double IndirectTab::getEFixed(Mantid::API::MatrixWorkspace_sptr ws) {
  Mantid::Geometry::Instrument_const_sptr inst = ws->getInstrument();
  if (!inst)
    throw std::runtime_error("No instrument on workspace");

  // Try to get the parameter form the base instrument
  if (inst->hasParameter("Efixed"))
    return inst->getNumberParameter("Efixed")[0];

  // Try to get it form the analyser component
  if (inst->hasParameter("analyser")) {
    std::string analyserName = inst->getStringParameter("analyser")[0];
    auto analyserComp = inst->getComponentByName(analyserName);

    if (analyserComp && analyserComp->hasParameter("Efixed"))
      return analyserComp->getNumberParameter("Efixed")[0];
  }

  throw std::runtime_error("Instrument has no efixed parameter");
}

/**
 * Checks the workspace's instrument for a resolution parameter to use as
 * a default for the energy range on the mini plot
 *
 * @param workspace :: Name of the workspace to use
 * @param res :: The retrieved values for the resolution parameter (if one was
 *found)
 */
bool IndirectTab::getResolutionRangeFromWs(const QString &workspace,
                                           QPair<double, double> &res) {
  auto ws = Mantid::API::AnalysisDataService::Instance()
                .retrieveWS<const Mantid::API::MatrixWorkspace>(
                    workspace.toStdString());
  return getResolutionRangeFromWs(ws, res);
}

/**
 * Checks the workspace's instrument for a resolution parameter to use as
 * a default for the energy range on the mini plot
 *
 * @param ws :: Pointer to the workspace to use
 * @param res :: The retrieved values for the resolution parameter (if one was
 *found)
 */
bool IndirectTab::getResolutionRangeFromWs(
    Mantid::API::MatrixWorkspace_const_sptr ws, QPair<double, double> &res) {
  auto inst = ws->getInstrument();
  auto analyser = inst->getStringParameter("analyser");

  if (analyser.size() > 0) {
    auto comp = inst->getComponentByName(analyser[0]);
    if (comp) {
      auto params = comp->getNumberParameter("resolution", true);

      // set the default instrument resolution
      if (params.size() > 0) {
        res = qMakePair(-params[0], params[0]);
        return true;
      }
    }
  }

  return false;
}

/**
 * Runs an algorithm async
 *
 * @param algorithm :: The algorithm to be run
 */
void IndirectTab::runAlgorithm(const Mantid::API::IAlgorithm_sptr algorithm) {
  algorithm->setRethrows(true);

  // There should never really be unexecuted algorithms in the queue, but it is
  // worth warning in case of possible weirdness
  size_t batchQueueLength = m_batchAlgoRunner->queueLength();
  if (batchQueueLength > 0)
    g_log.warning() << "Batch queue already contains " << batchQueueLength
                    << " algorithms!\n";

  m_batchAlgoRunner->addAlgorithm(algorithm);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles getting the results of an algorithm running async
 *
 * @param error :: True if execution failed, false otherwise
 */
void IndirectTab::algorithmFinished(bool error) {
  m_tabEndTime = DateAndTime::getCurrentTime();

  if (error) {
    emit showMessageBox(
        "Error running algorithm. \nSee results log for details.");
  }
}

/**
 * Run Python code and return anything printed to stdout.
 *
 * @param code Python code to execute
 * @param no_output Enable to ignore any output
 * @returns What was printed to stdout
 */
QString IndirectTab::runPythonCode(QString code, bool no_output) {
  return m_pythonRunner.runPythonCode(code, no_output);
}

/**
 * Checks if the ADS contains a workspace and opens a message box if not
 * @param workspaceName The name of the workspace to look for
 * @param plotting      If true use plotting error message, false use saving
 * error
 *                      message
 * @return              False if no workspace found, True if workspace found
 */
bool IndirectTab::checkADSForPlotSaveWorkspace(const std::string &workspaceName,
                                               const bool plotting,
                                               const bool warn) {
  const auto workspaceExists =
      AnalysisDataService::Instance().doesExist(workspaceName);
  if (warn && !workspaceExists) {
    const std::string plotSave = plotting ? "plotting" : "saving";
    const auto errorMessage = "Error while " + plotSave +
                              ":\nThe workspace \"" + workspaceName +
                              "\" could not be found.";
    const char *textMessage = errorMessage.c_str();
    QMessageBox::warning(nullptr, tr("Indirect "), tr(textMessage));
  }
  return workspaceExists;
}

std::unordered_map<std::string, size_t> IndirectTab::extractAxisLabels(
    Mantid::API::MatrixWorkspace_const_sptr workspace,
    const size_t &axisIndex) const {
  Axis *axis = workspace->getAxis(axisIndex);
  if (!axis->isText())
    return std::unordered_map<std::string, size_t>();

  TextAxis *textAxis = boost::static_pointer_cast<TextAxis>(axis);
  std::unordered_map<std::string, size_t> labels;

  for (size_t i = 0; i < textAxis->length(); ++i)
    labels[textAxis->label(i)] = i;
  return labels;
}

/*
 * Converts a standard vector of standard strings to a QVector of QStrings.
 *
 * @param stringVec The standard vector of standard strings to convert.
 * @return          A QVector of QStrings.
 */
QVector<QString> IndirectTab::convertStdStringVector(
    const std::vector<std::string> &stringVec) const {
  QVector<QString> resultVec;
  resultVec.reserve(boost::numeric_cast<int>(stringVec.size()));

  for (auto &str : stringVec) {
    resultVec.push_back(QString::fromStdString(str));
  }
  return resultVec;
}

} // namespace CustomInterfaces
} // namespace MantidQt
