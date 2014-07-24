#include "MantidQtCustomInterfaces/IndirectCalibration.h"

#include <QFileInfo>
#include <QInputDialog>

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
    m_valDbl = new QDoubleValidator(this);

    // General
    m_calDblMng = new QtDoublePropertyManager();
    m_calGrpMng = new QtGroupPropertyManager();

    /* Calib */
    m_calCalTree = new QtTreePropertyBrowser();
    m_uiForm.cal_treeCal->addWidget(m_calCalTree);

    DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
    m_calCalTree->setFactoryForManager(m_calDblMng, doubleEditorFactory);

    m_calCalProp["PeakMin"] = m_calDblMng->addProperty("Peak Min");
    m_calCalProp["PeakMax"] = m_calDblMng->addProperty("Peak Max");
    m_calCalProp["BackMin"] = m_calDblMng->addProperty("Back Min");
    m_calCalProp["BackMax"] = m_calDblMng->addProperty("Back Max");


    m_calCalTree->addProperty(m_calCalProp["PeakMin"]);
    m_calCalTree->addProperty(m_calCalProp["PeakMax"]);
    m_calCalTree->addProperty(m_calCalProp["BackMin"]);
    m_calCalTree->addProperty(m_calCalProp["BackMax"]);

    m_calCalPlot = new QwtPlot(this);
    m_calCalPlot->setAxisFont(QwtPlot::xBottom, this->font());
    m_calCalPlot->setAxisFont(QwtPlot::yLeft, this->font());
    m_uiForm.cal_plotCal->addWidget(m_calCalPlot);
    m_calCalPlot->setCanvasBackground(Qt::white);

    // R1 = Peak, R2 = Background
    m_calCalR1 = new MantidWidgets::RangeSelector(m_calCalPlot);
    connect(m_calCalR1, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(m_calCalR1, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    m_calCalR2 = new MantidWidgets::RangeSelector(m_calCalPlot);
    m_calCalR2->setColour(Qt::darkGreen); // dark green to signify background range
    connect(m_calCalR2, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(m_calCalR2, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));

    // Res
    m_calResTree = new QtTreePropertyBrowser();
    m_uiForm.cal_treeRes->addWidget(m_calResTree);

    m_calResTree->setFactoryForManager(m_calDblMng, doubleEditorFactory);

    // Res - Spectra Selection
    m_calResProp["SpecMin"] = m_calDblMng->addProperty("Spectra Min");
    m_calResProp["SpecMax"] = m_calDblMng->addProperty("Spectra Max");
    m_calResTree->addProperty(m_calResProp["SpecMin"]);
    m_calDblMng->setDecimals(m_calResProp["SpecMin"], 0);
    m_calResTree->addProperty(m_calResProp["SpecMax"]);
    m_calDblMng->setDecimals(m_calResProp["SpecMax"], 0);

    // Res - Background Properties
    QtProperty* resBG = m_calGrpMng->addProperty("Background");
    m_calResProp["Start"] = m_calDblMng->addProperty("Start");
    m_calResProp["End"] = m_calDblMng->addProperty("End");
    resBG->addSubProperty(m_calResProp["Start"]);
    resBG->addSubProperty(m_calResProp["End"]);
    m_calResTree->addProperty(resBG);

    // Res - rebinning
    const int NUM_DECIMALS = 3;
    QtProperty* resRB = m_calGrpMng->addProperty("Rebinning");

    m_calResProp["ELow"] = m_calDblMng->addProperty("Low");
    m_calDblMng->setDecimals(m_calResProp["ELow"], NUM_DECIMALS);
    m_calDblMng->setValue(m_calResProp["ELow"], -0.2);

    m_calResProp["EWidth"] = m_calDblMng->addProperty("Width");
    m_calDblMng->setDecimals(m_calResProp["EWidth"], NUM_DECIMALS);
    m_calDblMng->setValue(m_calResProp["EWidth"], 0.002);
    m_calDblMng->setMinimum(m_calResProp["EWidth"], 0.001);

    m_calResProp["EHigh"] = m_calDblMng->addProperty("High");
    m_calDblMng->setDecimals(m_calResProp["EHigh"], NUM_DECIMALS);
    m_calDblMng->setValue(m_calResProp["EHigh"], 0.2);

    resRB->addSubProperty(m_calResProp["ELow"]);
    resRB->addSubProperty(m_calResProp["EWidth"]);
    resRB->addSubProperty(m_calResProp["EHigh"]);

    m_calResTree->addProperty(resRB);

    m_calResPlot = new QwtPlot(this);
    m_calResPlot->setAxisFont(QwtPlot::xBottom, this->font());
    m_calResPlot->setAxisFont(QwtPlot::yLeft, this->font());
    m_uiForm.cal_plotRes->addWidget(m_calResPlot);
    m_calResPlot->setCanvasBackground(Qt::white);

    // Create ResR2 first so ResR1 is drawn above it.
    m_calResR2 = new MantidWidgets::RangeSelector(m_calResPlot, 
        MantidQt::MantidWidgets::RangeSelector::XMINMAX, true, false);
    m_calResR2->setColour(Qt::darkGreen);
    m_calResR1 = new MantidWidgets::RangeSelector(m_calResPlot, MantidQt::MantidWidgets::RangeSelector::XMINMAX, true, true);
      
    m_uiForm.cal_leIntensityScaleMultiplier->setValidator(m_valDbl);
    m_uiForm.cal_leResScale->setValidator(m_valDbl);

    m_uiForm.cal_valIntensityScaleMultiplier->setText(" ");

    connect(m_calResR1, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(m_calResR1, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    connect(m_calResR2, SIGNAL(minValueChanged(double)), this, SLOT(calMinChanged(double)));
    connect(m_calResR2, SIGNAL(maxValueChanged(double)), this, SLOT(calMaxChanged(double)));
    connect(m_calResR1, SIGNAL(rangeChanged(double, double)), m_calResR2, SLOT(setRange(double, double)));
    connect(m_calDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(calUpdateRS(QtProperty*, double)));
    connect(m_calDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(calUpdateRS(QtProperty*, double)));
    connect(m_calDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(calUpdateRS(QtProperty*, double)));

    connect(m_uiForm.cal_leRunNo, SIGNAL(filesFound()), this, SLOT(calPlotRaw()));
    connect(m_uiForm.cal_pbPlot, SIGNAL(clicked()), this, SLOT(calPlotRaw()));
    connect(m_uiForm.cal_ckRES, SIGNAL(toggled(bool)), this, SLOT(resCheck(bool)));
    connect(m_uiForm.cal_ckRES, SIGNAL(toggled(bool)), m_uiForm.cal_ckResScale, SLOT(setEnabled(bool)));
    connect(m_uiForm.cal_ckResScale, SIGNAL(toggled(bool)), m_uiForm.cal_leResScale, SLOT(setEnabled(bool)));
    connect(m_uiForm.cal_ckIntensityScaleMultiplier, SIGNAL(toggled(bool)), this, SLOT(intensityScaleMultiplierCheck(bool)));
    connect(m_uiForm.cal_leIntensityScaleMultiplier, SIGNAL(textChanged(const QString &)), this, SLOT(calibValidateIntensity(const QString &)));
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
      "calib.set_parameters(" + m_calCalProp["BackMin"]->valueText() + "," 
      + m_calCalProp["BackMax"]->valueText() + ","
      + m_calCalProp["PeakMin"]->valueText() + ","
      + m_calCalProp["PeakMax"]->valueText() + ")\n"
      "calib.set_analyser('" + m_uiForm.cbAnalyser->currentText() + "')\n"
      "calib.set_reflection('" + m_uiForm.cbReflection->currentText() + "')\n";

    //scale values by arbitrary scalar if requested
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
          createRESfile(filenames);
        }
        m_uiForm.ind_calibFile->setFileTextWithSearch(pyOutput + ".nxs");
        m_uiForm.ckUseCalib->setChecked(true);
      }
  }

  bool IndirectCalibration::validate()
  {
    MantidQt::CustomInterfaces::UserInputValidator uiv;

    uiv.checkMWRunFilesIsValid("Run", m_uiForm.cal_leRunNo);

    auto peakRange = std::make_pair(m_calDblMng->value(m_calCalProp["PeakMin"]), m_calDblMng->value(m_calCalProp["PeakMax"]));
    auto backRange = std::make_pair(m_calDblMng->value(m_calCalProp["BackMin"]), m_calDblMng->value(m_calCalProp["BackMax"]));

    uiv.checkValidRange("Peak Range", peakRange);
    uiv.checkValidRange("Back Range", backRange);
    uiv.checkRangesDontOverlap(peakRange, backRange);

    if ( m_uiForm.cal_ckRES->isChecked() )
    {
      auto backgroundRange = std::make_pair(m_calDblMng->value(m_calResProp["Start"]), m_calDblMng->value(m_calResProp["End"]));
      uiv.checkValidRange("Background", backgroundRange);

      double eLow   = m_calDblMng->value(m_calResProp["ELow"]);
      double eHigh  = m_calDblMng->value(m_calResProp["EHigh"]);
      double eWidth = m_calDblMng->value(m_calResProp["EWidth"]);

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

    return uiv.generateErrorMessage() == "";
  }

  void IndirectCalibration::calPlotRaw()
  {
    QString filename = m_uiForm.cal_leRunNo->getFirstFilename();

    if ( filename.isEmpty() )
    {
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

    if( ! pyOutput.isEmpty() )
    {
      emit showMessageBox("Unable to load file.  Error: \n\n" + pyOutput + "\nCheck whether your file exists and matches the selected instrument in the Energy Transfer tab.");
      return;
    }

    Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

    const Mantid::MantidVec & dataX = input->readX(0);
    const Mantid::MantidVec & dataY = input->readY(0);

    if ( m_calCalCurve != NULL )
    {
      m_calCalCurve->attach(0);
      delete m_calCalCurve;
      m_calCalCurve = 0;
    }

    m_calCalCurve = new QwtPlotCurve();
    m_calCalCurve->setData(&dataX[0], &dataY[0], static_cast<int>(input->blocksize()));
    m_calCalCurve->attach(m_calCalPlot);

    std::pair<double, double> range(dataX.front(), dataX.back());
    m_calCalPlot->setAxisScale(QwtPlot::xBottom, range.first, range.second);
    setPlotRange(m_calCalR1, m_calCalProp["PeakMin"], m_calCalProp["PeakMax"], range);
    setPlotRange(m_calCalR2, m_calCalProp["BackMin"], m_calCalProp["BackMax"], range);

    // Replot
    m_calCalPlot->replot();

    // also replot the energy
    calPlotEnergy();
  }

  void IndirectCalibration::calPlotEnergy()
  {
    if ( ! m_uiForm.cal_leRunNo->isValid() )
    {
      emit showMessageBox("Run number not valid.");
      return;
    }

    QString files = "[r'" + m_uiForm.cal_leRunNo->getFilenames().join("', r'") + "']";
    QString pyInput =
      "from IndirectEnergyConversion import resolution\n"
      "iconOpt = { 'first': " +QString::number(m_calDblMng->value(m_calResProp["SpecMin"]))+
      ", 'last': " +QString::number(m_calDblMng->value(m_calResProp["SpecMax"]))+ "}\n"
      "instrument = '" + m_uiForm.cbInst->currentText() + "'\n"
      "analyser = '" + m_uiForm.cbAnalyser->currentText() + "'\n"
      "reflection = '" + m_uiForm.cbReflection->currentText() + "'\n"
      "files = " + files + "\n"
      "outWS = resolution(files, iconOpt, '', '', instrument, analyser, reflection, Res=False)\n"
      "print outWS\n";
    QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();

    //something went wrong in the python
    if(pyOutput == "None")
    {
      emit showMessageBox("Failed to convert to energy. See log for details.");
      return;
    }

    Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(pyOutput.toStdString()));

    const Mantid::MantidVec & dataX = input->readX(0);
    const Mantid::MantidVec & dataY = input->readY(0);

    if ( m_calResCurve != NULL )
    {
      m_calResCurve->attach(0);
      delete m_calResCurve;
      m_calResCurve = 0;
    }

    m_calResCurve = new QwtPlotCurve();
    m_calResCurve->setData(&dataX[0], &dataY[0], static_cast<int>(input->blocksize()));
    m_calResCurve->attach(m_calResPlot);

    std::pair<double, double> range(dataX.front(), dataX.back());
    m_calResPlot->setAxisScale(QwtPlot::xBottom, range.first, range.second);

    setPlotRange(m_calResR1, m_calResProp["ELow"], m_calResProp["EHigh"], range);
    setPlotRange(m_calResR2, m_calResProp["Start"], m_calResProp["End"], range);

    calSetDefaultResolution(input);

    // Replot
    m_calResPlot->replot();
  }

  void IndirectCalibration::calSetDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws)
  {
    auto inst = ws->getInstrument();
    auto analyser = inst->getStringParameter("analyser");

    if(analyser.size() > 0)
    {
      auto comp = inst->getComponentByName(analyser[0]);
      auto params = comp->getNumberParameter("resolution", true);

      //set the default instrument resolution
      if(params.size() > 0)
      {
        double res = params[0];
        m_calDblMng->setValue(m_calResProp["ELow"], -res*10);
        m_calDblMng->setValue(m_calResProp["EHigh"], res*10);

        m_calDblMng->setValue(m_calResProp["Start"], -res*9);
        m_calDblMng->setValue(m_calResProp["End"], -res*8);
      }
    }
  }

  void IndirectCalibration::calMinChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_calCalR1 )
    {
      m_calDblMng->setValue(m_calCalProp["PeakMin"], val);
    }
    else if ( from == m_calCalR2 )
    {
      m_calDblMng->setValue(m_calCalProp["BackMin"], val);
    }
    else if ( from == m_calResR1 )
    {
      m_calDblMng->setValue(m_calResProp["ELow"], val);
    }
    else if ( from == m_calResR2 )
    {
      m_calDblMng->setValue(m_calResProp["Start"], val);
    }
  }

  void IndirectCalibration::calMaxChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_calCalR1 )
    {
      m_calDblMng->setValue(m_calCalProp["PeakMax"], val);
    }
    else if ( from == m_calCalR2 )
    {
      m_calDblMng->setValue(m_calCalProp["BackMax"], val);
    }
    else if ( from == m_calResR1 )
    {
      m_calDblMng->setValue(m_calResProp["EHigh"], val);
    }
    else if ( from == m_calResR2 )
    {
      m_calDblMng->setValue(m_calResProp["End"], val);
    }
  }

  void IndirectCalibration::calUpdateRS(QtProperty* prop, double val)
  {
    if ( prop == m_calCalProp["PeakMin"] ) m_calCalR1->setMinimum(val);
    else if ( prop == m_calCalProp["PeakMax"] ) m_calCalR1->setMaximum(val);
    else if ( prop == m_calCalProp["BackMin"] ) m_calCalR2->setMinimum(val);
    else if ( prop == m_calCalProp["BackMax"] ) m_calCalR2->setMaximum(val);
    else if ( prop == m_calResProp["Start"] ) m_calResR2->setMinimum(val);
    else if ( prop == m_calResProp["End"] ) m_calResR2->setMaximum(val);
    else if ( prop == m_calResProp["ELow"] ) m_calResR1->setMinimum(val);
    else if ( prop == m_calResProp["EHigh"] ) m_calResR1->setMaximum(val);
  }

  /**
  * This function enables/disables the display of the options involved in creating the RES file.
  * @param state :: whether checkbox is checked or unchecked
  */
  void IndirectCalibration::resCheck(bool state)
  {
    m_calResR1->setVisible(state);
    m_calResR2->setVisible(state);
  }

  /**
   * This function is called after calib has run and creates a RES file for use in later analysis (Fury,etc)
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

    QString pyInput =
      "from IndirectEnergyConversion import resolution\n"
      "iconOpt = { 'first': " +QString::number(m_calDblMng->value(m_calResProp["SpecMin"]))+
      ", 'last': " +QString::number(m_calDblMng->value(m_calResProp["SpecMax"]))+"}\n"

      "instrument = '" + m_uiForm.cbInst->currentText() + "'\n"
      "analyser = '" + m_uiForm.cbAnalyser->currentText() + "'\n"
      "reflection = '" + m_uiForm.cbReflection->currentText() + "'\n";

    if ( m_uiForm.cal_ckPlotResult->isChecked() ) { pyInput +=	"plot = True\n"; }
    else { pyInput += "plot = False\n"; }

    if ( m_uiForm.cal_ckVerbose->isChecked() ) { pyInput +=  "verbose = True\n"; }
    else { pyInput += "verbose = False\n"; }

    if ( m_uiForm.cal_ckSave->isChecked() ) { pyInput +=  "save = True\n"; }
    else { pyInput += "save = False\n"; }

    QString rebinParam = QString::number(m_calDblMng->value(m_calResProp["ELow"])) + "," +
      QString::number(m_calDblMng->value(m_calResProp["EWidth"])) + "," +
      QString::number(m_calDblMng->value(m_calResProp["EHigh"]));

    QString background = "[ " +QString::number(m_calDblMng->value(m_calResProp["Start"]))+ ", " +QString::number(m_calDblMng->value(m_calResProp["End"]))+"]";

    QString scaled = m_uiForm.cal_ckIntensityScaleMultiplier->isChecked() ? "True" : "False";
    pyInput +=
      "background = " + background + "\n"
      "rebinParam = '" + rebinParam + "'\n"
      "file = " + file + "\n"
      "ws = resolution(file, iconOpt, rebinParam, background, instrument, analyser, reflection, Verbose=verbose, Plot=plot, Save=save, factor="+scaleFactor+")\n"
      "scaled = "+ scaled +"\n"
      "scaleFactor = "+m_uiForm.cal_leIntensityScaleMultiplier->text()+"\n"
      "backStart = "+QString::number(m_calDblMng->value(m_calCalProp["BackMin"]))+"\n"
      "backEnd = "+QString::number(m_calDblMng->value(m_calCalProp["BackMax"]))+"\n"
      "rebinLow = "+QString::number(m_calDblMng->value(m_calResProp["ELow"]))+"\n"
      "rebinWidth = "+QString::number(m_calDblMng->value(m_calResProp["EWidth"]))+"\n"
      "rebinHigh = "+QString::number(m_calDblMng->value(m_calResProp["EHigh"]))+"\n"
      "AddSampleLog(Workspace=ws, LogName='scale', LogType='String', LogText=str(scaled))\n"
      "if scaled:"
      "  AddSampleLog(Workspace=ws, LogName='scale_factor', LogType='Number', LogText=str(scaleFactor))\n"
      "AddSampleLog(Workspace=ws, LogName='back_start', LogType='Number', LogText=str(backStart))\n"
      "AddSampleLog(Workspace=ws, LogName='back_end', LogType='Number', LogText=str(backEnd))\n"
      "AddSampleLog(Workspace=ws, LogName='rebin_low', LogType='Number', LogText=str(rebinLow))\n"
      "AddSampleLog(Workspace=ws, LogName='rebin_width', LogType='Number', LogText=str(rebinWidth))\n"
      "AddSampleLog(Workspace=ws, LogName='rebin_high', LogType='Number', LogText=str(rebinHigh))\n";

    QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();

    if ( pyOutput != "" )
    {
      emit showMessageBox("Unable to create RES file: \n" + pyOutput);
    }
  }

  /**
   * Plots raw time data from .raw file before any data conversion has been performed.
   */
  void IndirectCalibration::plotRaw()
  {
    if ( m_uiForm.ind_runFiles->isValid() )
    {
      bool ok;
      QString spectraRange = QInputDialog::getText(this, "Insert Spectra Ranges", "Range: ", QLineEdit::Normal, m_uiForm.leSpectraMin->text() +"-"+ m_uiForm.leSpectraMax->text(), &ok);

      if ( !ok || spectraRange.isEmpty() )
      {
        return;
      }
      QStringList specList = spectraRange.split("-");

      QString rawFile = m_uiForm.ind_runFiles->getFirstFilename();
      if ( (specList.size() > 2) || ( specList.size() < 1) )
      {
        emit showMessageBox("Invalid input. Must be of form <SpecMin>-<SpecMax>");
        return;
      }
      if ( specList.size() == 1 )
      {
        specList.append(specList[0]);
      }

      QString bgrange;

      //TODO: This uses settings from COnvert To Energy tab
      /* if ( m_bgRemoval ) */
      /* { */
      /*   QPair<double, double> range = m_backgroundDialog->getRange(); */
      /*   bgrange = "[ " + QString::number(range.first) + "," + QString::number(range.second) + " ]"; */
      /* } */
      /* else */
      /* { */
        bgrange = "[-1, -1]";
      /* } */

      QString pyInput =
        "from mantid.simpleapi import CalculateFlatBackground,GroupDetectors,Load\n"
        "from mantidplot import plotSpectrum\n"
        "import os.path as op\n"
        "file = r'" + rawFile + "'\n"
        "name = op.splitext( op.split(file)[1] )[0]\n"
        "bgrange = " + bgrange + "\n"
        "Load(Filename=file, OutputWorkspace=name, SpectrumMin="+specList[0]+", SpectrumMax="+specList[1]+")\n"
        "if ( bgrange != [-1, -1] ):\n"
        "    #Remove background\n"
        "    CalculateFlatBackground(InputWorkspace=name, OutputWorkspace=name+'_bg', StartX=bgrange[0], EndX=bgrange[1], Mode='Mean')\n"
        "    GroupDetectors(InputWorkspace=name+'_bg', OutputWorkspace=name+'_grp', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
        "    GroupDetectors(InputWorkspace=name, OutputWorkspace=name+'_grp_raw', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
        "else: # Just group detectors as they are\n"
        "    GroupDetectors(InputWorkspace=name, OutputWorkspace=name+'_grp', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
        "graph = plotSpectrum(name+'_grp', 0)\n";

      QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();

      if ( pyOutput != "" )
      {
        emit showMessageBox(pyOutput);
      }
    }
    else
    {
      emit showMessageBox("You must select a run file.");
    }
  }

} // namespace CustomInterfaces
} // namespace Mantid
