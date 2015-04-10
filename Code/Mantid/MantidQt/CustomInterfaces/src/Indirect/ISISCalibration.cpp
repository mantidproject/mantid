#include "MantidQtCustomInterfaces/Indirect/ISISCalibration.h"

#include "MantidKernel/Logger.h"

#include <QFileInfo>

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("ISISCalibration");
}

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ISISCalibration::ISISCalibration(IndirectDataReduction * idrUI, QWidget * parent) :
    IndirectDataReductionTab(idrUI, parent),
    m_lastCalPlotFilename("")
  {
    m_uiForm.setupUi(parent);

    DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();

    // CAL PROPERTY TREE
    m_propTrees["CalPropTree"] = new QtTreePropertyBrowser();
    m_propTrees["CalPropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);
    m_uiForm.propertiesCalibration->addWidget(m_propTrees["CalPropTree"]);

    // Cal Property Tree: Peak/Background
    m_properties["CalPeakMin"] = m_dblManager->addProperty("Peak Min");
    m_properties["CalPeakMax"] = m_dblManager->addProperty("Peak Max");
    m_properties["CalBackMin"] = m_dblManager->addProperty("Back Min");
    m_properties["CalBackMax"] = m_dblManager->addProperty("Back Max");

    m_propTrees["CalPropTree"]->addProperty(m_properties["CalPeakMin"]);
    m_propTrees["CalPropTree"]->addProperty(m_properties["CalPeakMax"]);
    m_propTrees["CalPropTree"]->addProperty(m_properties["CalBackMin"]);
    m_propTrees["CalPropTree"]->addProperty(m_properties["CalBackMax"]);

    // Cal plot range selectors
    auto calPeak = m_uiForm.ppCalibration->addRangeSelector("CalPeak");
    auto calBackground = m_uiForm.ppCalibration->addRangeSelector("CalBackground");
    calBackground->setColour(Qt::darkGreen); //Dark green to signify background range

    // RES PROPERTY TREE
    m_propTrees["ResPropTree"] = new QtTreePropertyBrowser();
    m_propTrees["ResPropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);
    m_uiForm.loResolutionOptions->addWidget(m_propTrees["ResPropTree"]);

    // Res Property Tree: Spectra Selection
    m_properties["ResSpecMin"] = m_dblManager->addProperty("Spectra Min");
    m_propTrees["ResPropTree"]->addProperty(m_properties["ResSpecMin"]);
    m_dblManager->setDecimals(m_properties["ResSpecMin"], 0);

    m_properties["ResSpecMax"] = m_dblManager->addProperty("Spectra Max");
    m_propTrees["ResPropTree"]->addProperty(m_properties["ResSpecMax"]);
    m_dblManager->setDecimals(m_properties["ResSpecMax"], 0);

    // Res Property Tree: Background Properties
    QtProperty* resBG = m_grpManager->addProperty("Background");
    m_propTrees["ResPropTree"]->addProperty(resBG);

    m_properties["ResStart"] = m_dblManager->addProperty("Start");
    resBG->addSubProperty(m_properties["ResStart"]);

    m_properties["ResEnd"] = m_dblManager->addProperty("End");
    resBG->addSubProperty(m_properties["ResEnd"]);

    // Res Property Tree: Rebinning
    const int NUM_DECIMALS = 3;
    QtProperty* resRB = m_grpManager->addProperty("Rebinning");
    m_propTrees["ResPropTree"]->addProperty(resRB);

    m_properties["ResELow"] = m_dblManager->addProperty("Low");
    m_dblManager->setDecimals(m_properties["ResELow"], NUM_DECIMALS);
    m_dblManager->setValue(m_properties["ResELow"], -0.2);
    resRB->addSubProperty(m_properties["ResELow"]);

    m_properties["ResEWidth"] = m_dblManager->addProperty("Width");
    m_dblManager->setDecimals(m_properties["ResEWidth"], NUM_DECIMALS);
    m_dblManager->setValue(m_properties["ResEWidth"], 0.002);
    m_dblManager->setMinimum(m_properties["ResEWidth"], 0.001);
    resRB->addSubProperty(m_properties["ResEWidth"]);

    m_properties["ResEHigh"] = m_dblManager->addProperty("High");
    m_dblManager->setDecimals(m_properties["ResEHigh"], NUM_DECIMALS);
    m_dblManager->setValue(m_properties["ResEHigh"], 0.2);
    resRB->addSubProperty(m_properties["ResEHigh"]);

    // Res plot range selectors
    // Create ResBackground first so ResPeak is drawn above it
    auto resBackground = m_uiForm.ppResolution->addRangeSelector("ResBackground");
    resBackground->setColour(Qt::darkGreen);
    auto resPeak = m_uiForm.ppResolution->addRangeSelector("ResPeak");
    resPeak->setInfoOnly(true);

    // SIGNAL/SLOT CONNECTIONS
    // Update instrument information when a new instrument config is selected
    connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(setDefaultInstDetails()));

    connect(resPeak, SIGNAL(rangeChanged(double, double)),
            resBackground, SLOT(setRange(double, double)));

    // Update property map when a range seclector is moved
    connect(calPeak, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(calPeak, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    connect(calBackground, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(calBackground, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    connect(resPeak, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(resPeak, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    connect(resBackground, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(resBackground, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));

    // Update range selctor positions when a value in the double manager changes
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(calUpdateRS(QtProperty*, double)));
    // Plot miniplots after a file has loaded
    connect(m_uiForm.leRunNo, SIGNAL(filesFound()), this, SLOT(calPlotRaw()));
    // Plot miniplots when the user clicks Plot Raw
    connect(m_uiForm.pbPlotRaw, SIGNAL(clicked()), this, SLOT(calPlotRaw()));
    // Toggle RES file options when user toggles Create RES File checkbox
    connect(m_uiForm.ckCreateResolution, SIGNAL(toggled(bool)), this, SLOT(resCheck(bool)));

    // Shows message on run buton when user is inputting a run number
    connect(m_uiForm.leRunNo, SIGNAL(fileTextChanged(const QString &)), this, SLOT(pbRunEditing()));
    // Shows message on run button when Mantid is finding the file for a given run number
    connect(m_uiForm.leRunNo, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
    // Reverts run button back to normal when file finding has finished
    connect(m_uiForm.leRunNo, SIGNAL(fileFindingFinished()), this, SLOT(pbRunFinished()));

    // Nudge resCheck to ensure res range selectors are only shown when Create RES file is checked
    resCheck(m_uiForm.ckCreateResolution->isChecked());

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ISISCalibration::~ISISCalibration()
  {
  }

  void ISISCalibration::setup()
  {
  }

  void ISISCalibration::run()
  {
    // Get properties
    QString firstFile = m_uiForm.leRunNo->getFirstFilename();
    QString filenames = m_uiForm.leRunNo->getFilenames().join(",");

    auto instDetails = getInstrumentDetails();
    QString instDetectorRange = instDetails["spectra-min"] + "," + instDetails["spectra-max"];

    QString peakRange = m_properties["CalPeakMin"]->valueText() + "," + m_properties["CalPeakMax"]->valueText();
    QString backgroundRange = m_properties["CalBackMin"]->valueText() + "," + m_properties["CalBackMax"]->valueText();

    QFileInfo firstFileInfo(firstFile);
    QString outputWorkspaceNameStem = firstFileInfo.baseName() + "_" + getInstrumentConfiguration()->getAnalyserName()
                                      + getInstrumentConfiguration()->getReflectionName();

    QString calibrationWsName = outputWorkspaceNameStem + "_calib";

    // Configure the calibration algorithm
    IAlgorithm_sptr calibrationAlg = AlgorithmManager::Instance().create("IndirectCalibration");
    calibrationAlg->initialize();

    calibrationAlg->setProperty("InputFiles", filenames.toStdString());
    calibrationAlg->setProperty("OutputWorkspace", calibrationWsName.toStdString());
    calibrationAlg->setProperty("DetectorRange", instDetectorRange.toStdString());
    calibrationAlg->setProperty("PeakRange", peakRange.toStdString());
    calibrationAlg->setProperty("BackgroundRange", backgroundRange.toStdString());
    calibrationAlg->setProperty("Plot", m_uiForm.ckPlot->isChecked());

    if(m_uiForm.ckScale->isChecked())
    {
      double scale = m_uiForm.spScale->value();
      calibrationAlg->setProperty("ScaleFactor", scale);
    }

    m_batchAlgoRunner->addAlgorithm(calibrationAlg);

    // Initially take the calibration workspace as the result
    m_pythonExportWsName = calibrationWsName.toStdString();

    // Properties for algorithms that use data from calibration as an input
    BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromCalProps;
    inputFromCalProps["InputWorkspace"] = calibrationWsName.toStdString();

    // Add save algorithm to queue if ticked
    if( m_uiForm.ckSave->isChecked() )
    {
      IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create("SaveNexusProcessed");
      saveAlg->initialize();
      saveAlg->setProperty("Filename", calibrationWsName.toStdString() + ".nxs");

      m_batchAlgoRunner->addAlgorithm(saveAlg, inputFromCalProps);
    }

    // Configure the resolution algorithm
    if(m_uiForm.ckCreateResolution->isChecked())
    {
      QString resolutionWsName = outputWorkspaceNameStem + "_res";

      QString resDetectorRange = QString::number(m_dblManager->value(m_properties["ResSpecMin"])) + ","
          + QString::number(m_dblManager->value(m_properties["ResSpecMax"]));

      QString rebinString = QString::number(m_dblManager->value(m_properties["ResELow"])) + "," +
        QString::number(m_dblManager->value(m_properties["ResEWidth"])) + "," +
        QString::number(m_dblManager->value(m_properties["ResEHigh"]));

      QString background = QString::number(m_dblManager->value(m_properties["ResStart"])) + ","
          + QString::number(m_dblManager->value(m_properties["ResEnd"]));

      bool smooth = m_uiForm.ckSmoothResolution->isChecked();
      bool save = m_uiForm.ckSave->isChecked();

      IAlgorithm_sptr resAlg = AlgorithmManager::Instance().create("IndirectResolution", -1);
      resAlg->initialize();

      resAlg->setProperty("InputFiles", filenames.toStdString());
      resAlg->setProperty("Instrument", getInstrumentConfiguration()->getInstrumentName().toStdString());
      resAlg->setProperty("Analyser", getInstrumentConfiguration()->getAnalyserName().toStdString());
      resAlg->setProperty("Reflection", getInstrumentConfiguration()->getReflectionName().toStdString());
      resAlg->setProperty("RebinParam", rebinString.toStdString());
      resAlg->setProperty("DetectorRange", resDetectorRange.toStdString());
      resAlg->setProperty("BackgroundRange", background.toStdString());
      resAlg->setProperty("Save", save);

      if(m_uiForm.ckResolutionScale->isChecked())
        resAlg->setProperty("ScaleFactor", m_uiForm.spScale->value());

      if(smooth)
      {
        resAlg->setProperty("OutputWorkspace", resolutionWsName.toStdString() + "_pre_smooth");
      }
      else
      {
        resAlg->setProperty("OutputWorkspace", resolutionWsName.toStdString());
        resAlg->setProperty("Plot", m_uiForm.ckPlot->isChecked());
      }

      m_batchAlgoRunner->addAlgorithm(resAlg);

      if(smooth)
      {
        IAlgorithm_sptr smoothAlg = AlgorithmManager::Instance().create("WienerSmooth");
        smoothAlg->initialize();
        smoothAlg->setProperty("OutputWorkspace", resolutionWsName.toStdString());

        BatchAlgorithmRunner::AlgorithmRuntimeProps smoothAlgInputProps;
        smoothAlgInputProps["InputWorkspace"] = resolutionWsName.toStdString() + "_pre_smooth";

        m_batchAlgoRunner->addAlgorithm(smoothAlg, smoothAlgInputProps);

        if(save)
        {
          IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create("SaveNexusProcessed");
          saveAlg->initialize();

          BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromSmoothProps;
          inputFromSmoothProps["InputWorkspace"] = calibrationWsName.toStdString();
          saveAlg->setProperty("Filename", resolutionWsName.toStdString() + ".nxs");

          m_batchAlgoRunner->addAlgorithm(saveAlg, inputFromSmoothProps);
        }
      }

      // When creating resolution file take the resolution workspace as the result
      m_pythonExportWsName = resolutionWsName.toStdString();
    }

    m_batchAlgoRunner->executeBatchAsync();
  }

  void ISISCalibration::algorithmComplete(bool error)
  {
    if(error)
      return;

    // Plot the smoothed workspace if required
    if(m_uiForm.ckSmoothResolution->isChecked() && m_uiForm.ckPlot->isChecked())
    {
      QStringList plotWorkspaces;
      plotWorkspaces << QString::fromStdString(m_pythonExportWsName)
                     << QString::fromStdString(m_pythonExportWsName) + "_pre_smooth";
      plotSpectrum(plotWorkspaces);
    }
  }

  bool ISISCalibration::validate()
  {
    MantidQt::CustomInterfaces::UserInputValidator uiv;

    uiv.checkMWRunFilesIsValid("Run", m_uiForm.leRunNo);

    auto peakRange = std::make_pair(m_dblManager->value(m_properties["CalPeakMin"]), m_dblManager->value(m_properties["CalPeakMax"]));
    auto backRange = std::make_pair(m_dblManager->value(m_properties["CalBackMin"]), m_dblManager->value(m_properties["CalBackMax"]));

    uiv.checkValidRange("Peak Range", peakRange);
    uiv.checkValidRange("Back Range", backRange);
    uiv.checkRangesDontOverlap(peakRange, backRange);

    if ( m_uiForm.ckCreateResolution->isChecked() )
    {
      auto backgroundRange = std::make_pair(m_dblManager->value(m_properties["ResStart"]), m_dblManager->value(m_properties["ResEnd"]));
      uiv.checkValidRange("Background", backgroundRange);

      double eLow   = m_dblManager->value(m_properties["ResELow"]);
      double eHigh  = m_dblManager->value(m_properties["ResEHigh"]);
      double eWidth = m_dblManager->value(m_properties["ResEWidth"]);

      uiv.checkBins(eLow, eWidth, eHigh);
    }

    QString error = uiv.generateErrorMessage();

    if(error != "")
      g_log.warning(error.toStdString());

    return (error == "");
  }

  /**
   * Sets default spectra, peak and background ranges.
   */
  void ISISCalibration::setDefaultInstDetails()
  {
    // Get spectra, peak and background details
    std::map<QString, QString> instDetails = getInstrumentDetails();

    // Set the search instrument for runs
    m_uiForm.leRunNo->setInstrumentOverride(instDetails["instrument"]);

    // Set spectra range
    m_dblManager->setValue(m_properties["ResSpecMin"], instDetails["spectra-min"].toDouble());
    m_dblManager->setValue(m_properties["ResSpecMax"], instDetails["spectra-max"].toDouble());

    // Set peak and background ranges
    std::map<std::string, double> ranges = getRangesFromInstrument();

    QPair<double, double> peakRange(ranges["peak-start-tof"], ranges["peak-end-tof"]);
    QPair<double, double> backgroundRange(ranges["back-start-tof"], ranges["back-end-tof"]);

    auto calPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
    auto calBackground = m_uiForm.ppCalibration->getRangeSelector("CalBackground");
    setRangeSelector(calPeak, m_properties["CalPeakMin"], m_properties["CalPeakMax"], peakRange);
    setRangeSelector(calBackground, m_properties["CalBackMin"], m_properties["CalBackMax"], backgroundRange);
  }

  /**
   * Replots the raw data mini plot and the energy mini plot
   */
  void ISISCalibration::calPlotRaw()
  {
    setDefaultInstDetails();

    QString filename = m_uiForm.leRunNo->getFirstFilename();

    // Don't do anything if the file we would plot has not changed
    if(filename == m_lastCalPlotFilename)
      return;

    m_lastCalPlotFilename = filename;

    if ( filename.isEmpty() )
    {
      emit showMessageBox("Cannot plot raw data without filename");
      return;
    }

    QFileInfo fi(filename);
    QString wsname = fi.baseName();

    auto instDetails = getInstrumentDetails();
    int specMin = instDetails["spectra-min"].toInt();
    int specMax = instDetails["spectra-max"].toInt();

    if(!loadFile(filename, wsname, specMin, specMax))
    {
      emit showMessageBox("Unable to load file.\nCheck whether your file exists and matches the selected instrument in the Energy Transfer tab.");
      return;
    }

    MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(wsname.toStdString()));

    const Mantid::MantidVec & dataX = input->readX(0);
    QPair<double, double> range(dataX.front(), dataX.back());

    m_uiForm.ppCalibration->clear();
    m_uiForm.ppCalibration->addSpectrum("Raw", input, 0);
    m_uiForm.ppCalibration->resizeX();

    auto calPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
    auto calBackground = m_uiForm.ppCalibration->getRangeSelector("CalBackground");
    setPlotPropertyRange(calPeak, m_properties["CalELow"], m_properties["CalEHigh"], range);
    setPlotPropertyRange(calBackground, m_properties["CalStart"], m_properties["CalEnd"], range);

    m_uiForm.ppCalibration->replot();

    //Also replot the energy
    calPlotEnergy();
  }

  /**
   * Replots the energy mini plot
   */
  void ISISCalibration::calPlotEnergy()
  {
    if ( ! m_uiForm.leRunNo->isValid() )
    {
      emit showMessageBox("Run number not valid.");
      return;
    }

    QString files = m_uiForm.leRunNo->getFilenames().join(",");

    QFileInfo fi(m_uiForm.leRunNo->getFirstFilename());

    QString detRange = QString::number(m_dblManager->value(m_properties["ResSpecMin"])) + ","
                     + QString::number(m_dblManager->value(m_properties["ResSpecMax"]));

    IAlgorithm_sptr reductionAlg = AlgorithmManager::Instance().create("InelasticIndirectReduction");
    reductionAlg->initialize();
    reductionAlg->setProperty("Instrument", getInstrumentConfiguration()->getInstrumentName().toStdString());
    reductionAlg->setProperty("Analyser", getInstrumentConfiguration()->getAnalyserName().toStdString());
    reductionAlg->setProperty("Reflection", getInstrumentConfiguration()->getReflectionName().toStdString());
    reductionAlg->setProperty("InputFiles", files.toStdString());
    reductionAlg->setProperty("OutputWorkspace", "__IndirectCalibration_reduction");
    reductionAlg->setProperty("DetectorRange", detRange.toStdString());
    reductionAlg->execute();

    if(!reductionAlg->isExecuted())
    {
      g_log.warning("Could not generate energy preview plot.");
      return;
    }

    WorkspaceGroup_sptr reductionOutputGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("__IndirectCalibration_reduction");
    if(reductionOutputGroup->size() == 0)
    {
      g_log.warning("No result workspaces, cannot plot energy preview.");
      return;
    }

    MatrixWorkspace_sptr energyWs = boost::dynamic_pointer_cast<MatrixWorkspace>(reductionOutputGroup->getItem(0));
    if(!energyWs)
    {
      g_log.warning("No result workspaces, cannot plot energy preview.");
      return;
    }

    const Mantid::MantidVec & dataX = energyWs->readX(0);
    QPair<double, double> range(dataX.front(), dataX.back());

    auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");
    setPlotPropertyRange(resBackground, m_properties["ResStart"], m_properties["ResEnd"], range);

    m_uiForm.ppResolution->clear();
    m_uiForm.ppResolution->addSpectrum("Energy", energyWs, 0);
    m_uiForm.ppResolution->resizeX();

    calSetDefaultResolution(energyWs);

    m_uiForm.ppResolution->replot();
  }

  /**
   * Set default background and rebinning properties for a given instument
   * and analyser
   *
   * @param ws :: Mantid workspace containing the loaded instument
   */
  void ISISCalibration::calSetDefaultResolution(MatrixWorkspace_const_sptr ws)
  {
    auto inst = ws->getInstrument();
    auto analyser = inst->getStringParameter("analyser");

    if(analyser.size() > 0)
    {
      auto comp = inst->getComponentByName(analyser[0]);

      if(!comp)
        return;

      auto params = comp->getNumberParameter("resolution", true);

      //Set the default instrument resolution
      if(params.size() > 0)
      {
        double res = params[0];

        // Set default rebinning bounds
        QPair<double, double> peakRange(-res*10, res*10);
        auto resPeak = m_uiForm.ppResolution->getRangeSelector("ResPeak");
        setRangeSelector(resPeak, m_properties["ResELow"], m_properties["ResEHigh"], peakRange);

        // Set default background bounds
        QPair<double, double> backgroundRange(-res*9, -res*8);
        auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");
        setRangeSelector(resBackground, m_properties["ResStart"], m_properties["ResEnd"], backgroundRange);
      }
    }
  }

  /**
   * Handles a range selector having it's minumum value changed.
   * Updates property in property map.
   *
   * @param val :: New minumum value
   */
  void ISISCalibration::calMinChanged(double val)
  {
    auto calPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
    auto calBackground = m_uiForm.ppCalibration->getRangeSelector("CalBackground");
    auto resPeak = m_uiForm.ppResolution->getRangeSelector("ResPeak");
    auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");

    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());

    if(from == calPeak)
    {
      m_dblManager->setValue(m_properties["CalPeakMin"], val);
    }
    else if(from == calBackground)
    {
      m_dblManager->setValue(m_properties["CalBackMin"], val);
    }
    else if(from == resPeak)
    {
      m_dblManager->setValue(m_properties["ResELow"], val);
    }
    else if(from == resBackground)
    {
      m_dblManager->setValue(m_properties["ResStart"], val);
    }
  }

  /**
   * Handles a range selector having it's maxumum value changed.
   * Updates property in property map.
   *
   * @param val :: New maxumum value
   */
  void ISISCalibration::calMaxChanged(double val)
  {
    auto calPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
    auto calBackground = m_uiForm.ppCalibration->getRangeSelector("CalBackground");
    auto resPeak = m_uiForm.ppResolution->getRangeSelector("ResPeak");
    auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");

    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());

    if(from == calPeak)
    {
      m_dblManager->setValue(m_properties["CalPeakMax"], val);
    }
    else if(from == calBackground)
    {
      m_dblManager->setValue(m_properties["CalBackMax"], val);
    }
    else if(from == resPeak)
    {
      m_dblManager->setValue(m_properties["ResEHigh"], val);
    }
    else if(from == resBackground)
    {
      m_dblManager->setValue(m_properties["ResEnd"], val);
    }
  }

  /**
   * Update a range selector given a QtProperty and new value
   *
   * @param prop :: The property to update
   * @param val :: New value for property
   */
  void ISISCalibration::calUpdateRS(QtProperty* prop, double val)
  {
    auto calPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
    auto calBackground = m_uiForm.ppCalibration->getRangeSelector("CalBackground");
    auto resPeak = m_uiForm.ppResolution->getRangeSelector("ResPeak");
    auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");

    if ( prop == m_properties["CalPeakMin"] )       calPeak->setMinimum(val);
    else if ( prop == m_properties["CalPeakMax"] )  calPeak->setMaximum(val);
    else if ( prop == m_properties["CalBackMin"] )  calBackground->setMinimum(val);
    else if ( prop == m_properties["CalBackMax"] )  calBackground->setMaximum(val);
    else if ( prop == m_properties["ResStart"] )    resBackground->setMinimum(val);
    else if ( prop == m_properties["ResEnd"] )      resBackground->setMaximum(val);
    else if ( prop == m_properties["ResELow"] )     resPeak->setMinimum(val);
    else if ( prop == m_properties["ResEHigh"] )    resPeak->setMaximum(val);
  }

  /**
  * This function enables/disables the display of the options involved in creating the RES file.
  *
  * @param state :: whether checkbox is checked or unchecked
  */
  void ISISCalibration::resCheck(bool state)
  {
    m_uiForm.ppResolution->getRangeSelector("ResPeak")->setVisible(state);
    m_uiForm.ppResolution->getRangeSelector("ResBackground")->setVisible(state);

    // Toggle scale and smooth options
    m_uiForm.ckResolutionScale->setEnabled(state);
    m_uiForm.ckSmoothResolution->setEnabled(state);
  }

  /**
   * Called when a user starts to type / edit the runs to load.
   */
  void ISISCalibration::pbRunEditing()
  {
    emit updateRunButton(false, "Editing...", "Run numbers are curently being edited.");
  }

  /**
   * Called when the FileFinder starts finding the files.
   */
  void ISISCalibration::pbRunFinding()
  {
    emit updateRunButton(false, "Finding files...", "Searchig for data files for the run numbers entered...");
    m_uiForm.leRunNo->setEnabled(false);
  }

  /**
   * Called when the FileFinder has finished finding the files.
   */
  void ISISCalibration::pbRunFinished()
  {
    if(!m_uiForm.leRunNo->isValid())
    {
      emit updateRunButton(false, "Invalid Run(s)", "Cannot find data files for some of the run numbers enetered.");
    }
    else
    {
      emit updateRunButton();
    }

    m_uiForm.leRunNo->setEnabled(true);
  }

} // namespace CustomInterfaces
} // namespace Mantid
