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
    // Property Tree
    m_propTrees["SlicePropTree"] = new QtTreePropertyBrowser();
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

    m_properties["PeakStart"] = m_dblManager->addProperty("Start");
    m_properties["PeakEnd"] = m_dblManager->addProperty("End");

    m_properties["BackgroundStart"] = m_dblManager->addProperty("Start");
    m_properties["BackgroundEnd"] = m_dblManager->addProperty("End");

    m_properties["UseTwoRanges"] = m_blnManager->addProperty("Use Two Ranges");

    m_properties["Range1"] = m_grpManager->addProperty("Peak");
    m_properties["Range1"]->addSubProperty(m_properties["PeakStart"]);
    m_properties["Range1"]->addSubProperty(m_properties["PeakEnd"]);

    m_properties["Range2"] = m_grpManager->addProperty("Background");
    m_properties["Range2"]->addSubProperty(m_properties["BackgroundStart"]);
    m_properties["Range2"]->addSubProperty(m_properties["BackgroundEnd"]);

    m_propTrees["SlicePropTree"]->addProperty(m_properties["SpecMin"]);
    m_propTrees["SlicePropTree"]->addProperty(m_properties["SpecMax"]);
    m_propTrees["SlicePropTree"]->addProperty(m_properties["Range1"]);
    m_propTrees["SlicePropTree"]->addProperty(m_properties["UseTwoRanges"]);
    m_propTrees["SlicePropTree"]->addProperty(m_properties["Range2"]);

    // Slice plot
    m_plots["SlicePlot"] = new QwtPlot(m_parentWidget);
    m_curves["SlicePlot"] = new QwtPlotCurve();
    m_rangeSelectors["SlicePeak"] = new MantidWidgets::RangeSelector(m_plots["SlicePlot"]);
    m_rangeSelectors["SliceBackground"] = new MantidWidgets::RangeSelector(m_plots["SlicePlot"]);

    m_plots["SlicePlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["SlicePlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["SlicePlot"]->setCanvasBackground(Qt::white);
    m_uiForm.slice_plot->addWidget(m_plots["SlicePlot"]);

    // Setup second range
    m_rangeSelectors["SliceBackground"]->setColour(Qt::darkGreen); // Dark green for background
    m_rangeSelectors["SliceBackground"]->setRange(m_rangeSelectors["SlicePeak"]->getRange());

    // Refresh the plot window
    m_plots["SlicePlot"]->replot();

    // SIGNAL/SLOT CONNECTIONS
    /* connect(m_rangeSelectors["SlicePeak"], SIGNAL(rangeChanged(double, double)), m_rangeSelectors["SliceBackground"], SLOT(setRange(double, double))); */

    // Update properties when a range selector is changed
    connect(m_rangeSelectors["SlicePeak"], SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
    connect(m_rangeSelectors["SlicePeak"], SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));
    connect(m_rangeSelectors["SliceBackground"], SIGNAL(minValueChanged(double)), this, SLOT(sliceMinChanged(double)));
    connect(m_rangeSelectors["SliceBackground"], SIGNAL(maxValueChanged(double)), this, SLOT(sliceMaxChanged(double)));
    // Update range seelctors when a property is changed
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(sliceUpdateRS(QtProperty*, double)));
    // Enable/disable second range options when checkbox is toggled
    connect(m_blnManager, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(sliceTwoRanges(QtProperty*, bool)));
    // Plot slice miniplot when file has finished loading
    connect(m_uiForm.slice_inputFile, SIGNAL(filesFound()), this, SLOT(slicePlotRaw()));
    // Plot slice miniplot when user clicks Plot Raw
    connect(m_uiForm.slice_pbPlotRaw, SIGNAL(clicked()), this, SLOT(slicePlotRaw()));
    // Enables/disables calibration file selection when user toggles Use Calibratin File checkbox
    connect(m_uiForm.slice_ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(sliceCalib(bool)));

    // Set default UI state
    sliceTwoRanges(0, false);
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
      "tofRange = [" + QString::number(m_dblManager->value(m_properties["PeakStart"])) + ","
      + QString::number(m_dblManager->value(m_properties["PeakEnd"]));
    if ( m_blnManager->value(m_properties["UseTwoRanges"]) )
    {
      pyInput +=
        "," + QString::number(m_dblManager->value(m_properties["BackgroundStart"])) + ","
        + QString::number(m_dblManager->value(m_properties["BackgroundEnd"])) + "]\n";
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

    auto rangeOne = std::make_pair(m_dblManager->value(m_properties["PeakStart"]), m_dblManager->value(m_properties["PeakEnd"]));
    uiv.checkValidRange("Range One", rangeOne);

    bool useTwoRanges = m_blnManager->value(m_properties["UseTwoRanges"]);
    if( useTwoRanges )
    {
      auto rangeTwo = std::make_pair(m_dblManager->value(m_properties["BackgroundStart"]), m_dblManager->value(m_properties["BackgroundEnd"]));
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

  /**
   * Sets default spectra, peak and background ranges
   */
  void IndirectDiagnostics::setDefaultInstDetails()
  {
    //Get spectra, peak and background details
    std::map<QString, QString> instDetails = getInstrumentDetails();

    //Set spectra range
    m_dblManager->setValue(m_properties["SpecMin"], instDetails["SpectraMin"].toDouble());
    m_dblManager->setValue(m_properties["SpecMax"], instDetails["SpectraMax"].toDouble());

    //Set peak and background ranges
    if(instDetails.size() >= 8)
    {
      setMiniPlotGuides("SlicePeak", m_properties["PeakStart"], m_properties["PeakEnd"],
          std::pair<double, double>(instDetails["PeakMin"].toDouble(), instDetails["PeakMax"].toDouble()));
      setMiniPlotGuides("SliceBackground", m_properties["BackStart"], m_properties["BackEnd"],
          std::pair<double, double>(instDetails["BackMin"].toDouble(), instDetails["BackMax"].toDouble()));
    }
  }

  /**
   * Redraw the raw input plot
   */
  void IndirectDiagnostics::slicePlotRaw()
  {
    using namespace Mantid::API;

    setDefaultInstDetails();

    if ( m_uiForm.slice_inputFile->isValid() )
    {
      QString filename = m_uiForm.slice_inputFile->getFirstFilename();
      QFileInfo fi(filename);
      QString wsname = fi.baseName();

      if(!loadFile(filename, wsname, m_uiForm.leSpectraMin->text().toInt(), m_uiForm.leSpectraMax->text().toInt()))
      {
        emit showMessageBox("Unable to load file.\nCheck whether your file exists and matches the selected instrument in the EnergyTransfer tab.");
        return;
      }

      Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

      const Mantid::MantidVec & dataX = input->readX(0);
      std::pair<double, double> range(dataX.front(), dataX.back());

      plotMiniPlot(input, 0, "SlicePlot");
      setXAxisToCurve("SlicePlot", "SlicePlot");

      setPlotRange("SlicePeak", m_properties["PeakStart"], m_properties["PeakEnd"], range);
      setPlotRange("SliceBackground", m_properties["BackgroundStart"], m_properties["BackgroundEnd"], range);

      replot("SlicePlot");
    }
    else
    {
      emit showMessageBox("Selected input files are invalid.");
    }
  }

  /**
   * Set if the second slice range selectors should be shown on the plot
   *
   * @param state :: True to show the second range selectors, false to hide
   */
  void IndirectDiagnostics::sliceTwoRanges(QtProperty*, bool state)
  {
    m_rangeSelectors["SliceBackground"]->setVisible(state);
  }

  /**
   * Enables/disables the calibration file field and validator
   *
   * @param state :: True to enable calibration file, false otherwise
   */
  void IndirectDiagnostics::sliceCalib(bool state)
  {
    m_uiForm.slice_calibFile->setEnabled(state);
    m_uiForm.slice_calibFile->isOptional(!state);
  }

  /**
   * Handles the value of a range selector minimum value being changed
   *
   * @param val :: New minimum value
   */
  void IndirectDiagnostics::sliceMinChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());

    if ( from == m_rangeSelectors["SlicePeak"] )
      m_dblManager->setValue(m_properties["PeakStart"], val);
    else if ( from == m_rangeSelectors["SliceBackground"] )
      m_dblManager->setValue(m_properties["BackgroundStart"], val);
  }

  /**
   * Handles the value of a range selector maximum value being changed
   *
   * @param val :: New maximum value
   */
  void IndirectDiagnostics::sliceMaxChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());

    if ( from == m_rangeSelectors["SlicePeak"] )
      m_dblManager->setValue(m_properties["PeakEnd"], val);
    else if ( from == m_rangeSelectors["SliceBackground"] )
      m_dblManager->setValue(m_properties["BackgroundEnd"], val);
  }

  /**
   * Update the value of a range selector given a QtProperty
   *
   * @param prop :: Pointer to the QtProperty
   * @param val :: New value of the range selector
   */
  void IndirectDiagnostics::sliceUpdateRS(QtProperty* prop, double val)
  {
    if ( prop == m_properties["PeakStart"] )      m_rangeSelectors["SlicePeak"]->setMinimum(val);
    else if ( prop == m_properties["PeakEnd"] ) m_rangeSelectors["SlicePeak"]->setMaximum(val);
    else if ( prop == m_properties["BackgroundStart"] ) m_rangeSelectors["SliceBackground"]->setMinimum(val);
    else if ( prop == m_properties["BackgroundEnd"] ) m_rangeSelectors["SliceBackground"]->setMaximum(val);
  }

} // namespace CustomInterfaces
} // namespace Mantid
