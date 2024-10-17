// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/InelasticTab.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
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

#include <MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h>
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
Mantid::Kernel::Logger g_log("InelasticTab");

std::string castToString(int value) { return boost::lexical_cast<std::string>(value); }

template <typename Predicate>
void setPropertyIf(const Algorithm_sptr &algorithm, std::string const &propName, std::string const &value,
                   Predicate const &condition) {
  if (!algorithm->existsProperty(propName))
    return;
  if (condition)
    algorithm->setPropertyValue(propName, value);
}
} // namespace

namespace MantidQt::CustomInterfaces {

InelasticTab::InelasticTab(QObject *parent)
    : QObject(parent), m_runPresenter(), m_properties(), m_dblManager(new QtDoublePropertyManager()),
      m_blnManager(new QtBoolPropertyManager()), m_grpManager(new QtGroupPropertyManager()),
      m_dblEdFac(new DoubleEditorFactory()), m_tabStartTime(DateAndTime::getCurrentTime()),
      m_tabEndTime(DateAndTime::maximum()), m_plotter(std::make_unique<Widgets::MplCpp::ExternalPlotter>()),
      m_adsInstance(Mantid::API::AnalysisDataService::Instance()) {
  m_parentWidget = dynamic_cast<QWidget *>(parent);

  m_batchAlgoRunner = new MantidQt::API::BatchAlgorithmRunner(m_parentWidget);
  m_valInt = new QIntValidator(m_parentWidget);
  m_valDbl = new QDoubleValidator(m_parentWidget);
  m_valPosDbl = new QDoubleValidator(m_parentWidget);

  const double tolerance = 0.00001;
  m_valPosDbl->setBottom(tolerance);

  connect(m_batchAlgoRunner, &API::BatchAlgorithmRunner::batchComplete, this, &InelasticTab::algorithmFinished);
}

void InelasticTab::setRunWidgetPresenter(std::unique_ptr<RunPresenter> presenter) {
  m_runPresenter = std::move(presenter);
}

/**
 * Handles generating a Python script for the algorithms run on the current tab.
 */
void InelasticTab::exportPythonScript() {
  g_log.information() << "Python export for workspace: " << m_pythonExportWsName << ", between " << m_tabStartTime
                      << " and " << m_tabEndTime << '\n';

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
  props["Filename"] = "InelasticInterfacePythonExport.py";
  props["InputWorkspace"] = QString::fromStdString(m_pythonExportWsName);
  props["SpecifyAlgorithmVersions"] = "Specify All";
  props["UnrollAll"] = "1";
  props["StartTimestamp"] = QString::fromStdString(startSearchTime.toISO8601String());
  props["EndTimestamp"] = QString::fromStdString(endSearchTime.toISO8601String());

  // Create an algorithm dialog for the script export algorithm
  MantidQt::API::InterfaceManager interfaceManager;
  MantidQt::API::AlgorithmDialog *dlg =
      interfaceManager.createDialogFromName("GeneratePythonScript", -1, nullptr, false, props, "", enabled);

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
bool InelasticTab::loadFile(const std::string &filename, const std::string &outputName, const int specMin,
                            const int specMax, bool loadHistory) {
  auto loader = AlgorithmManager::Instance().createUnmanaged("Load", -1);
  loader->initialize();
  loader->setProperty("Filename", filename);
  loader->setProperty("OutputWorkspace", outputName);
  setPropertyIf(loader, "SpectrumMin", castToString(specMin), specMin != -1);
  setPropertyIf(loader, "SpectrumMax", castToString(specMax), specMax != -1);
  setPropertyIf(loader, "LoadHistory", loadHistory ? "1" : "0", !loadHistory);
  loader->execute();

  return loader->isExecuted();
}

/**
 * Configures the SaveNexusProcessed algorithm to save a workspace in the
 * default save directory and adds the algorithm to the batch queue.
 *
 * @param wsName Name of workspace to save
 * @param filename Name of file to save as (including extension)
 */
void InelasticTab::addSaveWorkspaceToQueue(const QString &wsName, const QString &filename) {
  addSaveWorkspaceToQueue(wsName.toStdString(), filename.toStdString());
}

void InelasticTab::addSaveWorkspaceToQueue(const std::string &wsName, const std::string &filename) {
  // Setup the input workspace property
  auto saveProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  saveProps->setPropertyValue("InputWorkspace", wsName);

  // Setup the algorithm
  auto saveAlgo = AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlgo->initialize();

  if (filename.empty())
    saveAlgo->setProperty("Filename", wsName + ".nxs");
  else
    saveAlgo->setProperty("Filename", filename);

  // Add the save algorithm to the batch
  m_batchAlgoRunner->addAlgorithm(saveAlgo, std::move(saveProps));
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
void InelasticTab::setPlotPropertyRange(RangeSelector *rs, QtProperty *min, QtProperty *max,
                                        const QPair<double, double> &bounds) {
  m_dblManager->setRange(min, bounds.first, bounds.second);
  m_dblManager->setRange(max, bounds.first, bounds.second);
  rs->setBounds(bounds.first, bounds.second);
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
void InelasticTab::setRangeSelector(RangeSelector *rs, QtProperty *lower, QtProperty *upper,
                                    const QPair<double, double> &range,
                                    const std::optional<QPair<double, double>> &bounds) {
  m_dblManager->setValue(lower, range.first);
  m_dblManager->setValue(upper, range.second);
  rs->setRange(range.first, range.second);
  if (bounds) {
    // clamp the bounds of the selector
    const auto values = bounds.value();
    rs->setBounds(values.first, values.second);
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
void InelasticTab::setRangeSelectorMin(QtProperty *minProperty, QtProperty *maxProperty, RangeSelector *rangeSelector,
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
void InelasticTab::setRangeSelectorMax(QtProperty *minProperty, QtProperty *maxProperty, RangeSelector *rangeSelector,
                                       double newValue) {
  if (newValue >= minProperty->valueText().toDouble())
    rangeSelector->setMaximum(newValue);
  else
    m_dblManager->setValue(maxProperty, rangeSelector->getMaximum());
}

/**
 * Runs an algorithm async
 *
 * @param algorithm :: The algorithm to be run
 */
void InelasticTab::runAlgorithm(const Mantid::API::IAlgorithm_sptr &algorithm) {
  algorithm->setRethrows(true);

  // There should never really be unexecuted algorithms in the queue, but it is
  // worth warning in case of possible weirdness
  size_t batchQueueLength = m_batchAlgoRunner->queueLength();
  if (batchQueueLength > 0)
    g_log.warning() << "Batch queue already contains " << batchQueueLength << " algorithms!\n";

  m_batchAlgoRunner->addAlgorithm(algorithm);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles getting the results of an algorithm running async
 *
 * @param error :: True if execution failed, false otherwise
 */
void InelasticTab::algorithmFinished(bool error) {
  m_tabEndTime = DateAndTime::getCurrentTime();

  if (error) {
    emit showMessageBox("Error running algorithm. \nSee results log for details.");
  }
}

/**
 * Checks if the ADS contains a workspace and opens a message box if not
 * @param workspaceName The name of the workspace to look for
 * @param plotting      If true use plotting error message, false use saving
 * error
 *                      message
 * @return              False if no workspace found, True if workspace found
 */
bool InelasticTab::checkADSForPlotSaveWorkspace(const std::string &workspaceName, const bool plotting,
                                                const bool warn) {
  const auto workspaceExists = AnalysisDataService::Instance().doesExist(workspaceName);
  if (warn && !workspaceExists) {
    const std::string plotSave = plotting ? "plotting" : "saving";
    const auto errorMessage =
        "Error while " + plotSave + ":\nThe workspace \"" + workspaceName + "\" could not be found.";
    const char *textMessage = errorMessage.c_str();
    QMessageBox::warning(nullptr, tr("Indirect "), tr(textMessage));
  }
  return workspaceExists;
}

void InelasticTab::displayWarning(std::string const &message) {
  QMessageBox::warning(nullptr, "Warning!", QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces
