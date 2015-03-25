#include "MantidQtCustomInterfaces/Indirect/ISISEnergyTransfer.h"

#include "MantidGeometry/IDTypes.h"
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

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
  ISISEnergyTransfer::ISISEnergyTransfer(IndirectDataReduction * idrUI, QWidget * parent) :
      IndirectDataReductionTab(idrUI, parent)
  {
    m_uiForm.setupUi(parent);

    // SIGNAL/SLOT CONNECTIONS
    // Update instrument information when a new instrument config is selected
    connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(setInstrumentDefault()));
    // Shows required mapping option UI widgets when a new mapping option is selected from drop down
    connect(m_uiForm.cbGroupingOptions, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(mappingOptionSelected(const QString&)));
    // Plots raw input data when user clicks Plot Time
    connect(m_uiForm.pbPlotTime, SIGNAL(clicked()), this, SLOT(plotRaw()));
    // Shows message on run button when user is inputting a run number
    connect(m_uiForm.dsRunFiles, SIGNAL(fileTextChanged(const QString &)), this, SLOT(pbRunEditing()));
    // Shows message on run button when Mantid is finding the file for a given run number
    connect(m_uiForm.dsRunFiles, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
    // Reverts run button back to normal when file finding has finished
    connect(m_uiForm.dsRunFiles, SIGNAL(fileFindingFinished()), this, SLOT(pbRunFinished()));

    // Re-validate when certain inputs are changed
    connect(m_uiForm.spRebinLow, SIGNAL(valueChanged(double)), this, SLOT(validate()));
    connect(m_uiForm.spRebinWidth, SIGNAL(valueChanged(double)), this, SLOT(validate()));
    connect(m_uiForm.spRebinHigh, SIGNAL(valueChanged(double)), this, SLOT(validate()));
    connect(m_uiForm.leRebinString, SIGNAL(textChanged(const QString &)), this, SLOT(validate()));

    // Update UI widgets to show default values
    mappingOptionSelected(m_uiForm.cbGroupingOptions->currentText());

    // Validate to remove invalid markers
    validateTab();
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ISISEnergyTransfer::~ISISEnergyTransfer()
  {
  }


  void ISISEnergyTransfer::setup()
  {
  }


  bool ISISEnergyTransfer::validate()
  {
    UserInputValidator uiv;

    // Run files input
    if(!m_uiForm.dsRunFiles->isValid())
      uiv.addErrorMessage("Run file range is invalid.");

    // Calibration file input
    if(m_uiForm.ckUseCalib->isChecked() && !m_uiForm.dsCalibrationFile->isValid())
      uiv.addErrorMessage("Calibration file/workspace is invalid.");

    // Mapping file
    if((m_uiForm.cbGroupingOptions->currentText() == "File") && (!m_uiForm.dsMapFile->isValid()))
      uiv.addErrorMessage("Mapping file is invalid.");

    // Rebinning
    if(!m_uiForm.ckDoNotRebin->isChecked())
    {
      if(m_uiForm.cbRebinType->currentText() == "Single")
      {
        bool rebinValid = !uiv.checkBins(m_uiForm.spRebinLow->value(), m_uiForm.spRebinWidth->value(), m_uiForm.spRebinHigh->value());
        m_uiForm.valRebinLow->setVisible(rebinValid);
        m_uiForm.valRebinWidth->setVisible(rebinValid);
        m_uiForm.valRebinHigh->setVisible(rebinValid);
      }
      else
      {
        uiv.checkFieldIsNotEmpty("Rebin string", m_uiForm.leRebinString, m_uiForm.valRebinString);
      }
    }
    else
    {
      m_uiForm.valRebinLow->setVisible(false);
      m_uiForm.valRebinWidth->setVisible(false);
      m_uiForm.valRebinHigh->setVisible(false);
      m_uiForm.valRebinString->setVisible(false);
    }

    return uiv.isAllInputValid();
  }


  void ISISEnergyTransfer::run()
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

    if(m_uiForm.ckBackgroundRemoval->isChecked())
    {
      std::vector<double> backgroundRange;
      backgroundRange.push_back(m_uiForm.spBackgroundStart->value());
      backgroundRange.push_back(m_uiForm.spBackgroundEnd->value());
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

    if(m_uiForm.cbGroupingOptions->currentText() != "Default")
    {
      QString grouping = createMapFile(m_uiForm.cbGroupingOptions->currentText());
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

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
    disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(plotRawComplete(bool)));
    m_batchAlgoRunner->executeBatchAsync();
  }


  /**
   * Handles completion of the algorithm.
   *
   * Sets result workspace for Python export and ungroups result WorkspaceGroup.
   *
   * @param error True if the algorithm was stopped due to error, false otherwise
   */
  void ISISEnergyTransfer::algorithmComplete(bool error)
  {
    disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

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


  /**
   * Called when the instrument has changed, used to update default values.
   */
  void ISISEnergyTransfer::setInstrumentDefault()
  {
    std::map<QString, QString> instDetails = getInstrumentDetails();

    // Set the search instrument for runs
    m_uiForm.dsRunFiles->setInstrumentOverride(instDetails["instrument"]);

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
      if(rbp.size() == 3)
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

    if(!instDetails["fold-frames-choice"].isEmpty())
    {
      bool defaultOptions = instDetails["fold-frames-choice"] == "true";
      m_uiForm.ckFold->setChecked(defaultOptions);
    }
  }

  /**
   * This function runs when the user makes a selection on the cbGroupingOptions QComboBox.
   * @param groupType :: Value of selection made by user.
   */
  void ISISEnergyTransfer::mappingOptionSelected(const QString& groupType)
  {
    if ( groupType == "File" )
    {
      m_uiForm.swGrouping->setCurrentIndex(0);
    }
    else if ( groupType == "Groups" )
    {
      m_uiForm.swGrouping->setCurrentIndex(1);
    }
    else if ( groupType == "All" || groupType == "Individual" || groupType == "Default" )
    {
      m_uiForm.swGrouping->setCurrentIndex(2);
    }
  }

  /**
   * This function creates the mapping/grouping file for the data analysis.
   * @param groupType :: Type of grouping (All, Group, Indiviual)
   * @return path to mapping file, or an empty string if file could not be created.
   */
  QString ISISEnergyTransfer::createMapFile(const QString& groupType)
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

      groupingAlg->setProperty("FixedGroupCount", m_uiForm.spNumberGroups->value());
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
  std::vector<std::string> ISISEnergyTransfer::getSaveFormats()
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
  void ISISEnergyTransfer::plotRaw()
  {
    using MantidQt::API::BatchAlgorithmRunner;

    if(!m_uiForm.dsRunFiles->isValid())
    {
      emit showMessageBox("You must select a run file.");
      return;
    }

    std::vector<int> detectorRange;
    detectorRange.push_back(m_uiForm.spPlotTimeSpecMin->value());
    detectorRange.push_back(m_uiForm.spPlotTimeSpecMax->value());

    QString rawFile = m_uiForm.dsRunFiles->getFirstFilename();
    QFileInfo rawFileInfo(rawFile);
    std::string name = rawFileInfo.baseName().toStdString();

    IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->initialize();
    loadAlg->setProperty("Filename", rawFile.toStdString());
    loadAlg->setProperty("OutputWorkspace", name);
    loadAlg->setProperty("SpectrumMin", static_cast<Mantid::specid_t>(detectorRange[0]));
    loadAlg->setProperty("SpectrumMax", static_cast<Mantid::specid_t>(detectorRange[1]));
    m_batchAlgoRunner->addAlgorithm(loadAlg);

    BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromLoad;
    inputFromLoad["InputWorkspace"] = name;

    if(m_uiForm.ckBackgroundRemoval->isChecked())
    {
      std::vector<double> range;
      range.push_back(m_uiForm.spBackgroundStart->value());
      range.push_back(m_uiForm.spBackgroundEnd->value());

      IAlgorithm_sptr calcBackAlg = AlgorithmManager::Instance().create("CalculateFlatBackground");
      calcBackAlg->initialize();
      calcBackAlg->setProperty("OutputWorkspace", name + "_bg");
      calcBackAlg->setProperty("Mode", "Mean");
      calcBackAlg->setProperty("StartX", range[0]);
      calcBackAlg->setProperty("EndX", range[1]);
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

    disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(plotRawComplete(bool)));
    m_batchAlgoRunner->executeBatchAsync();
  }

  /**
   * Handles plotting the result of Plot Raw
   *
   * @param error Indicates if the algorithm chain failed
   */
  void ISISEnergyTransfer::plotRawComplete(bool error)
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
  void ISISEnergyTransfer::pbRunEditing()
  {
    emit updateRunButton(false, "Editing...", "Run numbers are curently being edited.");
  }

  /**
   * Called when the FileFinder starts finding the files.
   */
  void ISISEnergyTransfer::pbRunFinding()
  {
    emit updateRunButton(false, "Finding files...", "Searchig for data files for the run numbers entered...");
    m_uiForm.dsRunFiles->setEnabled(false);
  }

  /**
   * Called when the FileFinder has finished finding the files.
   */
  void ISISEnergyTransfer::pbRunFinished()
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
