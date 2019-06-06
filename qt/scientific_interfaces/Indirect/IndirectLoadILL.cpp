// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectLoadILL.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

#include <QFileInfo>
#include <QStringList>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {

std::string getInstrumentParameter(Instrument_const_sptr &instrument,
                                   std::string const &parameter,
                                   std::string const &defaultValue) {
  if (instrument->hasParameter(parameter))
    return instrument->getStringParameter(parameter)[0];
  return defaultValue;
}

std::string constructRunName(bool isILL, std::string const &instrumentName,
                             std::string const &runNumber) {
  return isILL ? instrumentName + "_" + runNumber : instrumentName + runNumber;
}

std::string constructPrefix(std::string const &runName,
                            std::string const &analyser,
                            std::string const &reflection) {
  auto const prefix = runName + '_' + analyser + reflection;
  return (!analyser.empty() && !reflection.empty()) ? prefix + "_" : prefix;
}

std::string constructPrefix(std::string const &runName,
                            Instrument_const_sptr instrument) {
  auto const analyser = getInstrumentParameter(instrument, "analyser", "");
  auto const reflection = getInstrumentParameter(instrument, "reflection", "");
  return constructPrefix(runName, analyser, reflection);
}

std::string getWorkspacePrefix(MatrixWorkspace_const_sptr workspace,
                               std::string const &facility) {
  auto const instrument = workspace->getInstrument();
  auto const runName =
      constructRunName(facility == "ILL", instrument->getName(),
                       std::to_string(workspace->getRunNumber()));
  return constructPrefix(runName, instrument);
}

std::string getWorkspacePrefix(std::string const &workspaceName) {
  auto &ads = AnalysisDataService::Instance();
  if (!workspaceName.empty() && ads.doesExist(workspaceName)) {
    auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName);
    auto const facility =
        ConfigService::Instance().getString("default.facility");
    return getWorkspacePrefix(workspace, facility);
  }
  return "";
}

void renameWorkspace(std::string const &inputName,
                     std::string const &outputName) {
  auto renamer = AlgorithmManager::Instance().create("RenameWorkspace");
  renamer->initialize();
  renamer->setProperty("InputWorkspace", inputName);
  renamer->setProperty("OutputWorkspace", outputName);
  renamer->execute();
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
IndirectLoadILL::IndirectLoadILL(QWidget *parent) : IndirectToolsTab(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));

  connect(m_uiForm.mwRun, SIGNAL(filesFound()), this, SLOT(handleFilesFound()));
  connect(m_uiForm.chkUseMap, SIGNAL(toggled(bool)), m_uiForm.mwMapFile,
          SLOT(setEnabled(bool)));
}

/**
 * Run any tab setup code.
 */
void IndirectLoadILL::setup() {}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool IndirectLoadILL::validate() {
  QString filename = m_uiForm.mwRun->getFirstFilename();
  QFileInfo finfo(filename);
  QString ext = finfo.suffix().toLower();

  bool invalidExt = (ext != "asc" && ext != "inx" && ext != "nxs");

  if (invalidExt) {
    emit showMessageBox(
        "File is not of expected type:\n File type must be .asc, .inx or .nxs");
  }

  return !invalidExt;
}

/**
 * Collect the settings on the GUI and build a python
 * script that runs IndirectLoadILL
 */
void IndirectLoadILL::run() {
  setRunIsRunning(true);

  QString plot("False");
  QString save("None");

  QString useMap("False");
  QString rejectZero("False");

  QString const filename = m_uiForm.mwRun->getFirstFilename();
  QFileInfo const finfo(filename);
  QString ext = finfo.suffix().toLower();

  QString const instrument =
      m_uiForm.iicInstrumentConfiguration->getInstrumentName();
  QString const analyser =
      m_uiForm.iicInstrumentConfiguration->getAnalyserName();
  QString const reflection =
      m_uiForm.iicInstrumentConfiguration->getReflectionName();

  if (m_uiForm.chkUseMap->isChecked()) {
    useMap = "True";
  }
  QString const mapPath = m_uiForm.mwMapFile->getFirstFilename();

  if (m_uiForm.chkRejectZero->isChecked()) {
    rejectZero = "True";
  }

  // output options
  if (m_uiForm.chkSave->isChecked()) {
    save = "True";
  }
  plot = m_uiForm.cbPlot->currentText();

  if (instrument == "IN16B") {
    auto const temporaryName = "__tmp_IndirectLoadASCII_IN16B";

    loadILLData(filename.toStdString(), temporaryName);
    renameWorkspace(temporaryName, getWorkspacePrefix(temporaryName) + "red");
  } else {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QString pyInput("");
    QString pyFunc("");
    // IN13 has a different loading routine
    if (instrument == "IN13") {
      pyFunc = "IN13Start";
    } else if (ext == "asc") // using ascii files
    {
      pyFunc += "IbackStart";
    } else if (ext == "inx") // using inx files
    {
      pyFunc += "InxStart";
    } else {
      setRunIsRunning(false);
      emit showMessageBox("Could not find appropriate loading routine for " +
                          filename);
      return;
    }

    pyInput += "from IndirectNeutron import " + pyFunc + "\n";
    pyInput += pyFunc + "('" + instrument + "','" + filename + "','" +
               analyser + "','" + reflection + "'," + rejectZero + "," +
               useMap + ",'" + mapPath +
               "'"
               ",'" +
               plot + "'," + save + ")";
    runPythonScript(pyInput);
#else
    emit showMessageBox("IN16B is currently the only instrument supported in "
                        "LoadILL on Mantid Workbench.");
#endif
  }

  setRunIsRunning(false);
}

void IndirectLoadILL::loadILLData(std::string const &filename,
                                  std::string const &outputName) {
  auto loader = AlgorithmManager::Instance().create("LoadILLIndirect");
  loader->initialize();
  loader->setProperty("Filename", filename);
  loader->setProperty("OutputWorkspace", outputName);
  loader->execute();
}

/**
 * Set the file browser to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The settings to loading into the interface
 */
void IndirectLoadILL::loadSettings(const QSettings &settings) {
  m_uiForm.mwRun->readSettings(settings.group());
}

/**
 * Set the instrument selected in the combobox based on
 * the file name of the run is possible.
 *
 * Assumes that names have the form \<instrument\>_\<run-number\>.\<ext\>
 */
void IndirectLoadILL::handleFilesFound() {
  // get first part of basename
  QString filename = m_uiForm.mwRun->getFirstFilename();
  QFileInfo finfo(filename);
  QStringList fnameParts = finfo.baseName().split('_');

  if (fnameParts.size() > 0) {
    // Check if the first part of the name is in the instruments list
    m_uiForm.iicInstrumentConfiguration->setInstrument(fnameParts[0]);
  }
}

void IndirectLoadILL::runClicked() { runTab(); }

void IndirectLoadILL::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setRunEnabled(!running);
  setPlotOptionsEnabled(!running);
}

void IndirectLoadILL::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectLoadILL::setPlotOptionsEnabled(bool enabled) {
  m_uiForm.cbPlot->setEnabled(enabled);
}

} // namespace CustomInterfaces
} // namespace MantidQt
