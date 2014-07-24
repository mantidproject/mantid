#include "MantidQtCustomInterfaces/IndirectDiagnostics.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QFileInfo>

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectDiagnostics::IndirectDiagnostics(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
    // Property Tree
    m_sltTree = new QtTreePropertyBrowser();
    m_uiForm.slice_properties->addWidget(m_sltTree);

    // Create Manager Objects
    m_sltDblMng = new QtDoublePropertyManager();
    m_sltBlnMng = new QtBoolPropertyManager();
    m_sltGrpMng = new QtGroupPropertyManager();

    // Editor Factories
    DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
    QtCheckBoxFactory *checkboxFactory = new QtCheckBoxFactory();
    m_sltTree->setFactoryForManager(m_sltDblMng, doubleEditorFactory);
    m_sltTree->setFactoryForManager(m_sltBlnMng, checkboxFactory);

    // Create Properties
    m_sltProp["SpecMin"] = m_sltDblMng->addProperty("Spectra Min");
    m_sltProp["SpecMax"] = m_sltDblMng->addProperty("Spectra Max");
    m_sltDblMng->setDecimals(m_sltProp["SpecMin"], 0);
    m_sltDblMng->setMinimum(m_sltProp["SpecMin"], 1);
    m_sltDblMng->setDecimals(m_sltProp["SpecMax"], 0);

    m_sltProp["R1S"] = m_sltDblMng->addProperty("Start");
    m_sltProp["R1E"] = m_sltDblMng->addProperty("End");
    m_sltProp["R2S"] = m_sltDblMng->addProperty("Start");
    m_sltProp["R2E"] = m_sltDblMng->addProperty("End");

    m_sltProp["UseTwoRanges"] = m_sltBlnMng->addProperty("Use Two Ranges");

    m_sltProp["Range1"] = m_sltGrpMng->addProperty("Range One");
    m_sltProp["Range1"]->addSubProperty(m_sltProp["R1S"]);
    m_sltProp["Range1"]->addSubProperty(m_sltProp["R1E"]);
    m_sltProp["Range2"] = m_sltGrpMng->addProperty("Range Two");
    m_sltProp["Range2"]->addSubProperty(m_sltProp["R2S"]);
    m_sltProp["Range2"]->addSubProperty(m_sltProp["R2E"]);

    m_sltTree->addProperty(m_sltProp["SpecMin"]);
    m_sltTree->addProperty(m_sltProp["SpecMax"]);
    m_sltTree->addProperty(m_sltProp["Range1"]);
    m_sltTree->addProperty(m_sltProp["UseTwoRanges"]);
    m_sltTree->addProperty(m_sltProp["Range2"]);

    // Create Slice Plot Widget for Range Selection
    m_sltPlot = new QwtPlot(this);
    m_sltPlot->setAxisFont(QwtPlot::xBottom, this->font());
    m_sltPlot->setAxisFont(QwtPlot::yLeft, this->font());
    m_uiForm.slice_plot->addWidget(m_sltPlot);
    m_sltPlot->setCanvasBackground(Qt::white);
    // We always want one range selector... the second one can be controlled from
    // within the sliceTwoRanges(bool state) function
    m_sltR1 = new MantidWidgets::RangeSelector(m_sltPlot);
    connect(m_sltR1, SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
    connect(m_sltR1, SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));

    // second range
    // create the second range
    m_sltR2 = new MantidWidgets::RangeSelector(m_sltPlot);
    m_sltR2->setColour(Qt::darkGreen); // dark green for background
    connect(m_sltR1, SIGNAL(rangeChanged(double, double)), m_sltR2, SLOT(setRange(double, double)));
    connect(m_sltR2, SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
    connect(m_sltR2, SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));
    m_sltR2->setRange(m_sltR1->getRange());

    // Refresh the plot window
    m_sltPlot->replot();

    connect(m_sltDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(sliceUpdateRS(QtProperty*, double)));
    connect(m_sltBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(sliceTwoRanges(QtProperty*, bool)));

    sliceTwoRanges(0, false); // set default value

    connect(m_uiForm.slice_inputFile, SIGNAL(filesFound()), this, SLOT(slicePlotRaw()));
    connect(m_uiForm.slice_pbPlotRaw, SIGNAL(clicked()), this, SLOT(slicePlotRaw()));
    connect(m_uiForm.slice_ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(sliceCalib(bool)));
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectDiagnostics::~IndirectDiagnostics()
  {
  }
  
  void IndirectDiagnostics::setup()
  {
  }

  void IndirectDiagnostics::run()
  {
    QString pyInput =
      "from IndirectEnergyConversion import slice\n"
      "tofRange = [" + QString::number(m_sltDblMng->value(m_sltProp["R1S"])) + ","
      + QString::number(m_sltDblMng->value(m_sltProp["R1E"]));
    if ( m_sltBlnMng->value(m_sltProp["UseTwoRanges"]) )
    {
      pyInput +=
        "," + QString::number(m_sltDblMng->value(m_sltProp["R2S"])) + ","
        + QString::number(m_sltDblMng->value(m_sltProp["R2E"])) + "]\n";
    }
    else
    {
      pyInput += "]\n";
    }
    if ( m_uiForm.slice_ckUseCalib->isChecked() )
    {
      pyInput +=
        "calib = r'" + m_uiForm.slice_calibFile->getFirstFilename() + "'\n";
    }
    else
    {
      pyInput +=
        "calib = ''\n";
    }
    QString filenames = m_uiForm.slice_inputFile->getFilenames().join("', r'");
    QString suffix = m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText();
    pyInput +=
      "rawfile = [r'" + filenames + "']\n"
      "spectra = ["+ QString::number(m_sltDblMng->value(m_sltProp["SpecMin"])) + "," + QString::number(m_sltDblMng->value(m_sltProp["SpecMax"])) +"]\n"
      "suffix = '" + suffix + "'\n";

    if ( m_uiForm.slice_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( m_uiForm.slice_ckPlot->isChecked() ) pyInput += "plot = True\n";
    else pyInput += "plot = False\n";

    if ( m_uiForm.slice_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput +=
      "slice(rawfile, calib, tofRange, spectra, suffix, Save=save, Verbose=verbose, Plot=plot)";

    QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();
  }

  bool IndirectDiagnostics::validate()
  {
    UserInputValidator uiv;

    uiv.checkMWRunFilesIsValid("Input", m_uiForm.slice_inputFile);
    if( m_uiForm.slice_ckUseCalib->isChecked() )
      uiv.checkMWRunFilesIsValid("Calibration", m_uiForm.slice_inputFile);

    auto rangeOne = std::make_pair(m_sltDblMng->value(m_sltProp["R1S"]), m_sltDblMng->value(m_sltProp["R1E"]));
    uiv.checkValidRange("Range One", rangeOne);

    bool useTwoRanges = m_sltBlnMng->value(m_sltProp["UseTwoRanges"]);
    if( useTwoRanges )
    {
      auto rangeTwo = std::make_pair(m_sltDblMng->value(m_sltProp["R2S"]), m_sltDblMng->value(m_sltProp["R2E"]));
      uiv.checkValidRange("Range Two", rangeTwo);

      uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
    }

    auto specRange = std::make_pair(m_sltDblMng->value(m_sltProp["SpecMin"]), m_sltDblMng->value(m_sltProp["SpecMax"]));
    uiv.checkValidRange("Spectra Range", specRange);

    return uiv.generateErrorMessage() == "";
  }

  void IndirectDiagnostics::slicePlotRaw()
  {
    if ( m_uiForm.slice_inputFile->isValid() )
    {
      QString filename = m_uiForm.slice_inputFile->getFirstFilename();
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
        emit showMessageBox("Unable to load file: \n\n\"" + pyOutput + "\".\n\nCheck whether your file exists and matches the selected instrument in the EnergyTransfer tab.");
        return;
      }

      Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

      const Mantid::MantidVec & dataX = input->readX(0);
      const Mantid::MantidVec & dataY = input->readY(0);

      if ( m_sltDataCurve != NULL )
      {
        m_sltDataCurve->attach(0);
        delete m_sltDataCurve;
        m_sltDataCurve = 0;
      }

      m_sltDataCurve = new QwtPlotCurve();
      m_sltDataCurve->setData(&dataX[0], &dataY[0], static_cast<int>(input->blocksize()));
      m_sltDataCurve->attach(m_sltPlot);

      std::pair<double, double> range(dataX.front(), dataX.back());
      /* m_calResPlot->setAxisScale(QwtPlot::xBottom, range.first, range.second); */
      setPlotRange(m_sltR1, m_sltProp["R1S"], m_sltProp["R1E"], range);
      setPlotRange(m_sltR2, m_sltProp["R2S"], m_sltProp["R2E"], range);

      // Replot
      m_sltPlot->replot();
    }
    else
    {
      emit showMessageBox("Selected input files are invalid.");
    }

  }

  void IndirectDiagnostics::sliceTwoRanges(QtProperty*, bool state)
  {
    m_sltR2->setVisible(state);
  }

  void IndirectDiagnostics::sliceCalib(bool state)
  {
    m_uiForm.slice_calibFile->setEnabled(state);
    m_uiForm.slice_calibFile->isOptional(!state);
  }

  void IndirectDiagnostics::sliceMinChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_sltR1 )
    {
      m_sltDblMng->setValue(m_sltProp["R1S"], val);
    }
    else if ( from == m_sltR2 )
    {
      m_sltDblMng->setValue(m_sltProp["R2S"], val);
    }
  }

  void IndirectDiagnostics::sliceMaxChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_sltR1 )
    {
      m_sltDblMng->setValue(m_sltProp["R1E"], val);
    }
    else if ( from == m_sltR2 )
    {
      m_sltDblMng->setValue(m_sltProp["R2E"], val);
    }
  }

  void IndirectDiagnostics::sliceUpdateRS(QtProperty* prop, double val)
  {
    if ( prop == m_sltProp["R1S"] ) m_sltR1->setMinimum(val);
    else if ( prop == m_sltProp["R1E"] ) m_sltR1->setMaximum(val);
    else if ( prop == m_sltProp["R2S"] ) m_sltR2->setMinimum(val);
    else if ( prop == m_sltProp["R2E"] ) m_sltR2->setMaximum(val);
  }

} // namespace CustomInterfaces
} // namespace Mantid
