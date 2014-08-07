#include "MantidQtCustomInterfaces/IndirectCalibration.h"

#include "MantidKernel/Logger.h"

#include <QFileInfo>

namespace
{
  Mantid::Kernel::Logger g_log("IndirectCalibration");
}

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectCalibration::IndirectCalibration(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
    DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();

    // CAL PROPERTY TREE
    m_propTrees["CalPropTree"] = new QtTreePropertyBrowser();
    m_propTrees["CalPropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);
    m_uiForm.cal_treeCal->addWidget(m_propTrees["CalPropTree"]);

    // Cal Property Tree: Peak/Background
    m_properties["CalPeakMin"] = m_dblManager->addProperty("Peak Min");
    m_properties["CalPeakMax"] = m_dblManager->addProperty("Peak Max");
    m_properties["CalBackMin"] = m_dblManager->addProperty("Back Min");
    m_properties["CalBackMax"] = m_dblManager->addProperty("Back Max");

    m_propTrees["CalPropTree"]->addProperty(m_properties["CalPeakMin"]);
    m_propTrees["CalPropTree"]->addProperty(m_properties["CalPeakMax"]);
    m_propTrees["CalPropTree"]->addProperty(m_properties["CalBackMin"]);
    m_propTrees["CalPropTree"]->addProperty(m_properties["CalBackMax"]);

    // CAL PLOT
    m_plots["CalPlot"] = new QwtPlot(m_parentWidget);
    m_plots["CalPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["CalPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["CalPlot"]->setCanvasBackground(Qt::white);
    m_uiForm.cal_plotCal->addWidget(m_plots["CalPlot"]);

    // Cal plot range selectors
    m_rangeSelectors["CalPeak"] = new MantidWidgets::RangeSelector(m_plots["CalPlot"]);
    m_rangeSelectors["CalBackground"] = new MantidWidgets::RangeSelector(m_plots["CalPlot"]);
    m_rangeSelectors["CalBackground"]->setColour(Qt::darkGreen); //Dark green to signify background range

    // RES PROPERTY TREE
    m_propTrees["ResPropTree"] = new QtTreePropertyBrowser();
    m_propTrees["ResPropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);
    m_uiForm.cal_treeRes->addWidget(m_propTrees["ResPropTree"]);

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

    // RES PLOT
    m_plots["ResPlot"] = new QwtPlot(m_parentWidget);
    m_plots["ResPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["ResPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["ResPlot"]->setCanvasBackground(Qt::white);
    m_uiForm.cal_plotRes->addWidget(m_plots["ResPlot"]);

    // Res plot range selectors
    // Create ResBackground first so ResPeak is drawn above it
    m_rangeSelectors["ResBackground"] = new MantidWidgets::RangeSelector(m_plots["ResPlot"],
        MantidQt::MantidWidgets::RangeSelector::XMINMAX, true, false);
    m_rangeSelectors["ResBackground"]->setColour(Qt::darkGreen);
    m_rangeSelectors["ResPeak"] = new MantidWidgets::RangeSelector(m_plots["ResPlot"],
        MantidQt::MantidWidgets::RangeSelector::XMINMAX, true, true);
    
    // MISC UI
    m_uiForm.cal_leIntensityScaleMultiplier->setValidator(m_valDbl);
    m_uiForm.cal_leResScale->setValidator(m_valDbl);
    m_uiForm.cal_valIntensityScaleMultiplier->setText(" ");

    // SIGNAL/SLOT CONNECTIONS
    connect(m_rangeSelectors["ResPeak"], SIGNAL(rangeChanged(double, double)), m_rangeSelectors["ResBackground"], SLOT(setRange(double, double)));

    // Update property map when a range seclector is moved
    connect(m_rangeSelectors["CalPeak"], SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(m_rangeSelectors["CalPeak"], SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    connect(m_rangeSelectors["CalBackground"], SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(m_rangeSelectors["CalBackground"], SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    connect(m_rangeSelectors["ResPeak"], SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(m_rangeSelectors["ResPeak"], SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    connect(m_rangeSelectors["ResBackground"], SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(m_rangeSelectors["ResBackground"], SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    // Update range selctor positions when a value in the double manager changes
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(calUpdateRS(QtProperty*, double)));
    // Plot miniplots after a file has loaded
    connect(m_uiForm.cal_leRunNo, SIGNAL(filesFound()), this, SLOT(calPlotRaw()));
    // Plot miniplots when the user clicks Plot Raw
    connect(m_uiForm.cal_pbPlot, SIGNAL(clicked()), this, SLOT(calPlotRaw()));
    // Toggle RES file options when user toggles Create RES File checkbox
    connect(m_uiForm.cal_ckRES, SIGNAL(toggled(bool)), this, SLOT(resCheck(bool)));
    // Toggle RES range selector when user toggles Create RES File checkbox
    connect(m_uiForm.cal_ckRES, SIGNAL(toggled(bool)), m_uiForm.cal_ckResScale, SLOT(setEnabled(bool)));
    // Enable/disable RES scaling option when user toggles Scale RES checkbox
    connect(m_uiForm.cal_ckResScale, SIGNAL(toggled(bool)), m_uiForm.cal_leResScale, SLOT(setEnabled(bool)));
    // Enable/dosable scale factor option when user toggles Intensity Scale Factor checkbox
    connect(m_uiForm.cal_ckIntensityScaleMultiplier, SIGNAL(toggled(bool)), this, SLOT(intensityScaleMultiplierCheck(bool)));
    // Validate the value entered in scale factor option whenever it changes
    connect(m_uiForm.cal_leIntensityScaleMultiplier, SIGNAL(textChanged(const QString &)), this, SLOT(calibValidateIntensity(const QString &)));

    // Nudge resCheck to ensure res range selectors are only shown when Create RES file is checked
    resCheck(m_uiForm.cal_ckRES->isChecked());

    // Set default values
    setDefaultInstDetails();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectCalibration::~IndirectCalibration()
  {
  }
  
  void IndirectCalibration::setup()
  {
  }

  void IndirectCalibration::run()
  {
    QString file = m_uiForm.cal_leRunNo->getFirstFilename();
    QString filenames = "[r'"+m_uiForm.cal_leRunNo->getFilenames().join("', r'")+"']";

    QString reducer = "from mantid.simpleapi import SaveNexus\n"
      "from inelastic_indirect_reduction_steps import CreateCalibrationWorkspace\n"
      "calib = CreateCalibrationWorkspace()\n"
      "calib.set_files(" + filenames + ")\n"
      "calib.set_detector_range(" + m_uiForm.leSpectraMin->text() + "-1, " + m_uiForm.leSpectraMax->text() + "-1)\n"
      "calib.set_parameters(" + m_properties["CalBackMin"]->valueText() + "," 
      + m_properties["CalBackMax"]->valueText() + ","
      + m_properties["CalPeakMin"]->valueText() + ","
      + m_properties["CalPeakMax"]->valueText() + ")\n"
      "calib.set_analyser('" + m_uiForm.cbAnalyser->currentText() + "')\n"
      "calib.set_reflection('" + m_uiForm.cbReflection->currentText() + "')\n";

    //Scale values by arbitrary scalar if requested
    if(m_uiForm.cal_ckIntensityScaleMultiplier->isChecked())
    {
      QString scale = m_uiForm.cal_leIntensityScaleMultiplier->text(); 
      if(scale.isEmpty())
      {
          scale = "1.0";
        }
        reducer += "calib.set_intensity_scale("+scale+")\n";
      }

      reducer += "calib.execute(None, None)\n"
        "result = calib.result_workspace()\n"
        "print result\n";

      if( m_uiForm.cal_ckSave->isChecked() )
      {
        reducer +=
          "SaveNexus(InputWorkspace=result, Filename=result+'.nxs')\n";
      }

      if ( m_uiForm.cal_ckPlotResult->isChecked() )
      {
        reducer += "from mantidplot import plotTimeBin\n"
          "plotTimeBin(result, 0)\n";
      }

      QString pyOutput = m_pythonRunner.runPythonCode(reducer).trimmed();

      if ( pyOutput == "" )
      {
        emit showMessageBox("An error occurred creating the calib file.\n");
      }
      else
      {
        if ( m_uiForm.cal_ckRES->isChecked() )
        {
          QString fnames2 = m_uiForm.cal_leRunNo->getFilenames().join(",");
          createRESfile(fnames2);
        }
        m_uiForm.ind_calibFile->setFileTextWithSearch(pyOutput + ".nxs");
        m_uiForm.ckUseCalib->setChecked(true);
      }
  }

  bool IndirectCalibration::validate()
  {
    MantidQt::CustomInterfaces::UserInputValidator uiv;

    uiv.checkMWRunFilesIsValid("Run", m_uiForm.cal_leRunNo);

    auto peakRange = std::make_pair(m_dblManager->value(m_properties["CalPeakMin"]), m_dblManager->value(m_properties["CalPeakMax"]));
    auto backRange = std::make_pair(m_dblManager->value(m_properties["CalBackMin"]), m_dblManager->value(m_properties["CalBackMax"]));

    uiv.checkValidRange("Peak Range", peakRange);
    uiv.checkValidRange("Back Range", backRange);
    uiv.checkRangesDontOverlap(peakRange, backRange);

    if ( m_uiForm.cal_ckRES->isChecked() )
    {
      auto backgroundRange = std::make_pair(m_dblManager->value(m_properties["ResStart"]), m_dblManager->value(m_properties["ResEnd"]));
      uiv.checkValidRange("Background", backgroundRange);

      double eLow   = m_dblManager->value(m_properties["ResELow"]);
      double eHigh  = m_dblManager->value(m_properties["ResEHigh"]);
      double eWidth = m_dblManager->value(m_properties["ResEWidth"]);

      uiv.checkBins(eLow, eWidth, eHigh);
    }

    if( m_uiForm.cal_ckIntensityScaleMultiplier->isChecked()
        && m_uiForm.cal_leIntensityScaleMultiplier->text().isEmpty() )
    {
      uiv.addErrorMessage("You must enter a scale for the calibration file");
    }

    if( m_uiForm.cal_ckResScale->isChecked() && m_uiForm.cal_leResScale->text().isEmpty() )
    {
      uiv.addErrorMessage("You must enter a scale for the resolution file");
    }

    QString error = uiv.generateErrorMessage();

    if(error != "")
      g_log.warning(error.toStdString());

    return (error == "");
  }

  /**
   * Sets default spectra, peak and background ranges
   */
  void IndirectCalibration::setDefaultInstDetails()
  {
    //Get spectra, peak and background details
    std::map<QString, QString> instDetails = getInstrumentDetails();

    //Set spectra range
    m_dblManager->setValue(m_properties["ResSpecMin"], instDetails["SpectraMin"].toDouble());
    m_dblManager->setValue(m_properties["ResSpecMax"], instDetails["SpectraMax"].toDouble());

    //Set pean and background ranges
    if(instDetails.size() >= 8)
    {
      setMiniPlotGuides("CalPeak", m_properties["CalPeakMin"], m_properties["CalPeakMax"],
          std::pair<double, double>(instDetails["PeakMin"].toDouble(), instDetails["PeakMax"].toDouble()));
      setMiniPlotGuides("CalBackground", m_properties["CalBackMin"], m_properties["CalBackMax"],
          std::pair<double, double>(instDetails["BackMin"].toDouble(), instDetails["BackMax"].toDouble()));
    }
  }

  /**
   * Replots the raw data mini plot and the energy mini plot
   */
  void IndirectCalibration::calPlotRaw()
  {
    setDefaultInstDetails();

    QString filename = m_uiForm.cal_leRunNo->getFirstFilename();

    if ( filename.isEmpty() )
    {
      emit showMessageBox("Cannot plot raw data without filename");
      return;
    }

    QFileInfo fi(filename);
    QString wsname = fi.baseName();
    QString pyInput = "Load(Filename=r'" + filename + "', OutputWorkspace='" + wsname + "', SpectrumMin="
      + m_uiForm.leSpectraMin->text() + ", SpectrumMax="
      + m_uiForm.leSpectraMax->text() + ")\n";

    pyInput = "try:\n  " +
      pyInput +
      "except ValueError as ve:" +
      "  print str(ve)";

    QString pyOutput = m_pythonRunner.runPythonCode(pyInput);

    if(!pyOutput.isEmpty())
    {
      emit showMessageBox("Unable to load file. Error: \n\n" + pyOutput + "\nCheck whether your file exists and matches the selected instrument in the Energy Transfer tab.");
      return;
    }

    Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

    const Mantid::MantidVec & dataX = input->readX(0);
    std::pair<double, double> range(dataX.front(), dataX.back());

    plotMiniPlot(input, 0, "CalPlot", "CalCurve");
    setXAxisToCurve("CalPlot", "CalCurve");

    setPlotRange("CalPeak", m_properties["CalELow"], m_properties["CalEHigh"], range);
    setPlotRange("CalBackground", m_properties["CalStart"], m_properties["CalEnd"], range);

    replot("CalPlot");

    //Also replot the energy
    calPlotEnergy();
  }

  /**
   * Replots the energy mini plot
   */
  void IndirectCalibration::calPlotEnergy()
  {
    if ( ! m_uiForm.cal_leRunNo->isValid() )
    {
      emit showMessageBox("Run number not valid.");
      return;
    }

    QString files = m_uiForm.cal_leRunNo->getFilenames().join(",");

    QFileInfo fi(m_uiForm.cal_leRunNo->getFirstFilename());
    QString outWS = fi.baseName() + "_" + m_uiForm.cbAnalyser->currentText() +
        m_uiForm.cbReflection->currentText() + "_red";
    /* outWS = outWS.toLower(); */

    QString detRange = QString::number(m_dblManager->value(m_properties["ResSpecMin"])) + ","
        + QString::number(m_dblManager->value(m_properties["ResSpecMax"]));

    Mantid::API::IAlgorithm_sptr resAlg = Mantid::API::AlgorithmManager::Instance().create("IndirectResolution", -1);
    resAlg->initialize();

    resAlg->setProperty("InputFiles", files.toStdString());
    resAlg->setProperty("OutputWorkspace", outWS.toStdString());
    resAlg->setProperty("Instrument", m_uiForm.cbInst->currentText().toStdString());
    resAlg->setProperty("Analyser", m_uiForm.cbAnalyser->currentText().toStdString());
    resAlg->setProperty("Reflection", m_uiForm.cbReflection->currentText().toStdString());
    resAlg->setProperty("Res", false);
    resAlg->setProperty("DetectorRange", detRange.toStdString());

    resAlg->execute();

    if(!resAlg->isExecuted())
    {
      emit showMessageBox("Failed to convert to energy. See log for details.");
      return;
    }

    Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(outWS.toStdString()));

    const Mantid::MantidVec & dataX = input->readX(0);
    std::pair<double, double> range(dataX.front(), dataX.back());

    setPlotRange("ResBackground", m_properties["ResStart"], m_properties["ResEnd"], range);

    plotMiniPlot(input, 0, "ResPlot", "ResCurve");
    setXAxisToCurve("ResPlot", "ResCurve");

    calSetDefaultResolution(input);

    replot("ResPlot");
  }

  /**
   * Set default background and rebinning properties for a given instument
   * and analyser
   *
   * @param ws :: Mantid workspace containing the loaded instument
   */
  void IndirectCalibration::calSetDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws)
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

        //Set default rebinning bounds
        std::pair<double, double> peakRange(-res*10, res*10);
        setMiniPlotGuides("ResPeak", m_properties["ResELow"], m_properties["ResEHigh"], peakRange);

        //Set default background bounds
        std::pair<double, double> backgroundRange(-res*9, -res*8);
        setMiniPlotGuides("ResBackground", m_properties["ResStart"], m_properties["ResEnd"], backgroundRange);
      }
    }
  }

  /**
   * Handles a range selector having it's minumum value changed.
   * Updates property in property map.
   *
   * @param val :: New minumum value
   */
  void IndirectCalibration::calMinChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_rangeSelectors["CalPeak"] )
    {
      m_dblManager->setValue(m_properties["CalPeakMin"], val);
    }
    else if ( from == m_rangeSelectors["CalBackground"] )
    {
      m_dblManager->setValue(m_properties["CalBackMin"], val);
    }
    else if ( from == m_rangeSelectors["ResPeak"] )
    {
      m_dblManager->setValue(m_properties["ResELow"], val);
    }
    else if ( from == m_rangeSelectors["ResBackground"] )
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
  void IndirectCalibration::calMaxChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_rangeSelectors["CalPeak"] )
    {
      m_dblManager->setValue(m_properties["CalPeakMax"], val);
    }
    else if ( from == m_rangeSelectors["CalBackground"] )
    {
      m_dblManager->setValue(m_properties["CalBackMax"], val);
    }
    else if ( from == m_rangeSelectors["ResPeak"] )
    {
      m_dblManager->setValue(m_properties["ResEHigh"], val);
    }
    else if ( from == m_rangeSelectors["ResBackground"] )
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
  void IndirectCalibration::calUpdateRS(QtProperty* prop, double val)
  {
    if ( prop == m_properties["CalPeakMin"] ) m_rangeSelectors["CalPeak"]->setMinimum(val);
    else if ( prop == m_properties["CalPeakMax"] ) m_rangeSelectors["CalPeak"]->setMaximum(val);
    else if ( prop == m_properties["CalBackMin"] ) m_rangeSelectors["CalBackground"]->setMinimum(val);
    else if ( prop == m_properties["CalBackMax"] ) m_rangeSelectors["CalBackground"]->setMaximum(val);
    else if ( prop == m_properties["ResStart"] ) m_rangeSelectors["ResBackground"]->setMinimum(val);
    else if ( prop == m_properties["ResEnd"] ) m_rangeSelectors["ResBackground"]->setMaximum(val);
    else if ( prop == m_properties["ResELow"] ) m_rangeSelectors["ResPeak"]->setMinimum(val);
    else if ( prop == m_properties["ResEHigh"] ) m_rangeSelectors["ResPeak"]->setMaximum(val);
  }

  /**
  * This function enables/disables the display of the options involved in creating the RES file.
  *
  * @param state :: whether checkbox is checked or unchecked
  */
  void IndirectCalibration::resCheck(bool state)
  {
    m_rangeSelectors["ResPeak"]->setVisible(state);
    m_rangeSelectors["ResBackground"]->setVisible(state);
  }

  /**
   * This function is called after calib has run and creates a RES file for use in later analysis (Fury,etc)
   * 
   * @param file :: the input file (WBV run.raw)
   */
  void IndirectCalibration::createRESfile(const QString& file)
  {
    QString scaleFactor("1.0");
    if(m_uiForm.cal_ckResScale->isChecked())
    {
      if(!m_uiForm.cal_leResScale->text().isEmpty())
      {
        scaleFactor = m_uiForm.cal_leResScale->text();
      }
    }

    QFileInfo fi(file);
    QString outWS = fi.baseName() + "_" + m_uiForm.cbAnalyser->currentText() +
        m_uiForm.cbReflection->currentText() + "_res";
    /* outWS = outWS.toLower(); */

    QString detRange = QString::number(m_dblManager->value(m_properties["ResSpecMin"])) + ","
        + QString::number(m_dblManager->value(m_properties["ResSpecMax"]));

    QString rebinString = QString::number(m_dblManager->value(m_properties["ResELow"])) + "," +
      QString::number(m_dblManager->value(m_properties["ResEWidth"])) + "," +
      QString::number(m_dblManager->value(m_properties["ResEHigh"]));

    QString background = QString::number(m_dblManager->value(m_properties["ResStart"])) + ","
        + QString::number(m_dblManager->value(m_properties["ResEnd"]));

    Mantid::API::IAlgorithm_sptr resAlg = Mantid::API::AlgorithmManager::Instance().create("IndirectResolution", -1);
    resAlg->initialize();

    resAlg->setProperty("InputFiles", file.toStdString());
    resAlg->setProperty("OutputWorkspace", outWS.toStdString());

    resAlg->setProperty("Instrument", m_uiForm.cbInst->currentText().toStdString());
    resAlg->setProperty("Analyser", m_uiForm.cbAnalyser->currentText().toStdString());
    resAlg->setProperty("Reflection", m_uiForm.cbReflection->currentText().toStdString());

    resAlg->setProperty("RebinString", rebinString.toStdString());

    resAlg->setProperty("Res", true);
    resAlg->setProperty("DetectorRange", detRange.toStdString());
    resAlg->setProperty("BackgroundRange", background.toStdString());

    resAlg->setProperty("ScaleFactor", m_uiForm.cal_leIntensityScaleMultiplier->text().toDouble());

    resAlg->setProperty("Verbose", m_uiForm.cal_ckVerbose->isChecked());
    resAlg->setProperty("Plot", m_uiForm.cal_ckPlotResult->isChecked());
    resAlg->setProperty("Save", m_uiForm.cal_ckSave->isChecked());

    resAlg->execute();

    if(!resAlg->isExecuted())
    {
      emit showMessageBox("Unable to create RES file");
    }
  }

  /**
   * Enables or disables the scale multiplier input box when
   * the check box state changes
   *
   * @param state :: True to enable input, false to disable
   */
  void IndirectCalibration::intensityScaleMultiplierCheck(bool state)
  {
    m_uiForm.cal_leIntensityScaleMultiplier->setEnabled(state);
  }

  /**
   * Hides/shows the required indicator of the scale multiplier
   * input box
   *
   * @param text :: Text currently in box
   */
  void IndirectCalibration::calibValidateIntensity(const QString & text)
  {
    if(!text.isEmpty())
    {
      m_uiForm.cal_valIntensityScaleMultiplier->setText(" ");
    }
    else
    {
      m_uiForm.cal_valIntensityScaleMultiplier->setText("*");
    }
  }

} // namespace CustomInterfaces
} // namespace Mantid
