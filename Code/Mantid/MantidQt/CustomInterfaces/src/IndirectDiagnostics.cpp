#include "MantidQtCustomInterfaces/IndirectDiagnostics.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>

namespace
{
  Mantid::Kernel::Logger g_log("IndirectDiagnostics");
}

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
    m_plots["SlicePlot"] = new QwtPlot(m_parentWidget);
    m_curves["SlicePlot"] = new QwtPlotCurve();
    m_rangeSelectors["SliceRange1"] = new MantidWidgets::RangeSelector(m_plots["SlicePlot"]);
    m_rangeSelectors["SliceRange2"] = new MantidWidgets::RangeSelector(m_plots["SlicePlot"]);
    m_propTrees["SlicePropTree"] = new QtTreePropertyBrowser();

    // Property Tree
    m_uiForm.slice_properties->addWidget(m_propTrees["SlicePropTree"]);

    // Editor Factories
    DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
    QtCheckBoxFactory *checkboxFactory = new QtCheckBoxFactory();
    m_propTrees["SlicePropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);
    m_propTrees["SlicePropTree"]->setFactoryForManager(m_blnManager, checkboxFactory);

    // Create Properties
    m_properties["SpecMin"] = m_dblManager->addProperty("Spectra Min");
    m_properties["SpecMax"] = m_dblManager->addProperty("Spectra Max");
    m_dblManager->setDecimals(m_properties["SpecMin"], 0);
    m_dblManager->setMinimum(m_properties["SpecMin"], 1);
    m_dblManager->setDecimals(m_properties["SpecMax"], 0);

    m_properties["R1S"] = m_dblManager->addProperty("Start");
    m_properties["R1E"] = m_dblManager->addProperty("End");
    m_properties["R2S"] = m_dblManager->addProperty("Start");
    m_properties["R2E"] = m_dblManager->addProperty("End");

    m_properties["UseTwoRanges"] = m_blnManager->addProperty("Use Two Ranges");

    m_properties["Range1"] = m_grpManager->addProperty("Range One");
    m_properties["Range1"]->addSubProperty(m_properties["R1S"]);
    m_properties["Range1"]->addSubProperty(m_properties["R1E"]);
    m_properties["Range2"] = m_grpManager->addProperty("Range Two");
    m_properties["Range2"]->addSubProperty(m_properties["R2S"]);
    m_properties["Range2"]->addSubProperty(m_properties["R2E"]);

    m_propTrees["SlicePropTree"]->addProperty(m_properties["SpecMin"]);
    m_propTrees["SlicePropTree"]->addProperty(m_properties["SpecMax"]);
    m_propTrees["SlicePropTree"]->addProperty(m_properties["Range1"]);
    m_propTrees["SlicePropTree"]->addProperty(m_properties["UseTwoRanges"]);
    m_propTrees["SlicePropTree"]->addProperty(m_properties["Range2"]);

    // Create Slice Plot Widget for Range Selection
    m_plots["SlicePlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["SlicePlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_uiForm.slice_plot->addWidget(m_plots["SlicePlot"]);
    m_plots["SlicePlot"]->setCanvasBackground(Qt::white);
    // We always want one range selector... the second one can be controlled from
    // within the sliceTwoRanges(bool state) function
    connect(m_rangeSelectors["SliceRange1"], SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
    connect(m_rangeSelectors["SliceRange1"], SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));

    // second range
    // create the second range
    m_rangeSelectors["SliceRange2"]->setColour(Qt::darkGreen); // dark green for background
    connect(m_rangeSelectors["SliceRange1"], SIGNAL(rangeChanged(double, double)), m_rangeSelectors["SliceRange2"], SLOT(setRange(double, double)));
    connect(m_rangeSelectors["SliceRange2"], SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
    connect(m_rangeSelectors["SliceRange2"], SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));
    m_rangeSelectors["SliceRange2"]->setRange(m_rangeSelectors["SliceRange1"]->getRange());

    // Refresh the plot window
    m_plots["SlicePlot"]->replot();

    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(sliceUpdateRS(QtProperty*, double)));
    connect(m_blnManager, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(sliceTwoRanges(QtProperty*, bool)));

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
      "tofRange = [" + QString::number(m_dblManager->value(m_properties["R1S"])) + ","
      + QString::number(m_dblManager->value(m_properties["R1E"]));
    if ( m_blnManager->value(m_properties["UseTwoRanges"]) )
    {
      pyInput +=
        "," + QString::number(m_dblManager->value(m_properties["R2S"])) + ","
        + QString::number(m_dblManager->value(m_properties["R2E"])) + "]\n";
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
      "spectra = ["+ QString::number(m_dblManager->value(m_properties["SpecMin"])) + "," + QString::number(m_dblManager->value(m_properties["SpecMax"])) +"]\n"
      "suffix = '" + suffix + "'\n";

    if(m_uiForm.slice_ckVerbose->isChecked())
      pyInput += "verbose = True\n";
    else
      pyInput += "verbose = False\n";

    if(m_uiForm.slice_ckPlot->isChecked())
      pyInput += "plot = True\n";
    else
      pyInput += "plot = False\n";

    if(m_uiForm.slice_ckSave->isChecked())
      pyInput += "save = True\n";
    else
      pyInput += "save = False\n";

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

    auto rangeOne = std::make_pair(m_dblManager->value(m_properties["R1S"]), m_dblManager->value(m_properties["R1E"]));
    uiv.checkValidRange("Range One", rangeOne);

    bool useTwoRanges = m_blnManager->value(m_properties["UseTwoRanges"]);
    if( useTwoRanges )
    {
      auto rangeTwo = std::make_pair(m_dblManager->value(m_properties["R2S"]), m_dblManager->value(m_properties["R2E"]));
      uiv.checkValidRange("Range Two", rangeTwo);

      uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
    }

    auto specRange = std::make_pair(m_dblManager->value(m_properties["SpecMin"]), m_dblManager->value(m_properties["SpecMax"]));
    uiv.checkValidRange("Spectra Range", specRange);

    QString error = uiv.generateErrorMessage();

    if(error != "")
      g_log.warning(error.toStdString());

    return (error == "");
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

      plotMiniPlot(wsname, 0, "SlicePlot");
    }
    else
    {
      emit showMessageBox("Selected input files are invalid.");
    }

  }

  void IndirectDiagnostics::sliceTwoRanges(QtProperty*, bool state)
  {
    m_rangeSelectors["SliceRange2"]->setVisible(state);
  }

  void IndirectDiagnostics::sliceCalib(bool state)
  {
    m_uiForm.slice_calibFile->setEnabled(state);
    m_uiForm.slice_calibFile->isOptional(!state);
  }

  void IndirectDiagnostics::sliceMinChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_rangeSelectors["SliceRange1"] )
    {
      m_dblManager->setValue(m_properties["R1S"], val);
    }
    else if ( from == m_rangeSelectors["SliceRange2"] )
    {
      m_dblManager->setValue(m_properties["R2S"], val);
    }
  }

  void IndirectDiagnostics::sliceMaxChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());

    if ( from == m_rangeSelectors["SliceRange1"] )
      m_dblManager->setValue(m_properties["R1E"], val);
    else if ( from == m_rangeSelectors["SliceRange2"] )
      m_dblManager->setValue(m_properties["R2E"], val);
  }

  void IndirectDiagnostics::sliceUpdateRS(QtProperty* prop, double val)
  {
    if ( prop == m_properties["R1S"] )      m_rangeSelectors["SliceRange1"]->setMinimum(val);
    else if ( prop == m_properties["R1E"] ) m_rangeSelectors["SliceRange1"]->setMaximum(val);
    else if ( prop == m_properties["R2S"] ) m_rangeSelectors["SliceRange2"]->setMinimum(val);
    else if ( prop == m_properties["R2E"] ) m_rangeSelectors["SliceRange2"]->setMaximum(val);
  }

} // namespace CustomInterfaces
} // namespace Mantid
