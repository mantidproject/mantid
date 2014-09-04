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
    m_properties["XCut"] = m_dblManager->addProperty("X Cut");
    m_propTrees["SymmPropTree"]->addProperty(m_properties["XCut"]);

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

    m_rangeSelectors["NegativeXCut_Raw"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, false);
    m_rangeSelectors["PositiveXCut_Raw"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, false);

    m_rangeSelectors["NegativeXCutYPos"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::YSINGLE, true, false);
    m_rangeSelectors["PositiveXCutYPos"] = new MantidWidgets::RangeSelector(m_plots["SymmRawPlot"],
        MantidWidgets::RangeSelector::YSINGLE, true, false);

    m_rangeSelectors["NegativeXCutYPos"]->setColour(Qt::red);
    m_rangeSelectors["PositiveXCutYPos"]->setColour(Qt::red);
    m_rangeSelectors["NegativeXCutYPos"]->setMinimum(0);
    m_rangeSelectors["PositiveXCutYPos"]->setMinimum(0);

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

    m_rangeSelectors["NegativeXCut_PV"] = new MantidWidgets::RangeSelector(m_plots["SymmPreviewPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, true);
    m_rangeSelectors["PositiveXCut_PV"] = new MantidWidgets::RangeSelector(m_plots["SymmPreviewPlot"],
        MantidWidgets::RangeSelector::XSINGLE, true, true);

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
    m_dblManager->setValue(m_properties["XCut"], 0.3);

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

    // Check for a valid XCut value
    if(m_dblManager->value(m_properties["XCut"]) <= 0.0)
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

    double x_cut = m_dblManager->value(m_properties["XCut"]);

    IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise", -1);
    symmetriseAlg->initialize();
    symmetriseAlg->setProperty("Sample", workspaceName.toStdString());
    symmetriseAlg->setProperty("XCut", x_cut);
    symmetriseAlg->setProperty("Plot", plot);
    symmetriseAlg->setProperty("Verbose", verbose);
    symmetriseAlg->setProperty("Save", save);
    symmetriseAlg->setProperty("OutputWorkspace", outputWorkspaceName.toStdString());

    // Execute algorithm on seperate thread
    runAlgorithm(symmetriseAlg);
  }

  void IndirectSymmetrise::plotRawInput(const QString &workspaceName)
  {
    UNUSED_ARG(workspaceName);

    updateRawPlot();

    auto axisRange = getCurveRange("SymmRawPlot");
    double symmRange = std::max(fabs(axisRange.first), fabs(axisRange.second));
    g_log.information() << "Symmetrise x axis range +/- " << symmRange << std::endl;
    m_dblManager->setValue(m_properties["PreviewRange"], symmRange);

    updateRawPlot();
  }

  void IndirectSymmetrise::updateRawPlot()
  {
    if(!m_uiForm.symm_dsInput->isValid())
      return;

    QString workspaceName = m_uiForm.symm_dsInput->getCurrentDataName();
    int spectrumNumber = static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"]));

    Mantid::API::MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName.toStdString()));

    std::pair<double, double>range;
    range.first = -m_dblManager->value(m_properties["PreviewRange"]);
    range.second = m_dblManager->value(m_properties["PreviewRange"]);
    setAxisRange("SymmRawPlot", QwtPlot::xBottom, range);

    plotMiniPlot(input, spectrumNumber, "SymmRawPlot");

    // Match X axis range on preview plot
    setAxisRange("SymmPreviewPlot", QwtPlot::xBottom, range);
    m_plots["SymmPreviewPlot"]->replot();
  }

  void IndirectSymmetrise::replotNewSpectrum(QtProperty *prop, double value)
  {
    UNUSED_ARG(value);

    if((prop == m_properties["PreviewSpec"]) || (prop == m_properties["PreviewRange"]))
      updateRawPlot();
  }

  void IndirectSymmetrise::updateRangeSelectors(QtProperty *prop, double value)
  {
    if(prop == m_properties["XCut"])
    {
      m_rangeSelectors["NegativeXCut_Raw"]->setMinimum(-value);
      m_rangeSelectors["PositiveXCut_Raw"]->setMinimum(value);

      m_rangeSelectors["NegativeXCut_PV"]->setMinimum(-value);
      m_rangeSelectors["PositiveXCut_PV"]->setMinimum(value);
    }
  }

  void IndirectSymmetrise::preview()
  {
    connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(previewAlgDone(bool)));

    QString workspaceName = m_uiForm.symm_dsInput->getCurrentDataName();

    if(workspaceName.isEmpty())
      return;

    double x_cut = m_dblManager->value(m_properties["XCut"]);

    IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise", -1);
    symmetriseAlg->initialize();
    symmetriseAlg->setProperty("Sample", workspaceName.toStdString());
    symmetriseAlg->setProperty("XCut", x_cut);
    symmetriseAlg->setProperty("Plot", false);
    symmetriseAlg->setProperty("Verbose", false);
    symmetriseAlg->setProperty("Save", false);
    symmetriseAlg->setProperty("OutputWorkspace", "__Symmetrise_temp");
    symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

    runAlgorithm(symmetriseAlg);
  }

  void IndirectSymmetrise::previewAlgDone(bool error)
  {
    if(error)
      return;

    QString workspaceName = m_uiForm.symm_dsInput->getCurrentDataName();
    int spectrumNumber = static_cast<int>(m_dblManager->value(m_properties["PreviewSpec"]));

    MatrixWorkspace_sptr sampleWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
    ITableWorkspace_sptr propsTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("__SymmetriseProps_temp");

    int negativeIndex = propsTable->getColumn("NegativeCutIndex")->cell<int>(0);
    int positiveIndex = propsTable->getColumn("PositiveCutIndex")->cell<int>(0);

    double negativeY = sampleWS->dataY(0)[negativeIndex];
    double positiveY = sampleWS->dataY(0)[positiveIndex];
    double deltaY = fabs(negativeY - positiveY);

    m_dblManager->setValue(m_properties["NegativeYValue"], negativeY);
    m_dblManager->setValue(m_properties["PositiveYValue"], positiveY);
    m_dblManager->setValue(m_properties["DeltaY"], deltaY);

    m_rangeSelectors["NegativeXCutYPos"]->setMinimum(negativeY);
    m_rangeSelectors["PositiveXCutYPos"]->setMinimum(positiveY);

    plotMiniPlot("__Symmetrise_temp", spectrumNumber, "SymmPreviewPlot");

    disconnect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(previewAlgDone(bool)));
  }

} // namespace CustomInterfaces
} // namespace Mantid
