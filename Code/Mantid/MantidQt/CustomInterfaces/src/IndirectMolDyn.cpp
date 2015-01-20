#include "MantidQtCustomInterfaces/IndirectMolDyn.h"

#include <QFileInfo>
#include <QString>

using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    IndirectMolDyn::IndirectMolDyn(QWidget * parent) :
      IndirectSimulationTab(parent)
    {
      m_uiForm.setupUi(parent);

      connect(m_uiForm.ckCropEnergy, SIGNAL(toggled(bool)), m_uiForm.dspMaxEnergy, SLOT(setEnabled(bool)));
      connect(m_uiForm.ckResolution, SIGNAL(toggled(bool)), m_uiForm.dsResolution, SLOT(setEnabled(bool)));
    }

    void IndirectMolDyn::setup()
    {
    }

    /**
     * Validate the form to check the program can be run
     *
     * @return :: Whether the form was valid
     */
    bool IndirectMolDyn::validate()
    {
      QString filename = m_uiForm.mwRun->getFirstFilename();
      QFileInfo finfo(filename);
      QString ext = finfo.extension().toLower();

      if(ext != "dat" && ext != "cdl")
      {
        emit showMessageBox("File is not of expected type.\n File type must be .dat or .cdl");
        return false;
      }

      QString functions = m_uiForm.leFunctionNames->text();
      if(ext == "cdl" && functions.isEmpty())
      {
        emit showMessageBox("Must specify at least one function when loading CDL file.");
        return false;
      }

      if(m_uiForm.ckResolution->isChecked() && !m_uiForm.dsResolution->isValid())
      {
        emit showMessageBox("Invalid resolution file.");
        return false;
      }

      if(m_uiForm.ckResolution->isChecked() && !m_uiForm.ckSymmetrise->isChecked())
      {
        emit showMessageBox("Must symmetrise when convolving with resolution.");
        return false;
      }

      return true;
    }

    /**
     * Collect the settings on the GUI and run the MolDyn algorithm.
     */
    void IndirectMolDyn::run()
    {
      // Get filename and base filename (for naming output workspace group)
      QString filename = m_uiForm.mwRun->getFirstFilename();
      QFileInfo fi(filename);
      QString baseName = fi.baseName();

      // Setup algorithm
      IAlgorithm_sptr molDynAlg = AlgorithmManager::Instance().create("MolDyn");
      molDynAlg->setProperty("Filename", filename.toStdString());
      molDynAlg->setProperty("Functions", m_uiForm.leFunctionNames->text().toStdString());
      molDynAlg->setProperty("SymmetriseEnergy", m_uiForm.ckSymmetrise->isChecked());
      molDynAlg->setProperty("Save", m_uiForm.ckSave->isChecked());
      molDynAlg->setProperty("Plot", m_uiForm.cbPlot->currentText().toStdString());
      molDynAlg->setProperty("OutputWorkspace", baseName.toStdString());

      // Set energy crop option
      if(m_uiForm.ckCropEnergy->isChecked())
        molDynAlg->setProperty("MaxEnergy", QString::number(m_uiForm.dspMaxEnergy->value()).toStdString());

      // Set instrument resolution option
      if(m_uiForm.ckResolution->isChecked())
        molDynAlg->setProperty("Resolution", m_uiForm.dsResolution->getCurrentDataName().toStdString());

      runAlgorithm(molDynAlg);
    }

    /**
     * Set the data selectors to use the default save directory
     * when browsing for input files.
     *
     * @param settings :: The settings to loading into the interface
     */
    void IndirectMolDyn::loadSettings(const QSettings& settings)
    {
      m_uiForm.mwRun->readSettings(settings.group());
    }

  } // namespace CustomInterfaces
} // namespace MantidQt
