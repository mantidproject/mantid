#include "MantidQtCustomInterfaces/Indirect/IndirectConvertToEnergy.h"

#include "MantidQtCustomInterfaces/Background.h"

#include <QFileInfo>
#include <QInputDialog>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectConvertToEnergy::IndirectConvertToEnergy(IndirectDataReduction * idrUI, QWidget * parent) :
      IndirectDataReductionTab(idrUI, parent),
      m_backgroundDialog(NULL), m_bgRemoval(false)
  {
    m_uiForm.setupUi(parent);

    // Add validators to UI form
    m_uiForm.leNoGroups->setValidator(m_valInt);

    // SIGNAL/SLOT CONNECTIONS
    // Update instrument information when a new instrument config is selected
    connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(setInstrumentDefault()));
    // Shows required mapping option UI widgets when a new mapping option is selected from drop down
    connect(m_uiForm.cbMappingOptions, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(mappingOptionSelected(const QString&)));
    // Shows background removal dialog when user clicks Background Removal
    connect(m_uiForm.pbBackgroundRemoval, SIGNAL(clicked()), this, SLOT(backgroundClicked()));
    // Plots raw input data when user clicks Plot Time
    connect(m_uiForm.pbPlotTime, SIGNAL(clicked()), this, SLOT(plotRaw()));
    // Shows message on run buton when user is inputting a run number
    connect(m_uiForm.dsRunFiles, SIGNAL(fileTextChanged(const QString &)), this, SLOT(pbRunEditing()));
    // Shows message on run button when Mantid is finding the file for a given run number
    connect(m_uiForm.dsRunFiles, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
    // Reverts run button back to normal when file finding has finished
    connect(m_uiForm.dsRunFiles, SIGNAL(fileFindingFinished()), this, SLOT(pbRunFinished()));

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

    // Update UI widgets to show default values
    mappingOptionSelected(m_uiForm.cbMappingOptions->currentText());
    backgroundRemoval();

    // Validate to remove invalid markers
    validateTab();
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectConvertToEnergy::~IndirectConvertToEnergy()
  {
  }

  void IndirectConvertToEnergy::setup()
  {
  }

  void IndirectConvertToEnergy::run()
  {
    using MantidQt::API::BatchAlgorithmRunner;

    IAlgorithm_sptr reductionAlg = AlgorithmManager::Instance().create("InelasticIndirectReduction", -1);
    reductionAlg->initialize();

    reductionAlg->setProperty("Instrument", getInstrumentConfiguration()->getInstrumentName().toStdString());
    reductionAlg->setProperty("Analyser", getInstrumentConfiguration()->getAnalyserName().toStdString());
    reductionAlg->setProperty("Reflection", getInstrumentConfiguration()->getReflectionName().toStdString());

    QString files = m_uiForm.dsRunFiles->getFilenames().join(",");
    reductionAlg->setProperty("InputFiles", files.toStdString());

    reductionAlg->setProperty("SumFiles", m_uiForm.ckSumFiles->isChecked());
    reductionAlg->setProperty("LoadLogs", m_uiForm.ckLoadLogs->isChecked());

    if(m_uiForm.ckUseCalib->isChecked())
    {
      QString calibWorkspaceName = m_uiForm.dsCalibrationFile->getCurrentDataName();
      reductionAlg->setProperty("CalibrationWorkspace", calibWorkspaceName.toStdString());
    }

    std::vector<long> detectorRange;
    detectorRange.push_back(m_uiForm.spSpectraMin->value());
    detectorRange.push_back(m_uiForm.spSpectraMax->value());
    reductionAlg->setProperty("DetectorRange", detectorRange);

    if(m_bgRemoval)
    {
      QPair<double, double> background = m_backgroundDialog->getRange();
      std::vector<double> backgroundRange;
      backgroundRange.push_back(background.first);
      backgroundRange.push_back(background.second);
      reductionAlg->setProperty("BackgroundRange", backgroundRange);
    }

    if(!m_uiForm.ckDoNotRebin->isChecked())
    {
      QString rebin;
      if(m_uiForm.cbRebinType->currentIndex() == 0)
        rebin = m_uiForm.spRebinLow->text() + "," + m_uiForm.spRebinWidth->text() + "," + m_uiForm.spRebinHigh->text();
      else
        rebin = m_uiForm.leRebinString->text();

      reductionAlg->setProperty("RebinString", rebin.toStdString());
    }

    if(m_uiForm.ckDetailedBalance->isChecked())
      reductionAlg->setProperty("DetailedBalance", m_uiForm.spDetailedBalance->value());

    if(m_uiForm.ckScaleMultiplier->isChecked())
      reductionAlg->setProperty("ScaleFactor", m_uiForm.spScaleMultiplier->value());

    if(m_uiForm.cbMappingOptions->currentText() != "Default")
    {
      QString grouping = createMapFile(m_uiForm.cbMappingOptions->currentText());
      reductionAlg->setProperty("Grouping", grouping.toStdString());
    }

    reductionAlg->setProperty("Fold", m_uiForm.ckFold->isChecked());
    reductionAlg->setProperty("SaveCM1", m_uiForm.ckCm1Units->isChecked());
    reductionAlg->setProperty("SaveFormats", getSaveFormats());

    reductionAlg->setProperty("OutputWorkspace", "IndirectEnergyTransfer_Workspaces");

    // Plot Output options
    switch(m_uiForm.cbPlotType->currentIndex())
    {
      case 0: // "None"
        break;
      case 1: // "Spectra"
        reductionAlg->setProperty("Plot", "spectra");
        break;
      case 2: // "Contour"
        reductionAlg->setProperty("Plot", "contour");
        break;
    }

    m_batchAlgoRunner->addAlgorithm(reductionAlg);
    m_batchAlgoRunner->executeBatchAsync();

    // Set output workspace name for Python export
    m_pythonExportWsName = "IndirectInergyTransfer_Workspaces";
  }

  /**
   * Handles completion of the algorithm.
   *
   * Sets result workspace for Python export and ungroups result WorkspaceGroup.
   *
   * @param error True if the algorithm was stopped due to error, false otherwise
   */
  void IndirectConvertToEnergy::algorithmComplete(bool error)
  {
    if(error)
      return;

    WorkspaceGroup_sptr energyTransferOutputGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IndirectEnergyTransfer_Workspaces");
    if(energyTransferOutputGroup->size() == 0)
      return;

    // Set workspace for Python export as the first result workspace
    m_pythonExportWsName = energyTransferOutputGroup->getNames()[0];

    // Ungroup the output workspace
    energyTransferOutputGroup->removeAll();
    AnalysisDataService::Instance().remove("IndirectEnergyTransfer_Workspaces");
  }

  bool IndirectConvertToEnergy::validate()
  {
    bool valid = true;

    // Run files input
    if(!m_uiForm.dsRunFiles->isValid())
      valid = false;

    // Calibration file input
    if(m_uiForm.ckUseCalib->isChecked() && !m_uiForm.dsCalibrationFile->isValid())
      valid = false;

    // Mapping selection
    if(
       (m_uiForm.cbMappingOptions->currentText() == "Groups" && m_uiForm.leNoGroups->text() == "")
       ||
       (m_uiForm.cbMappingOptions->currentText() == "File" && ! m_uiForm.dsMapFile->isValid())
      )
    {
      valid = false;
      m_uiForm.valNoGroups->setText("*");
    }
    else
    {
      m_uiForm.valNoGroups->setText("");
    }

    return valid;
  }


  /**
   * Called when the instrument has changed, used to update default values.
   */
  void IndirectConvertToEnergy::setInstrumentDefault()
  {
    std::map<QString, QString> instDetails = getInstrumentDetails();

    if(instDetails["spectra-min"].isEmpty() || instDetails["spectra-max"].isEmpty())
    {
      emit showMessageBox("Could not gather necessary data from parameter file.");
      return;
    }

    int specMin = instDetails["spectra-min"].toInt();
    int specMax = instDetails["spectra-max"].toInt();

    m_uiForm.spSpectraMin->setMinimum(specMin);
    m_uiForm.spSpectraMin->setMaximum(specMax);
    m_uiForm.spSpectraMin->setValue(specMin);

    m_uiForm.spSpectraMax->setMinimum(specMin);
    m_uiForm.spSpectraMax->setMaximum(specMax);
    m_uiForm.spSpectraMax->setValue(specMax);

    if(!instDetails["efixed-val"].isEmpty())
      m_uiForm.leEfixed->setText(instDetails["efixed-val"]);
    else
      m_uiForm.leEfixed->clear();

    // Default rebinning parameters can be set in instrument parameter file
    if(!instDetails["rebin-default"].isEmpty())
    {
      m_uiForm.leRebinString->setText(instDetails["rebin-default"]);
      m_uiForm.ckDoNotRebin->setChecked(false);
      QStringList rbp = instDetails["rebin-default"].split(",", QString::SkipEmptyParts);
      if ( rbp.size() == 3 )
      {
        m_uiForm.spRebinLow->setValue(rbp[0].toDouble());
        m_uiForm.spRebinWidth->setValue(rbp[1].toDouble());
        m_uiForm.spRebinHigh->setValue(rbp[2].toDouble());
        m_uiForm.cbRebinType->setCurrentIndex(0);
      }
      else
      {
        m_uiForm.cbRebinType->setCurrentIndex(1);
      }
    }
    else
    {
      m_uiForm.ckDoNotRebin->setChecked(true);
      m_uiForm.spRebinLow->setValue(0.0);
      m_uiForm.spRebinWidth->setValue(0.0);
      m_uiForm.spRebinHigh->setValue(0.0);
      m_uiForm.leRebinString->setText("");
    }

    if(!instDetails["cm-1-convert-choice"].isEmpty())
    {
      bool defaultOptions = instDetails["cm-1-convert-choice"] == "true";
      m_uiForm.ckCm1Units->setChecked(defaultOptions);
    }

    if(!instDetails["save-ascii-choice"].isEmpty())
    {
      bool defaultOptions = instDetails["save-ascii-choice"] == "true";
      m_uiForm.ckSaveASCII->setChecked(defaultOptions);
    }
  }

  /**
   * This function runs when the user makes a selection on the cbMappingOptions QComboBox.
   * @param groupType :: Value of selection made by user.
   */
  void IndirectConvertToEnergy::mappingOptionSelected(const QString& groupType)
  {
    if ( groupType == "File" )
    {
      m_uiForm.swMapping->setCurrentIndex(0);
    }
    else if ( groupType == "Groups" )
    {
      m_uiForm.swMapping->setCurrentIndex(1);
    }
    else if ( groupType == "All" || groupType == "Individual" || groupType == "Default" )
    {
      m_uiForm.swMapping->setCurrentIndex(2);
    }
  }

  /**
   * This function is called when the user clicks on the Background Removal button. It
   * displays the Background Removal dialog, initialising it if it hasn't been already.
   */
  void IndirectConvertToEnergy::backgroundClicked()
  {
    if(!m_backgroundDialog)
    {
      m_backgroundDialog = new Background(m_parentWidget);
      connect(m_backgroundDialog, SIGNAL(accepted()), this, SLOT(backgroundRemoval()));
      connect(m_backgroundDialog, SIGNAL(rejected()), this, SLOT(backgroundRemoval()));
    }
    m_backgroundDialog->show();
  }

  /**
   * Slot called when m_backgroundDialog is closed. Assesses whether user desires background removal.
   * Can be called before m_backgroundDialog even exists, for the purposes of setting the button to
   * it's initial (default) value.
   */
  void IndirectConvertToEnergy::backgroundRemoval()
  {
    if(m_backgroundDialog != NULL)
      m_bgRemoval = m_backgroundDialog->removeBackground();

    if(m_bgRemoval)
      m_uiForm.pbBackgroundRemoval->setText("Background Removal (On)");
    else
      m_uiForm.pbBackgroundRemoval->setText("Background Removal (Off)");
  }

  /**
   * This function creates the mapping/grouping file for the data analysis.
   * @param groupType :: Type of grouping (All, Group, Indiviual)
   * @return path to mapping file, or an empty string if file could not be created.
   */
  QString IndirectConvertToEnergy::createMapFile(const QString& groupType)
  {
    QString specRange = m_uiForm.spSpectraMin->text() + "," + m_uiForm.spSpectraMax->text();

    if(groupType == "File")
    {
      QString groupFile = m_uiForm.dsMapFile->getFirstFilename();
      if(groupFile == "")
      {
        emit showMessageBox("You must enter a path to the .map file.");
      }
      return groupFile;
    }
    else if(groupType == "Groups")
    {
      QString groupWS = "__Grouping";

      IAlgorithm_sptr groupingAlg = AlgorithmManager::Instance().create("CreateGroupingWorkspace");
      groupingAlg->initialize();

      groupingAlg->setProperty("FixedGroupCount", m_uiForm.leNoGroups->text().toInt());
      groupingAlg->setProperty("InstrumentName", getInstrumentConfiguration()->getInstrumentName().toStdString());
      groupingAlg->setProperty("ComponentName", getInstrumentConfiguration()->getAnalyserName().toStdString());
      groupingAlg->setProperty("OutputWorkspace", groupWS.toStdString());

      m_batchAlgoRunner->addAlgorithm(groupingAlg);

      return groupWS;
    }
    else
    {
      // Catch All and Individual
      return groupType;
    }
  }

  /**
   * Converts the checkbox selection to a comma delimited list of save formats for the
   * InelasticIndirectReduction algorithm.
   *
   * @return A vector of save formats
   */
  std::vector<std::string> IndirectConvertToEnergy::getSaveFormats()
  {
    std::vector<std::string> fileFormats;

    if ( m_uiForm.ckSaveNexus->isChecked() )
      fileFormats.push_back("nxs");
    if ( m_uiForm.ckSaveSPE->isChecked() )
      fileFormats.push_back("spe");
    if ( m_uiForm.ckSaveNXSPE->isChecked() )
      fileFormats.push_back("nxspe");
    if ( m_uiForm.ckSaveASCII->isChecked() )
      fileFormats.push_back("ascii");
    if ( m_uiForm.ckSaveAclimax->isChecked() )
      fileFormats.push_back("aclimax");
    if ( m_uiForm.ckSaveDaveGrp->isChecked() )
      fileFormats.push_back("davegrp");

    return fileFormats;
  }

  /**
   * Plots raw time data from .raw file before any data conversion has been performed.
   */
  void IndirectConvertToEnergy::plotRaw()
  {
    using MantidQt::API::BatchAlgorithmRunner;

    if(!m_uiForm.dsRunFiles->isValid())
    {
      emit showMessageBox("You must select a run file.");
      return;
    }

    bool ok;
    QString spectraRange = QInputDialog::getText(0, "Insert Spectra Ranges", "Range: ", QLineEdit::Normal, m_uiForm.spSpectraMin->text() +"-"+ m_uiForm.spSpectraMax->text(), &ok);

    if(!ok || spectraRange.isEmpty())
      return;

    QStringList specList = spectraRange.split("-");
    if(specList.size() != 2)
    {
      emit showMessageBox("Invalid input. Must be of form <SpecMin>-<SpecMax>");
      return;
    }

    std::vector<int> detectorRange;
    detectorRange.push_back(specList[0].toInt());

    if(specList.size() == 1)
      detectorRange.push_back(specList[0].toInt() + 1);
    else
      detectorRange.push_back(specList[1].toInt() + 1);

    QString rawFile = m_uiForm.dsRunFiles->getFirstFilename();
    QFileInfo rawFileInfo(rawFile);
    std::string name = rawFileInfo.baseName().toStdString();

    IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->initialize();
    loadAlg->setProperty("Filename", rawFile.toStdString());
    loadAlg->setProperty("OutputWorkspace", name);
    loadAlg->setProperty("SpectrumMin", specList[0].toStdString());
    loadAlg->setProperty("SpectrumMax", specList[1].toStdString());
    m_batchAlgoRunner->addAlgorithm(loadAlg);

    BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromLoad;
    inputFromLoad["InputWorkspace"] = name;

    if(m_bgRemoval)
    {
      QPair<double, double> range = m_backgroundDialog->getRange();

      IAlgorithm_sptr calcBackAlg = AlgorithmManager::Instance().create("CalculateFlatBackground");
      calcBackAlg->initialize();
      calcBackAlg->setProperty("OutputWorkspace", name + "_bg");
      calcBackAlg->setProperty("Mode", "Mean");
      calcBackAlg->setProperty("StartX", range.first);
      calcBackAlg->setProperty("EndX", range.second);
      m_batchAlgoRunner->addAlgorithm(calcBackAlg, inputFromLoad);

      BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromCalcBG;
      inputFromCalcBG["InputWorkspace"] = name + "_bg";

      IAlgorithm_sptr groupAlg = AlgorithmManager::Instance().create("GroupDetectors");
      groupAlg->initialize();
      groupAlg->setProperty("OutputWorkspace", name + "_grp");
      groupAlg->setProperty("DetectorList", detectorRange);
      m_batchAlgoRunner->addAlgorithm(groupAlg, inputFromCalcBG);

      IAlgorithm_sptr rawGroupAlg = AlgorithmManager::Instance().create("GroupDetectors");
      rawGroupAlg->initialize();
      rawGroupAlg->setProperty("OutputWorkspace", name + "_grp_raw");
      rawGroupAlg->setProperty("DetectorList", detectorRange);
      m_batchAlgoRunner->addAlgorithm(rawGroupAlg, inputFromLoad);
    }
    else
    {
      IAlgorithm_sptr rawGroupAlg = AlgorithmManager::Instance().create("GroupDetectors");
      rawGroupAlg->initialize();
      rawGroupAlg->setProperty("OutputWorkspace", name + "_grp");
      rawGroupAlg->setProperty("DetectorList", detectorRange);
      m_batchAlgoRunner->addAlgorithm(rawGroupAlg, inputFromLoad);
    }

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(plotRawComplete(bool)));
    m_batchAlgoRunner->executeBatchAsync();
  }

  /**
   * Handles plotting the result of Plot Raw
   *
   * @param error Indicates if the algorithm chain failed
   */
  void IndirectConvertToEnergy::plotRawComplete(bool error)
  {
    disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(plotRawComplete(bool)));

    if(error)
      return;

    QString rawFile = m_uiForm.dsRunFiles->getFirstFilename();
    QFileInfo rawFileInfo(rawFile);
    std::string name = rawFileInfo.baseName().toStdString();

    std::string pyInput = "from mantidplot import plotSpectrum\nplotSpectrum('" + name + "_grp', 0)\n";
    m_pythonRunner.runPythonCode(QString::fromStdString(pyInput));
  }

  /**
   * Called when a user starts to type / edit the runs to load.
   */
  void IndirectConvertToEnergy::pbRunEditing()
  {
    emit updateRunButton(false, "Editing...", "Run numbers are curently being edited.");
  }

  /**
   * Called when the FileFinder starts finding the files.
   */
  void IndirectConvertToEnergy::pbRunFinding()
  {
    emit updateRunButton(false, "Finding files...", "Searchig for data files for the run numbers entered...");
    m_uiForm.dsRunFiles->setEnabled(false);
  }

  /**
   * Called when the FileFinder has finished finding the files.
   */
  void IndirectConvertToEnergy::pbRunFinished()
  {
    if(!m_uiForm.dsRunFiles->isValid())
    {
      emit updateRunButton(false, "Invalid Run(s)", "Cannot find data files for some of the run numbers enetered.");
    }
    else
    {
      emit updateRunButton();
    }

    m_uiForm.dsRunFiles->setEnabled(true);
  }

} // namespace CustomInterfaces
} // namespace Mantid
