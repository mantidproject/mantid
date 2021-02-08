// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Unit.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QDomDocument>
#include <QFile>
#include <QMessageBox>
#include <QtXml>

#include <boost/algorithm/string/find.hpp>
#include <boost/pointer_cast.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using Mantid::Types::Core::DateAndTime;

namespace {
Mantid::Kernel::Logger g_log("IndirectTab");

double roundToPrecision(double value, double precision) {
  return value - std::remainder(value, precision);
}

QPair<double, double> roundRangeToPrecision(double rangeStart, double rangeEnd,
                                            double precision) {
  return QPair<double, double>(
      roundToPrecision(rangeStart, precision) + precision,
      roundToPrecision(rangeEnd, precision) - precision);
}

std::string castToString(int value) {
  return boost::lexical_cast<std::string>(value);
}

template <typename Predicate>
void setPropertyIf(const Algorithm_sptr &algorithm, std::string const &propName,
                   std::string const &value, Predicate const &condition) {
  if (condition)
    algorithm->setPropertyValue(propName, value);
}

std::string getAttributeFromTag(QDomElement const &tag,
                                QString const &attribute,
                                QString const &defaultValue) {
  if (tag.hasAttribute(attribute))
    return tag.attribute(attribute, defaultValue).toStdString();
  return defaultValue.toStdString();
}

bool hasCorrectAttribute(QDomElement const &child,
                         std::string const &attributeName,
                         std::string const &searchValue) {
  auto const name = QString::fromStdString(attributeName);
  return child.hasAttribute(name) &&
         child.attribute(name).toStdString() == searchValue;
}

std::string getInterfaceAttribute(QDomElement const &root,
                                  std::string const &interfaceName,
                                  std::string const &propertyName,
                                  std::string const &attribute) {
  // Loop through interfaces
  auto interfaceChild = root.firstChild().toElement();
  while (!interfaceChild.isNull()) {
    if (hasCorrectAttribute(interfaceChild, "id", interfaceName)) {

      // Loop through interface properties
      auto propertyChild = interfaceChild.firstChild().toElement();
      while (!propertyChild.isNull()) {

        // Return value of an attribute of the property if it is found
        if (propertyChild.tagName().toStdString() == propertyName)
          return getAttributeFromTag(propertyChild,
                                     QString::fromStdString(attribute), "");

        propertyChild = propertyChild.nextSibling().toElement();
      }
    }
    interfaceChild = interfaceChild.nextSibling().toElement();
  }
  return "";
}

std::string getInterfaceAttribute(QFile &file, std::string const &interfaceName,
                                  std::string const &propertyName,
                                  std::string const &attribute) {
  QDomDocument xmlBOM;
  xmlBOM.setContent(&file);
  return getInterfaceAttribute(xmlBOM.documentElement(), interfaceName,
                               propertyName, attribute);
}

QStringList convertToQStringList(std::vector<std::string> const &strings) {
  QStringList list;
  for (auto const &str : strings)
    list << QString::fromStdString(str);
  return list;
}

QStringList convertToQStringList(std::string const &str,
                                 std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  return convertToQStringList(subStrings);
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
      m_tabStartTime(DateAndTime::getCurrentTime()),
      m_tabEndTime(DateAndTime::maximum()),
      m_plotter(std::make_unique<IndirectPlotter>(this)) {
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
                                  std::string const &propertyName,
                                  std::string const &attribute) const {
  QFile file(":/interface-properties.xml");
  if (file.open(QIODevice::ReadOnly))
    return getInterfaceAttribute(file, interfaceName, propertyName, attribute);

  g_log.warning("There was an error while loading interface-properties.xml.");
  return "";
}

QStringList IndirectTab::getExtensions(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "EXTENSIONS", "all"), ",");
}

QStringList
IndirectTab::getCalibrationExtensions(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "EXTENSIONS", "calibration"), ",");
}

QStringList
IndirectTab::getSampleFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "sample"), ",");
}

QStringList
IndirectTab::getSampleWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "sample"), ",");
}

QStringList
IndirectTab::getVanadiumFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "vanadium"), ",");
}

QStringList
IndirectTab::getVanadiumWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "vanadium"),
      ",");
}

QStringList
IndirectTab::getResolutionFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "resolution"), ",");
}

QStringList
IndirectTab::getResolutionWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "resolution"),
      ",");
}

QStringList
IndirectTab::getCalibrationFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "calibration"), ",");
}

QStringList
IndirectTab::getCalibrationWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "calibration"),
      ",");
}

QStringList
IndirectTab::getContainerFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "container"), ",");
}

QStringList
IndirectTab::getContainerWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "container"),
      ",");
}

