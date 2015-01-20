#include "MantidQtCustomInterfaces/IndirectDiagnostics.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>

using namespace Mantid::API;

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
      IndirectDataReductionTab(uiForm, parent), m_lastDiagFilename("")
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

    // Preview plot
    m_plots["SlicePreviewPlot"] = new QwtPlot(m_parentWidget);
    m_plots["SlicePreviewPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["SlicePreviewPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["SlicePreviewPlot"]->setCanvasBackground(Qt::white);
    m_uiForm.slice_plotPreview->addWidget(m_plots["SlicePreviewPlot"]);
    m_plots["SlicePreviewPlot"]->replot();

    // SIGNAL/SLOT CONNECTIONS

    // Update instrument information when a new instrument config is selected
    connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(setDefaultInstDetails()));

    // Update properties when a range selector is changed
    connect(m_rangeSelectors["SlicePeak"], SIGNAL(selectionChangedLazy(double, double)), this, SLOT(rangeSelectorDropped(double, double)));
    connect(m_rangeSelectors["SliceBackground"], SIGNAL(selectionChangedLazy(double, double)), this, SLOT(rangeSelectorDropped(double, double)));

    // Update range selctors when a property is changed
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(sliceUpdateRS(QtProperty*, double)));
    // Enable/disable second range options when checkbox is toggled
    connect(m_blnManager, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(sliceTwoRanges(QtProperty*, bool)));
    // Enables/disables calibration file selection when user toggles Use Calibratin File checkbox
    connect(m_uiForm.slice_ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(sliceCalib(bool)));

    // Plot slice miniplot when file has finished loading
    connect(m_uiForm.slice_inputFile, SIGNAL(filesFound()), this, SLOT(slicePlotRaw()));
    // Shows message on run buton when user is inputting a run number
    connect(m_uiForm.slice_inputFile, SIGNAL(fileTextChanged(const QString &)), this, SLOT(pbRunEditing()));
    // Shows message on run button when Mantid is finding the file for a given run number
    connect(m_uiForm.slice_inputFile, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
    // Reverts run button back to normal when file finding has finished
    connect(m_uiForm.slice_inputFile, SIGNAL(fileFindingFinished()), this, SLOT(pbRunFinished()));

    // Update preview plot when slice algorithm completes
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(sliceAlgDone(bool)));

    // Set default UI state
    sliceTwoRanges(0, false);
    m_uiForm.slice_ckUseCalib->setChecked(false);
    sliceCalib(false);
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
    QString suffix = "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText() + "_slice";
    QString filenames = m_uiForm.slice_inputFile->getFilenames().join(",");

    std::vector<long> spectraRange;
    spectraRange.push_back(static_cast<long>(m_dblManager->value(m_properties["SpecMin"])));
    spectraRange.push_back(static_cast<long>(m_dblManager->value(m_properties["SpecMax"])));

    std::vector<double> peakRange;
    peakRange.push_back(m_dblManager->value(m_properties["PeakStart"]));
    peakRange.push_back(m_dblManager->value(m_properties["PeakEnd"]));

    IAlgorithm_sptr sliceAlg = AlgorithmManager::Instance().create("TimeSlice");
    sliceAlg->initialize();

    sliceAlg->setProperty("InputFiles", filenames.toStdString());
    sliceAlg->setProperty("SpectraRange", spectraRange);
    sliceAlg->setProperty("PeakRange", peakRange);
    sliceAlg->setProperty("Plot", m_uiForm.slice_ckPlot->isChecked());
    sliceAlg->setProperty("Save", m_uiForm.slice_ckSave->isChecked());
    sliceAlg->setProperty("OutputNameSuffix", suffix.toStdString());
    sliceAlg->setProperty("OutputWorkspace", "IndirectDiagnostics_Workspaces");

    if(m_uiForm.slice_ckUseCalib->isChecked())
    {
      QString calibWsName = m_uiForm.slice_dsCalibFile->getCurrentDataName();
      sliceAlg->setProperty("CalibrationWorkspace", calibWsName.toStdString());
    }

    if(m_blnManager->value(m_properties["UseTwoRanges"]))
    {
      std::vector<double> backgroundRange;
      backgroundRange.push_back(m_dblManager->value(m_properties["BackgroundStart"]));
      backgroundRange.push_back(m_dblManager->value(m_properties["BackgroundEnd"]));
      sliceAlg->setProperty("BackgroundRange", backgroundRange);
    }

    runAlgorithm(sliceAlg);
  }

  bool IndirectDiagnostics::validate()
  {
    UserInputValidator uiv;

    // Check raw input
    uiv.checkMWRunFilesIsValid("Input", m_uiForm.slice_inputFile);
    if(m_uiForm.slice_ckUseCalib->isChecked())
      uiv.checkMWRunFilesIsValid("Calibration", m_uiForm.slice_inputFile);

    // Check peak range
    auto rangeOne = std::make_pair(m_dblManager->value(m_properties["PeakStart"]), m_dblManager->value(m_properties["PeakEnd"]));
    uiv.checkValidRange("Range One", rangeOne);

    // Check background range
    bool useTwoRanges = m_blnManager->value(m_properties["UseTwoRanges"]);
    if(useTwoRanges)
    {
      auto rangeTwo = std::make_pair(m_dblManager->value(m_properties["BackgroundStart"]), m_dblManager->value(m_properties["BackgroundEnd"]));
      uiv.checkValidRange("Range Two", rangeTwo);

      uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
    }

    // Check spectra range
    auto specRange = std::make_pair(m_dblManager->value(m_properties["SpecMin"]), m_dblManager->value(m_properties["SpecMax"]) + 1);
    uiv.checkValidRange("Spectra Range", specRange);

    QString error = uiv.generateErrorMessage();
    bool isError = error != "";

    if(isError)
      g_log.warning(error.toStdString());

    return !isError;
  }

  /**
   * Sets default spectra, peak and background ranges.
   */
  void IndirectDiagnostics::setDefaultInstDetails()
  {
    //Get spectra, peak and background details
    std::map<QString, QString> instDetails = getInstrumentDetails();

    //Set spectra range
    m_dblManager->setValue(m_properties["SpecMin"], instDetails["spectra-min"].toDouble());
    m_dblManager->setValue(m_properties["SpecMax"], instDetails["spectra-max"].toDouble());

    //Set peak and background ranges
    if(instDetails.size() >= 8)
    {
      setMiniPlotGuides("SlicePeak", m_properties["PeakStart"], m_properties["PeakEnd"],
          std::pair<double, double>(instDetails["peak-start"].toDouble(), instDetails["peak-end"].toDouble()));
      setMiniPlotGuides("SliceBackground", m_properties["BackgroundStart"], m_properties["BackgroundEnd"],
          std::pair<double, double>(instDetails["back-start"].toDouble(), instDetails["back-end"].toDouble()));
    }
  }

  /**
   * Redraw the raw input plot
   */
  void IndirectDiagnostics::slicePlotRaw()
  {
    QString filename = m_uiForm.slice_inputFile->getFirstFilename();

    // Only update if we have a different file
    if(filename == m_lastDiagFilename)
      return;

    m_lastDiagFilename = filename;

    disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePreviewPlot()));
    disconnect(m_blnManager, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(updatePreviewPlot()));

    setDefaultInstDetails();

    if ( m_uiForm.slice_inputFile->isValid() )
    {
      QFileInfo fi(filename);
      QString wsname = fi.baseName();

      int specMin = static_cast<int>(m_dblManager->value(m_properties["SpecMin"]));
      int specMax = static_cast<int>(m_dblManager->value(m_properties["SpecMax"]));

      if(!loadFile(filename, wsname, specMin, specMax))
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

    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePreviewPlot()));
    connect(m_blnManager, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(updatePreviewPlot()));

    updatePreviewPlot();
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
    m_uiForm.slice_dsCalibFile->setEnabled(state);
  }

  void IndirectDiagnostics::rangeSelectorDropped(double min, double max)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());

    if(from == m_rangeSelectors["SlicePeak"])
    {
      m_dblManager->setValue(m_properties["PeakStart"], min);
      m_dblManager->setValue(m_properties["PeakEnd"], max);
    }
    else if(from == m_rangeSelectors["SliceBackground"])
    {
      m_dblManager->setValue(m_properties["BackgroundStart"], min);
      m_dblManager->setValue(m_properties["BackgroundEnd"], max);
    }
  }

  /**
   * Update the value of a range selector given a QtProperty
   *
   * @param prop :: Pointer to the QtProperty
   * @param val :: New value of the range selector
   */
  void IndirectDiagnostics::sliceUpdateRS(QtProperty* prop, double val)
  {
    if(prop == m_properties["PeakStart"])             m_rangeSelectors["SlicePeak"]->setMinimum(val);
    else if(prop == m_properties["PeakEnd"])          m_rangeSelectors["SlicePeak"]->setMaximum(val);
    else if(prop == m_properties["BackgroundStart"])  m_rangeSelectors["SliceBackground"]->setMinimum(val);
    else if(prop == m_properties["BackgroundEnd"])    m_rangeSelectors["SliceBackground"]->setMaximum(val);
  }

  /**
   * Runs the slice algorithm with preview properties.
   */
  void IndirectDiagnostics::updatePreviewPlot()
  {
    QString suffix = "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText() + "_slice";
    QString filenames = m_uiForm.slice_inputFile->getFilenames().join(",");

    std::vector<long> spectraRange;
    spectraRange.push_back(static_cast<long>(m_dblManager->value(m_properties["SpecMin"])));
    spectraRange.push_back(static_cast<long>(m_dblManager->value(m_properties["SpecMax"])));

    std::vector<double> peakRange;
    peakRange.push_back(m_dblManager->value(m_properties["PeakStart"]));
    peakRange.push_back(m_dblManager->value(m_properties["PeakEnd"]));

    IAlgorithm_sptr sliceAlg = AlgorithmManager::Instance().create("TimeSlice");
    sliceAlg->initialize();

    sliceAlg->setProperty("InputFiles", filenames.toStdString());
    sliceAlg->setProperty("SpectraRange", spectraRange);
    sliceAlg->setProperty("PeakRange", peakRange);
    sliceAlg->setProperty("Plot", false);
    sliceAlg->setProperty("Save", false);
    sliceAlg->setProperty("OutputNameSuffix", suffix.toStdString());
    sliceAlg->setProperty("OutputWorkspace", "IndirectDiagnostics_Workspaces");

    if(m_uiForm.slice_ckUseCalib->isChecked())
    {
      QString calibWsName = m_uiForm.slice_dsCalibFile->getCurrentDataName();
      sliceAlg->setProperty("CalibrationWorkspace", calibWsName.toStdString());
    }

    if(m_blnManager->value(m_properties["UseTwoRanges"]))
    {
      std::vector<double> backgroundRange;
      backgroundRange.push_back(m_dblManager->value(m_properties["BackgroundStart"]));
      backgroundRange.push_back(m_dblManager->value(m_properties["BackgroundEnd"]));
      sliceAlg->setProperty("BackgroundRange", backgroundRange);
    }

    // Stop the algorithm conflicting with it's self if it is already running
    if(m_batchAlgoRunner->queueLength() == 0)
      runAlgorithm(sliceAlg);
  }

  /**
   * Updates the preview plot when the algorithm is complete.
   *
   * @param error True if the algorithm was stopped due to error, false otherwise
   */
  void IndirectDiagnostics::sliceAlgDone(bool error)
  {
    if(error)
      return;

    QStringList filenames = m_uiForm.slice_inputFile->getFilenames();
    if(filenames.size() < 1)
      return;

    WorkspaceGroup_sptr sliceOutputGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IndirectDiagnostics_Workspaces");
    if(sliceOutputGroup->size() == 0)
    {
      g_log.warning("No result workspaces, cannot plot preview.");
      return;
    }

    MatrixWorkspace_sptr sliceWs = boost::dynamic_pointer_cast<MatrixWorkspace>(sliceOutputGroup->getItem(0));
    if(!sliceWs)
    {
      g_log.warning("No result workspaces, cannot plot preview.");
      return;
    }

    // Set workspace for Python export as the first result workspace
    m_pythonExportWsName = sliceWs->getName();

    // Plot result spectrum
    plotMiniPlot(sliceWs, 0, "SlicePreviewPlot", "SlicePreviewCurve");

    // Set X range to data range
    setXAxisToCurve("SlicePreviewPlot", "SlicePreviewCurve");
    m_plots["SlicePreviewPlot"]->replot();

    // Ungroup the output workspace
    sliceOutputGroup->removeAll();
    AnalysisDataService::Instance().remove("IndirectDiagnostics_Workspaces");
  }

  /**
   * Called when a user starts to type / edit the runs to load.
   */
  void IndirectDiagnostics::pbRunEditing()
  {
    emit updateRunButton(false, "Editing...", "Run numbers are curently being edited.");
  }

  /**
   * Called when the FileFinder starts finding the files.
   */
  void IndirectDiagnostics::pbRunFinding()
  {
    emit updateRunButton(false, "Finding files...", "Searchig for data files for the run numbers entered...");
    m_uiForm.slice_inputFile->setEnabled(false);
  }

  /**
   * Called when the FileFinder has finished finding the files.
   */
  void IndirectDiagnostics::pbRunFinished()
  {
    if(!m_uiForm.slice_inputFile->isValid())
    {
      emit updateRunButton(false, "Invalid Run(s)", "Cannot find data files for some of the run numbers enetered.");
    }
    else
    {
      emit updateRunButton();
    }

    m_uiForm.slice_inputFile->setEnabled(true);
  }

} // namespace CustomInterfaces
} // namespace Mantid
