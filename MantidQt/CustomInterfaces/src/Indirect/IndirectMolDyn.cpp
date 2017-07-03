#include "MantidQtCustomInterfaces/Indirect/IndirectMolDyn.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QFileInfo>
#include <QString>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
IndirectMolDyn::IndirectMolDyn(QWidget *parent)
    : IndirectSimulationTab(parent) {
  m_uiForm.setupUi(parent);

  connect(m_uiForm.ckCropEnergy, SIGNAL(toggled(bool)), m_uiForm.dspMaxEnergy,
          SLOT(setEnabled(bool)));
  connect(m_uiForm.ckResolution, SIGNAL(toggled(bool)), m_uiForm.dsResolution,
          SLOT(setEnabled(bool)));
  connect(m_uiForm.cbVersion, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(versionSelected(const QString &)));
  // Handle plotting and saving
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
}

void IndirectMolDyn::setup() {}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool IndirectMolDyn::validate() {
  UserInputValidator uiv;

  if (uiv.checkMWRunFilesIsValid("Data", m_uiForm.mwRun)) {
    QString filename = m_uiForm.mwRun->getFirstFilename();
    QString version = m_uiForm.cbVersion->currentText();
    QFileInfo finfo(filename);
    QString ext = finfo.suffix().toLower();

    if (version == "3") {
      if (ext != "dat" && ext != "cdl")
        uiv.addErrorMessage(
            "File is not of expected type.\n File type must be .dat or .cdl");

      QString functions = m_uiForm.leFunctionNames->text();
      if (ext == "cdl" && functions.isEmpty())
        uiv.addErrorMessage(
            "Must specify at least one function when loading CDL file.");
    }
  }

  // Validate resolution
  if (m_uiForm.ckResolution->isChecked())
    uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  emit showMessageBox(uiv.generateErrorMessage());
  return uiv.isAllInputValid();
}

/**
 * Collect the settings on the GUI and run the MolDyn algorithm.
 */
void IndirectMolDyn::run() {
  // Get filename and base filename (for naming output workspace group)
  QString filename = m_uiForm.mwRun->getFirstFilename();
  QFileInfo fi(filename);
  QString baseName = fi.baseName();

  // Setup algorithm
  IAlgorithm_sptr molDynAlg = AlgorithmManager::Instance().create("MolDyn");
  molDynAlg->setProperty("Data", filename.toStdString());
  molDynAlg->setProperty("Functions",
                         m_uiForm.leFunctionNames->text().toStdString());
  molDynAlg->setProperty("SymmetriseEnergy",
                         m_uiForm.ckSymmetrise->isChecked());
  molDynAlg->setProperty("OutputWorkspace", baseName.toStdString());

  // Set energy crop option
  if (m_uiForm.ckCropEnergy->isChecked())
    molDynAlg->setProperty(
        "MaxEnergy",
        QString::number(m_uiForm.dspMaxEnergy->value()).toStdString());

  // Set instrument resolution option
  if (m_uiForm.ckResolution->isChecked())
    molDynAlg->setProperty(
        "Resolution",
        m_uiForm.dsResolution->getCurrentDataName().toStdString());

  runAlgorithm(molDynAlg);

  // Enable plot and save
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
  m_uiForm.cbPlot->setEnabled(true);
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The settings to loading into the interface
 */
void IndirectMolDyn::loadSettings(const QSettings &settings) {
  m_uiForm.mwRun->readSettings(settings.group());
}

/**
 * Handles the version of nMoldyn being selected.
 *
 * @param version The version as a string ("3" or "4")
 */
void IndirectMolDyn::versionSelected(const QString &version) {
  bool version4(version == "4");
  m_uiForm.mwRun->isForDirectory(version4);
}
/**
 * Handle plotting of mantid workspace
 */
void IndirectMolDyn::plotClicked() {

  QString filename = m_uiForm.mwRun->getFirstFilename();
  QFileInfo fi(filename);
  QString baseName = fi.baseName();

  if (checkADSForPlotSaveWorkspace(baseName.toStdString(), true)) {

    WorkspaceGroup_sptr diffResultsGroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            baseName.toStdString());

    auto names = diffResultsGroup->getNames();
    auto plotType = m_uiForm.cbPlot->currentText();

    for (const auto wsName : names) {
      if (plotType == "Spectra" || plotType == "Both")
        plotSpectrum(QString::fromStdString(wsName));

      if (plotType == "Contour" || plotType == "Both")
        plot2D(QString::fromStdString(wsName));
    }
  }
}

/**
 * Handle saving workspaces
 */
void IndirectMolDyn::saveClicked() {

  QString filename = m_uiForm.mwRun->getFirstFilename();
  QFileInfo fi(filename);
  QString baseName = fi.baseName();

  if (checkADSForPlotSaveWorkspace(baseName.toStdString(), false))
    addSaveWorkspaceToQueue(baseName);
  m_batchAlgoRunner->executeBatchAsync();
}

} // namespace CustomInterfaces
} // namespace MantidQt