QStringList
IndirectTab::getCorrectionsFBSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "FILE-SUFFIXES", "corrections"), ",");
}

QStringList
IndirectTab::getCorrectionsWSSuffixes(std::string const &interfaceName) const {
  return convertToQStringList(
      getInterfaceProperty(interfaceName, "WORKSPACE-SUFFIXES", "corrections"),
      ",");
}

/**
 * Used to run python code
 *
 * @param pythonCode The python code to run
 */
void IndirectTab::runPythonCode(std::string const &pythonCode) {
  m_pythonRunner.runPythonCode(QString::fromStdString(pythonCode));
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
 * @param range :: The range to set the range selector to.
 */
void IndirectTab::setRangeSelector(
    RangeSelector *rs, QtProperty *lower, QtProperty *upper,
    const QPair<double, double> &bounds,
    const boost::optional<QPair<double, double>> &range) {
  m_dblManager->setValue(lower, bounds.first);
  m_dblManager->setValue(upper, bounds.second);
  if (range) {
    rs->setMinimum(range.get().first);
    rs->setMaximum(range.get().second);
    // clamp the bounds of the selector
    rs->setRange(range.get().first, range.get().second);
  } else {
    rs->setMinimum(bounds.first);
    rs->setMaximum(bounds.second);
  }
}

/**
 * Set the minimum of a range selector if it is less than the maximum value.
 * To be used when changing the min or max via the Property table
 *
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the minimum
 */
void IndirectTab::setRangeSelectorMin(QtProperty *minProperty,
                                      QtProperty *maxProperty,
                                      RangeSelector *rangeSelector,
                                      double newValue) {
  if (newValue <= maxProperty->valueText().toDouble())
    rangeSelector->setMinimum(newValue);
  else
    m_dblManager->setValue(minProperty, rangeSelector->getMinimum());
}

/**
 * Set the maximum of a range selector if it is greater than the minimum value
 * To be used when changing the min or max via the Property table
 *
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the maximum
 */
void IndirectTab::setRangeSelectorMax(QtProperty *minProperty,
                                      QtProperty *maxProperty,
                                      RangeSelector *rangeSelector,
                                      double newValue) {
  if (newValue >= minProperty->valueText().toDouble())
    rangeSelector->setMaximum(newValue);
  else
    m_dblManager->setValue(maxProperty, rangeSelector->getMaximum());
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
std::string IndirectTab::getEMode(const Mantid::API::MatrixWorkspace_sptr &ws) {
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
double IndirectTab::getEFixed(const Mantid::API::MatrixWorkspace_sptr &ws) {
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
    const Mantid::API::MatrixWorkspace_const_sptr &workspace,
    QPair<double, double> &res) {
  if (workspace) {
    auto const instrument = workspace->getInstrument();
    if (instrument && instrument->hasParameter("analyser")) {
      auto const analyser = instrument->getStringParameter("analyser");
      if (analyser.size() > 0) {
        auto comp = instrument->getComponentByName(analyser[0]);
        if (comp) {
          auto params = comp->getNumberParameter("resolution", true);

          // set the default instrument resolution
          if (params.size() > 0) {
            res = qMakePair(-params[0], params[0]);
            return true;
          }
        }
      }
    }
  }
  return false;
}

QPair<double, double>
IndirectTab::getXRangeFromWorkspace(std::string const &workspaceName,
                                    double precision) const {
  auto const &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    return getXRangeFromWorkspace(
        ads.retrieveWS<MatrixWorkspace>(workspaceName), precision);
  return QPair<double, double>(0.0, 0.0);
}

QPair<double, double> IndirectTab::getXRangeFromWorkspace(
    const Mantid::API::MatrixWorkspace_const_sptr &workspace,
    double precision) const {
  auto const xValues = workspace->x(0);
  return roundRangeToPrecision(xValues.front(), xValues.back(), precision);
}

/**
 * Runs an algorithm async
 *
 * @param algorithm :: The algorithm to be run
 */
void IndirectTab::runAlgorithm(const Mantid::API::IAlgorithm_sptr &algorithm) {
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
QString IndirectTab::runPythonCode(const QString &code, bool no_output) {
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

void IndirectTab::displayWarning(std::string const &message) {
  QMessageBox::warning(nullptr, "Warning!", QString::fromStdString(message));
}

std::unordered_map<std::string, size_t> IndirectTab::extractAxisLabels(
    const Mantid::API::MatrixWorkspace_const_sptr &workspace,
    const size_t &axisIndex) const {
  Axis *axis = workspace->getAxis(axisIndex);
  if (!axis->isText())
    return std::unordered_map<std::string, size_t>();

  auto *textAxis = static_cast<TextAxis *>(axis);
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
