#include "MantidQtCustomInterfaces/IndirectConvertToEnergy.h"

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
  IndirectConvertToEnergy::IndirectConvertToEnergy(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent), m_backgroundDialog(NULL), m_bgRemoval(false)
  {
    // Add validators to UI form
    m_uiForm.leScaleMultiplier->setValidator(m_valPosDbl);
    m_uiForm.leNoGroups->setValidator(m_valInt);
    m_uiForm.leDetailedBalance->setValidator(m_valPosDbl);

    m_uiForm.leSpectraMin->setValidator(m_valInt);
    m_uiForm.leSpectraMax->setValidator(m_valInt);

    m_uiForm.entryRebinLow->setValidator(m_valDbl);
    m_uiForm.entryRebinWidth->setValidator(m_valDbl);
    m_uiForm.entryRebinHigh->setValidator(m_valDbl);

    // SIGNAL/SLOT CONNECTIONS
    // Update instrument information when a new instrument config is selected
    connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(setInstrumentDefault()));
    // Shows required mapping option UI widgets when a new mapping option is selected from drop down
    connect(m_uiForm.cbMappingOptions, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(mappingOptionSelected(const QString&)));
    // Shows background removal dialog when user clicks Background Removal
    connect(m_uiForm.pbBack_2, SIGNAL(clicked()), this, SLOT(backgroundClicked()));
    // Plots raw input data when user clicks Plot Time
    connect(m_uiForm.pbPlotRaw, SIGNAL(clicked()), this, SLOT(plotRaw()));
    // Enables/disables rebin options when user toggles Do Not Rebin checkbox
    connect(m_uiForm.rebin_ckDNR, SIGNAL(toggled(bool)), this, SLOT(rebinEntryToggle(bool)));
    // Enables/disables detail balance option when user toggle Detailed Balance checkbox
    connect(m_uiForm.ckDetailedBalance, SIGNAL(toggled(bool)), this, SLOT(detailedBalanceCheck(bool)));
    // Enables/disables scale multiply option when user toggles Scale checkbox
    connect(m_uiForm.ckScaleMultiplier, SIGNAL(toggled(bool)), this, SLOT(scaleMultiplierCheck(bool)));
    connect(m_uiForm.ind_calibFile, SIGNAL(fileTextChanged(const QString &)), this, SLOT(calibFileChanged(const QString &)));
    // Enables/disables calibration file options when user toggles Use Calib File checkbox
    connect(m_uiForm.ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(useCalib(bool)));
    // Displays correct UI widgets for selected rebin type when changed via Rebin Steps drop down
    connect(m_uiForm.comboRebinType, SIGNAL(currentIndexChanged(int)), m_uiForm.swIndRebin, SLOT(setCurrentIndex(int)));
    // Shows message on run buton when user is inputting a run number
    connect(m_uiForm.ind_runFiles, SIGNAL(fileTextChanged(const QString &)), this, SLOT(pbRunEditing()));
    // Shows message on run button when Mantid is finding the file for a given run number
    connect(m_uiForm.ind_runFiles, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
    // Reverts run button back to normal when file finding has finished
    connect(m_uiForm.ind_runFiles, SIGNAL(fileFindingFinished()), this, SLOT(pbRunFinished()));
    // Perform validation when editing an option
    connect(m_uiForm.leDetailedBalance, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.leScaleMultiplier, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.leSpectraMin, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.leSpectraMax, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.entryRebinLow, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.entryRebinWidth, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.entryRebinHigh, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

    // Update UI widgets to show default values
    mappingOptionSelected(m_uiForm.cbMappingOptions->currentText());
    rebinEntryToggle(m_uiForm.rebin_ckDNR->isChecked());
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
    detailedBalanceCheck(m_uiForm.ckDetailedBalance->isChecked());
    scaleMultiplierCheck(m_uiForm.ckScaleMultiplier->isChecked());

    // Load the default instrument parameters
    setInstrumentDefault();
  }

  void IndirectConvertToEnergy::run()
  {
    using MantidQt::API::BatchAlgorithmRunner;

    IAlgorithm_sptr reductionAlg = AlgorithmManager::Instance().create("InelasticIndirectReduction", -1);
    reductionAlg->initialize();
    BatchAlgorithmRunner::AlgorithmRuntimeProps reductionRuntimeProps;

    reductionAlg->setProperty("Instrument", m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString());
    reductionAlg->setProperty("Analyser", m_uiForm.iicInstrumentConfiguration->getAnalyserName().toStdString());
    reductionAlg->setProperty("Reflection", m_uiForm.iicInstrumentConfiguration->getReflectionName().toStdString());

    QString files = m_uiForm.ind_runFiles->getFilenames().join(",");
    reductionAlg->setProperty("InputFiles", files.toStdString());

    reductionAlg->setProperty("SumFiles", m_uiForm.ckSumFiles->isChecked());
    reductionAlg->setProperty("LoadLogs", m_uiForm.ckLoadLogs->isChecked());

    // If using a calibration file, load it
    if(m_uiForm.ckUseCalib->isChecked())
    {
      QString calibFilename = m_uiForm.ind_calibFile->getFirstFilename();

      QFileInfo fi(calibFilename);
      std::string calibWorkspaceName = fi.baseName().toStdString();

      IAlgorithm_sptr calibLoadAlg = AlgorithmManager::Instance().create("LoadNexus", -1);
      calibLoadAlg->initialize();
      calibLoadAlg->setProperty("Filename", calibFilename.toStdString());
      calibLoadAlg->setProperty("OutputWorkspace", calibWorkspaceName);
      m_batchAlgoRunner->addAlgorithm(calibLoadAlg);

      reductionRuntimeProps["CalibrationWorkspace"] = calibWorkspaceName;
    }

    std::vector<long> detectorRange;
    detectorRange.push_back(m_uiForm.leSpectraMin->text().toInt());
    detectorRange.push_back(m_uiForm.leSpectraMax->text().toInt());
    reductionAlg->setProperty("DetectorRange", detectorRange);

    if(m_bgRemoval)
    {
      QPair<double,double> background = m_backgroundDialog->getRange();
      std::vector<double> backgroundRange;
      backgroundRange.push_back(background.first);
      backgroundRange.push_back(background.second);
      reductionAlg->setProperty("BackgroundRange", backgroundRange);
    }

    if(!m_uiForm.rebin_ckDNR->isChecked())
    {
      QString rebin;
      if(m_uiForm.comboRebinType->currentIndex() == 0)
        rebin = m_uiForm.entryRebinLow->text() + "," + m_uiForm.entryRebinWidth->text() + "," + m_uiForm.entryRebinHigh->text();
      else
        rebin = m_uiForm.entryRebinString->text();

      reductionAlg->setProperty("RebinString", rebin.toStdString());
    }

    if(m_uiForm.ckDetailedBalance->isChecked())
      reductionAlg->setProperty("DetailedBalance", m_uiForm.leDetailedBalance->text().toDouble());

    if(m_uiForm.ckScaleMultiplier->isChecked())
      reductionAlg->setProperty("ScaleFactor", m_uiForm.leScaleMultiplier->text().toDouble());

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
    switch(m_uiForm.ind_cbPlotOutput->currentIndex())
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

    m_batchAlgoRunner->addAlgorithm(reductionAlg, reductionRuntimeProps);
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

    // run files input
    if ( ! m_uiForm.ind_runFiles->isValid() )
    {
      valid = false;
    }

    // calib file input
    if ( m_uiForm.ckUseCalib->isChecked() && !m_uiForm.ind_calibFile->isValid() )
    {
      valid = false;
    }

    // mapping selection
    if (
        ( m_uiForm.cbMappingOptions->currentText() == "Groups" && m_uiForm.leNoGroups->text() == "" )
        ||
        ( m_uiForm.cbMappingOptions->currentText() == "File" && ! m_uiForm.ind_mapFile->isValid() )
       )
    {
      valid = false;
      m_uiForm.valNoGroups->setText("*");
    }
    else
    {
      m_uiForm.valNoGroups->setText("");
    }

    int dummyPos = 0;

    QString text = m_uiForm.leDetailedBalance->text();
    QValidator::State fieldState = m_uiForm.leDetailedBalance->validator()->validate(text, dummyPos);

    // detailed balance
    if ( m_uiForm.ckDetailedBalance->isChecked() && fieldState != QValidator::Acceptable )
    {
      valid = false;
      m_uiForm.valDetailedBalance->setText("*");
    }
    else
    {
      m_uiForm.valDetailedBalance->setText("");
    }

    int dummyPos2 = 0;

    // scale multiplier
    QString scaleMultiplierText = m_uiForm.leScaleMultiplier->text();
    QValidator::State fieldState2 = m_uiForm.leScaleMultiplier->validator()->validate(scaleMultiplierText, dummyPos2);

    if ( m_uiForm.ckScaleMultiplier->isChecked() && fieldState2 != QValidator::Acceptable )
    {
      valid = false;
      m_uiForm.valScaleMultiplier->setText("*");
    }
    else
    {
      m_uiForm.valScaleMultiplier->setText("");
    }

    // SpectraMin/SpectraMax
    const QString specMin = m_uiForm.leSpectraMin->text();
    const QString specMax = m_uiForm.leSpectraMax->text();

    if (specMin.isEmpty() || specMax.isEmpty() ||
        (specMin.toDouble() < 1 || specMax.toDouble() < 1) ||
        (specMin.toDouble() > specMax.toDouble()))
    {
      valid = false;
      m_uiForm.valSpectraMin->setText("*");
      m_uiForm.valSpectraMax->setText("*");
    }
    else
    {
      m_uiForm.valSpectraMin->setText("");
      m_uiForm.valSpectraMax->setText("");
    }

    if ( ! m_uiForm.rebin_ckDNR->isChecked() )
    {
      if ( m_uiForm.comboRebinType->currentIndex() == 0 )
      {
        if ( m_uiForm.entryRebinLow->text() == "" )
        {
          valid = false;
          m_uiForm.valELow->setText("*");
        }
        else
        {
          m_uiForm.valELow->setText("");
        }

        if ( m_uiForm.entryRebinWidth->text() == "" )
        {
          valid = false;
          m_uiForm.valEWidth->setText("*");
        }
        else
        {
          m_uiForm.valEWidth->setText("");
        }

        if ( m_uiForm.entryRebinHigh->text() == "" )
        {
          valid = false;
          m_uiForm.valEHigh->setText("*");
        }
        else
        {
          m_uiForm.valEHigh->setText("");
        }

        if ( m_uiForm.entryRebinLow->text().toDouble() > m_uiForm.entryRebinHigh->text().toDouble() )
        {
          valid = false;
          m_uiForm.valELow->setText("*");
          m_uiForm.valEHigh->setText("*");
        }
      }
      else
      {
        if ( m_uiForm.entryRebinString->text() == "" )
        {
          valid = false;
        }
      }
    }
    else
    {
      m_uiForm.valELow->setText("");
      m_uiForm.valEWidth->setText("");
      m_uiForm.valEHigh->setText("");
    }

    return valid;
  }


  /**
   * Called when the instrument has changed, used to update default values.
   */
  void IndirectConvertToEnergy::setInstrumentDefault()
  {
    m_uiForm.leSpectraMin->clear();
    m_uiForm.leSpectraMax->clear();
    m_uiForm.leEfixed->clear();

    std::map<QString, QString> instDetails = getInstrumentDetails();

    if(instDetails["spectra-min"].isEmpty() || instDetails["spectra-max"].isEmpty())
    {
      emit showMessageBox("Could not gather necessary data from parameter file.");
      return;
    }

    m_uiForm.leSpectraMin->setText(instDetails["spectra-min"]);
    m_uiForm.leSpectraMax->setText(instDetails["spectra-max"]);

    if(!instDetails["efixed-val"].isEmpty())
      m_uiForm.leEfixed->setText(instDetails["efixed-val"]);
    else
      m_uiForm.leEfixed->clear();

    // Default rebinning parameters can be set in instrument parameter file
    if(!instDetails["rebin-default"].isEmpty())
    {
      m_uiForm.entryRebinString->setText(instDetails["rebin-default"]);
      m_uiForm.rebin_ckDNR->setChecked(false);
      QStringList rbp = instDetails["rebin-default"].split(",", QString::SkipEmptyParts);
      if ( rbp.size() == 3 )
      {
        m_uiForm.entryRebinLow->setText(rbp[0]);
        m_uiForm.entryRebinWidth->setText(rbp[1]);
        m_uiForm.entryRebinHigh->setText(rbp[2]);
        m_uiForm.comboRebinType->setCurrentIndex(0);
      }
      else
      {
        m_uiForm.comboRebinType->setCurrentIndex(1);
      }
    }
    else
    {
      m_uiForm.rebin_ckDNR->setChecked(true);
      m_uiForm.entryRebinLow->setText("");
      m_uiForm.entryRebinWidth->setText("");
      m_uiForm.entryRebinHigh->setText("");
      m_uiForm.entryRebinString->setText("");
    }

    if(!instDetails["cm-1-convert-choice"].isEmpty())
    {
      bool defaultOptions = instDetails["cm-1-convert-choice"] == "true";
      m_uiForm.ckCm1Units->setChecked(defaultOptions);
    }

    if(!instDetails["save-ascii-choice"].isEmpty())
    {
      bool defaultOptions = instDetails["save-ascii-choice"] == "true";
      m_uiForm.save_ckAscii->setChecked(defaultOptions);
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
      m_uiForm.pbBack_2->setText("Background Removal (On)");
    else
      m_uiForm.pbBack_2->setText("Background Removal (Off)");
  }

  /**
   * This function will disable the necessary elements of the interface when the user selects "Do Not Rebin"
   * and enable them again when this is de-selected.
   *
   * @param state :: whether the "Do Not Rebin" checkbox is checked
   */
  void IndirectConvertToEnergy::rebinEntryToggle(bool state)
  {
    //Determine value for single rebin field
    QString val;
    if(state)
      val = " ";
    else
      val = "*";

    //Rebin mode selection
    m_uiForm.comboRebinType->setEnabled(!state);
    m_uiForm.labelRebinSteps->setEnabled(!state);

    //Single rebin text entry
    m_uiForm.labelRebinLow->setEnabled( !state );
    m_uiForm.labelRebinWidth->setEnabled( !state );
    m_uiForm.labelRebinHigh->setEnabled( !state );
    m_uiForm.entryRebinLow->setEnabled( !state );
    m_uiForm.entryRebinWidth->setEnabled( !state );
    m_uiForm.entryRebinHigh->setEnabled( !state );

    //Rebin required markers
    m_uiForm.valELow->setEnabled(!state);
    m_uiForm.valELow->setText(val);
    m_uiForm.valEWidth->setEnabled(!state);
    m_uiForm.valEWidth->setText(val);
    m_uiForm.valEHigh->setEnabled(!state);
    m_uiForm.valEHigh->setText(val);

    //Rebin string entry
    m_uiForm.entryRebinString->setEnabled(!state);
    m_uiForm.labelRebinString->setEnabled(!state);
  }

  /**
   * Disables/enables the relevant parts of the UI when user checks/unchecks the Detailed Balance
   * ckDetailedBalance checkbox.
   * @param state :: state of the checkbox
   */
  void IndirectConvertToEnergy::detailedBalanceCheck(bool state)
  {
    m_uiForm.leDetailedBalance->setEnabled(state);
    m_uiForm.lbDBKelvin->setEnabled(state);
  }

  /**
   * Disables/enables the relevant parts of the UI when user checks/unchecks the 'Multiplication Factor (Scale):'
   * ckScaleMultiplier checkbox.
   * @param state :: state of the checkbox
   */
  void IndirectConvertToEnergy::scaleMultiplierCheck(bool state)
  {
    m_uiForm.leScaleMultiplier->setEnabled(state);
  }

  /**
   * This function creates the mapping/grouping file for the data analysis.
   * @param groupType :: Type of grouping (All, Group, Indiviual)
   * @return path to mapping file, or an empty string if file could not be created.
   */
  QString IndirectConvertToEnergy::createMapFile(const QString& groupType)
  {
    QString specRange = m_uiForm.leSpectraMin->text() + "," + m_uiForm.leSpectraMax->text();

    if(groupType == "File")
    {
      QString groupFile = m_uiForm.ind_mapFile->getFirstFilename();
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
      groupingAlg->setProperty("InstrumentName", m_uiForm.iicInstrumentConfiguration->getInstrumentName().toStdString());
      groupingAlg->setProperty("ComponentName", m_uiForm.iicInstrumentConfiguration->getAnalyserName().toStdString());
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

    if ( m_uiForm.save_ckNexus->isChecked() )
      fileFormats.push_back("nxs");
    if ( m_uiForm.save_ckSPE->isChecked() )
      fileFormats.push_back("spe");
    if ( m_uiForm.save_ckNxSPE->isChecked() )
      fileFormats.push_back("nxspe");
    if ( m_uiForm.save_ckAscii->isChecked() )
      fileFormats.push_back("ascii");
    if ( m_uiForm.save_ckAclimax->isChecked() )
      fileFormats.push_back("aclimax");
    if ( m_uiForm.save_ckDaveGrp->isChecked() )
      fileFormats.push_back("davegrp");

    return fileFormats;
  }

  /**
   * Plots raw time data from .raw file before any data conversion has been performed.
   */
  void IndirectConvertToEnergy::plotRaw()
  {
    using MantidQt::API::BatchAlgorithmRunner;

    if(!m_uiForm.ind_runFiles->isValid())
    {
      emit showMessageBox("You must select a run file.");
      return;
    }

    bool ok;
    QString spectraRange = QInputDialog::getText(0, "Insert Spectra Ranges", "Range: ", QLineEdit::Normal, m_uiForm.leSpectraMin->text() +"-"+ m_uiForm.leSpectraMax->text(), &ok);

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

    QString rawFile = m_uiForm.ind_runFiles->getFirstFilename();
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

    QString rawFile = m_uiForm.ind_runFiles->getFirstFilename();
    QFileInfo rawFileInfo(rawFile);
    std::string name = rawFileInfo.baseName().toStdString();

    std::string pyInput = "from mantidplot import plotSpectrum\nplotSpectrum('" + name + "_grp', 0)\n";
    m_pythonRunner.runPythonCode(QString::fromStdString(pyInput));
  }

  void IndirectConvertToEnergy::useCalib(bool state)
  {
    m_uiForm.ind_calibFile->isOptional(!state);
    m_uiForm.ind_calibFile->setEnabled(state);
  }

  /**
   * Controls the ckUseCalib checkbox to automatically check it when a user inputs a file from clicking on 'browse'.
   * @param calib :: path to calib file
   */
  void IndirectConvertToEnergy::calibFileChanged(const QString & calib)
  {
    if(calib.isEmpty())
      m_uiForm.ckUseCalib->setChecked(false);
    else
      m_uiForm.ckUseCalib->setChecked(true);
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
    m_uiForm.ind_runFiles->setEnabled(false);
  }

  /**
   * Called when the FileFinder has finished finding the files.
   */
  void IndirectConvertToEnergy::pbRunFinished()
  {
    if(!m_uiForm.ind_runFiles->isValid())
    {
      emit updateRunButton(false, "Invalid Run(s)", "Cannot find data files for some of the run numbers enetered.");
    }
    else
    {
      emit updateRunButton();
    }

    m_uiForm.ind_runFiles->setEnabled(true);
  }

} // namespace CustomInterfaces
} // namespace Mantid
