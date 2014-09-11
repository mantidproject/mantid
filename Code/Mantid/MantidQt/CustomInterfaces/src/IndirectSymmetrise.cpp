#include "MantidQtCustomInterfaces/IndirectSymmetrise.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QFileInfo>

namespace
{
  Mantid::Kernel::Logger g_log("IndirectSymmetrise");
}

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectSymmetrise::IndirectSymmetrise(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent)
  {
    // Property Trees
    m_propTrees["SymmPropTree"] = new QtTreePropertyBrowser();
    m_uiForm.symm_properties->addWidget(m_propTrees["SymmPropTree"]);

    m_propTrees["SymmPVPropTree"] = new QtTreePropertyBrowser();
    m_uiForm.symm_previewProperties->addWidget(m_propTrees["SymmPVPropTree"]);

    // Editor Factories
    DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory();
    m_propTrees["SymmPropTree"]->setFactoryForManager(m_dblManager, doubleEditorFactory);

    // Raw Properties
    m_properties["EMin"] = m_dblManager->addProperty("EMin");
    m_propTrees["SymmPropTree"]->addProperty(m_properties["EMin"]);
    m_properties["EMax"] = m_dblManager->addProperty("EMax");
    m_propTrees["SymmPropTree"]->addProperty(m_properties["EMax"]);

    QtProperty* rawPlotProps = m_grpManager->addProperty("Raw Plot");
    m_propTrees["SymmPropTree"]->addProperty(rawPlotProps);

    m_properties["PreviewSpec"] = m_dblManager->addProperty("Spectrum No");
    m_dblManager->setDecimals(m_properties["PreviewSpec"], 0);
    rawPlotProps->addSubProperty(m_properties["PreviewSpec"]);

    m_properties["PreviewRange"] = m_dblManager->addProperty("X Range");
    rawPlotProps->addSubProperty(m_properties["PreviewRange"]);

    // Preview Properties
    // Mainly used for display rather than getting user input
    m_properties["NegativeYValue"] = m_dblManager->addProperty("Negative Y");
    m_propTrees["SymmPVPropTree"]->addProperty(m_properties["NegativeYValue"]);

    m_properties["PositiveYValue"] = m_dblManager->addProperty("Positive Y");
    m_propTrees["SymmPVPropTree"]->addProperty(m_properties["PositiveYValue"]);

    m_properties["DeltaY"] = m_dblManager->addProperty("Delta Y");
    m_propTrees["SymmPVPropTree"]->addProperty(m_properties["DeltaY"]);

    // Raw plot
    m_plots["SymmRawPlot"] = new QwtPlot(m_parentWidget);
    m_curves["SymmRawPlot"] = new QwtPlotCurve();

    // Indicators for negative and positive XCut values on X axis
    m_rangeSelectors["NegativeXCut_Raw"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, false);
    m_rangeSelectors["PositiveXCut_Raw"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, false);

    m_rangeSelectors["NegativeXCut_Raw"]->setColour(Qt::darkGreen);
    m_rangeSelectors["PositiveXCut_Raw"]->setColour(Qt::darkGreen);

    // Indicators for Y value at each XCut position
    m_rangeSelectors["NegativeXCutYPos"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::YSINGLE, true, false);
    m_rangeSelectors["PositiveXCutYPos"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::YSINGLE, true, false);

    m_rangeSelectors["NegativeXCutYPos"]->setColour(Qt::red);
    m_rangeSelectors["PositiveXCutYPos"]->setColour(Qt::blue);
    m_rangeSelectors["NegativeXCutYPos"]->setMinimum(0);
    m_rangeSelectors["PositiveXCutYPos"]->setMinimum(0);

    // Indicator for centre of symmetry (x=0)
    m_rangeSelectors["CentreMark_Raw"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, true);
    m_rangeSelectors["CentreMark_Raw"]->setColour(Qt::cyan);
    m_rangeSelectors["CentreMark_Raw"]->setMinimum(0.0);

    m_plots["SymmRawPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["SymmRawPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["SymmRawPlot"]->setCanvasBackground(Qt::white);
    m_uiForm.symm_plot->addWidget(m_plots["SymmRawPlot"]);

    // Preview plot
    m_plots["SymmPreviewPlot"] = new QwtPlot(m_parentWidget);
    m_curves["SymmPreviewPlot"] = new QwtPlotCurve();

    // Indicators for negative and positive XCut values on X axis
    m_rangeSelectors["NegativeXCut_PV"] = new MantidWidgets::RangeSelector(m_plots["SymmPreviewPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, true);
    m_rangeSelectors["PositiveXCut_PV"] = new MantidWidgets::RangeSelector(m_plots["SymmPreviewPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, true);

    m_rangeSelectors["NegativeXCut_PV"]->setColour(Qt::darkGreen);
    m_rangeSelectors["PositiveXCut_PV"]->setColour(Qt::darkGreen);

    // Indicator for centre of symmetry (x=0)
    m_rangeSelectors["CentreMark_PV"] = new MantidWidgets::RangeSelector(m_plots["SymmPreviewPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, true);
    m_rangeSelectors["CentreMark_PV"]->setColour(Qt::cyan);
    m_rangeSelectors["CentreMark_PV"]->setMinimum(0.0);

    m_plots["SymmPreviewPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
    m_plots["SymmPreviewPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
    m_plots["SymmPreviewPlot"]->setCanvasBackground(Qt::white);
    m_uiForm.symm_previewPlot->addWidget(m_plots["SymmPreviewPlot"]);

    // Refresh the plot windows
    m_plots["SymmRawPlot"]->replot();
    m_plots["SymmPreviewPlot"]->replot();

    // SIGNAL/SLOT CONNECTIONS
    // Update range selctors when a property is changed
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRangeSelectors(QtProperty*, double)));
    // Plot a new spectrum when the user changes the value of the preview spectrum
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(replotNewSpectrum(QtProperty*, double)));
    // Plot miniplot when file has finished loading
    connect(m_uiForm.symm_dsInput, SIGNAL(dataReady(const QString&)), this, SLOT(plotRawInput(const QString&)));
    // Preview symmetrise
    connect(m_uiForm.symm_previewButton, SIGNAL(clicked()), this, SLOT(preview()));

    // Set default XCut value
    m_dblManager->setValue(m_properties["EMin"], 0.1);
    m_dblManager->setValue(m_properties["EMax"], 0.5);

    // Set default x axis range
    std::pair<double, double> defaultRange(-1.0, 1.0);
    setAxisRange("SymmRawPlot", QwtPlot::xBottom, defaultRange);
    setAxisRange("SymmPreviewPlot", QwtPlot::xBottom, defaultRange);
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectSymmetrise::~IndirectSymmetrise()
  {
  }

  void IndirectSymmetrise::setup()
  {
  }

  bool IndirectSymmetrise::validate()
  {
    // Check for a valid input file
    if(!m_uiForm.symm_dsInput->isValid())
      return false;

    // EMin and EMax must be positive
    if(m_dblManager->value(m_properties["EMin"]) <= 0.0)
      return false;
    if(m_dblManager->value(m_properties["EMax"]) <= 0.0)
      return false;

    return true;
  }

  void IndirectSymmetrise::run()
  {
    QString workspaceName = m_uiForm.symm_dsInput->getCurrentDataName();
    QString outputWorkspaceName = workspaceName.left(workspaceName.length() - 4) + "_Symmetrise";

    bool plot = m_uiForm.symm_ckPlot->isChecked();
    bool verbose = m_uiForm.symm_ckVerbose->isChecked();
    bool save = m_uiForm.symm_ckSave->isChecked();

    double e_min = m_dblManager->value(m_properties["XEMin"]);
    double e_max = m_dblManager->value(m_properties["XEMax"]);

    IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise", -1);
    symmetriseAlg->initialize();
    symmetriseAlg->setProperty("Sample", workspaceName.toStdString());
    symmetriseAlg->setProperty("XMin", e_min);
    symmetriseAlg->setProperty("XMax", e_max);
    symmetriseAlg->setProperty("Plot", plot);
    symmetriseAlg->setProperty("Verbose", verbose);
    symmetriseAlg->setProperty("Save", save);
    symmetriseAlg->setProperty("OutputWorkspace", outputWorkspaceName.toStdString());

    // Execute algorithm on seperate thread
    runAlgorithm(symmetriseAlg);
  }

  /**
   * Plots a new workspace in the mini plot when it is loaded form the data selector.
   *
   * @param workspaceName Name of the workspace that has been laoded
   */
  void IndirectSymmetrise::plotRawInput(const QString &workspaceName)
  {
    // Set the preview spectrum number to the first spectrum in the workspace
    MatrixWorkspace_sptr sampleWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
    int minSpectrumRange = sampleWS->getSpectrum(0)->getSpectrumNo();
    m_dblManager->setValue(m_properties["PreviewSpec"], static_cast<double>(minSpectrumRange));

    updateMiniPlots();

    // Set the preview range to the maximum absolute X value
    auto axisRange = getCurveRange("SymmRawPlot");
    double symmRange = std::max(fabs(axisRange.first), fabs(axisRange.second));
    g_log.information() << "Symmetrise x axis range +/- " << symmRange << std::endl;
    m_dblManager->setValue(m_properties["PreviewRange"], symmRange);

    updateMiniPlots();
  }

  /**
   * Updates the mini plots.
   */
  void IndirectSymmetrise::updateMiniPlots()
  {
    if(!m_uiForm.symm_dsInput->isValid())
      return;

    QString workspaceName = m_uiForm.symm_dsInput->getCurrentDataName();
    int spectrumNumber = static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"]));

    Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName.toStdString()));

    // Set the X axis range based on the range specified by the user
    std::pair<double, double>range;
    range.first = -m_dblManager->value(m_properties["PreviewRange"]);
    range.second = m_dblManager->value(m_properties["PreviewRange"]);
    setAxisRange("SymmRawPlot", QwtPlot::xBottom, range);

    // Plot the spectrum chosen by the user
    size_t spectrumIndex = input->getIndexFromSpectrumNumber(spectrumNumber);
    plotMiniPlot(input, spectrumIndex, "SymmRawPlot");

    // Match X axis range on preview plot
    setAxisRange("SymmPreviewPlot", QwtPlot::xBottom, range);
    m_plots["SymmPreviewPlot"]->replot();
  }

  /**
   * Redraws mini plots when user changes previw range or spectrum.
   */
  void IndirectSymmetrise::replotNewSpectrum(QtProperty *prop, double value)
  {
    UNUSED_ARG(value);

    if((prop == m_properties["PreviewSpec"]) || (prop == m_properties["PreviewRange"]))
      updateMiniPlots();
  }

  /**
   * Updates position of XCut range selectors when used changed value of XCut.
   */
  void IndirectSymmetrise::updateRangeSelectors(QtProperty *prop, double value)
  {
    if(prop == m_properties["EMin"])
    {
      m_rangeSelectors["NegativeXCut_Raw"]->setMinimum(-value);
      m_rangeSelectors["PositiveXCut_Raw"]->setMinimum(value);

      m_rangeSelectors["NegativeXCut_PV"]->setMinimum(-value);
      m_rangeSelectors["PositiveXCut_PV"]->setMinimum(value);
    }

    if(prop == m_properties["EMax"])
    {
    }
  }

  /**
   * Handles a request to preview the symmetrise.
   *
   * Runs Symmetrise on the current spectrum and plots in preview mini plot.
   *
   * @see IndirectSymmetrise::previewAlgDone()
   */
  void IndirectSymmetrise::preview()
  {
    // Handle algorithm completion signal
    // TODO: Temp. removal to checkbuild #10092
    /* connect(&m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(previewAlgDone(bool))); */

    // Do nothing if no data has been laoded
    QString workspaceName = m_uiForm.symm_dsInput->getCurrentDataName();
    if(workspaceName.isEmpty())
      return;

    bool verbose = m_uiForm.symm_ckVerbose->isChecked();
    double e_min = m_dblManager->value(m_properties["EMin"]);
    double e_max = m_dblManager->value(m_properties["EMax"]);
    long spectrumNumber = static_cast<long>(m_dblManager->value(m_properties["PreviewSpec"]));
    std::vector<long> spectraRange(2, spectrumNumber);

    // Run the algorithm on the preview spectrum only
    IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise", -1);
    symmetriseAlg->initialize();
    symmetriseAlg->setProperty("Sample", workspaceName.toStdString());
    symmetriseAlg->setProperty("XMin", e_min);
    symmetriseAlg->setProperty("XMax", e_max);
    symmetriseAlg->setProperty("Plot", false);
    symmetriseAlg->setProperty("Verbose", verbose);
    symmetriseAlg->setProperty("Save", false);
    symmetriseAlg->setProperty("SpectraRange", spectraRange);
    symmetriseAlg->setProperty("OutputWorkspace", "__Symmetrise_temp");
    symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

    runAlgorithm(symmetriseAlg);
  }

  /**
   * Handles completion of the preview algorithm.
   */
  void IndirectSymmetrise::previewAlgDone(bool error)
  {
    if(error)
      return;

    QString workspaceName = m_uiForm.symm_dsInput->getCurrentDataName();
    int spectrumNumber = static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"]));

    MatrixWorkspace_sptr sampleWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
    ITableWorkspace_sptr propsTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("__SymmetriseProps_temp");
    MatrixWorkspace_sptr symmWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("__Symmetrise_temp");

    // Get the index of XCut on each side of zero
    int negativeIndex = propsTable->getColumn("NegativeCutIndex")->cell<int>(0);
    int positiveIndex = propsTable->getColumn("PositiveCutIndex")->cell<int>(0);

    // Get the Y values for each XCut and the difference between them
    double negativeY = sampleWS->dataY(0)[negativeIndex];
    double positiveY = sampleWS->dataY(0)[positiveIndex];
    double deltaY = fabs(negativeY - positiveY);

    // Show values in property tree
    m_dblManager->setValue(m_properties["NegativeYValue"], negativeY);
    m_dblManager->setValue(m_properties["PositiveYValue"], positiveY);
    m_dblManager->setValue(m_properties["DeltaY"], deltaY);

    // Set indicator positions
    m_rangeSelectors["NegativeXCutYPos"]->setMinimum(negativeY);
    m_rangeSelectors["PositiveXCutYPos"]->setMinimum(positiveY);

    // Plot preview plot
    size_t spectrumIndex = symmWS->getIndexFromSpectrumNumber(spectrumNumber);
    plotMiniPlot("__Symmetrise_temp", spectrumIndex, "SymmPreviewPlot");

    // Don't want this to trigger when the algorithm is run for all spectra
    // TODO: Temp. removal to checkbuild #10092
    /* disconnect(&m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(previewAlgDone(bool))); */
  }

} // namespace CustomInterfaces
} // namespace Mantid
