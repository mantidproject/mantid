#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomInterfaces/MSDFit.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("MSDFit");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  MSDFit::MSDFit(QWidget * parent) : IDATab(parent),
    m_currentWsName(""), m_msdTree(NULL)
  {
  }

  void MSDFit::setup()
  {
    // Tree Browser
    m_msdTree = new QtTreePropertyBrowser();
    uiForm().msd_properties->addWidget(m_msdTree);

    m_msdTree->setFactoryForManager(m_dblManager, doubleEditorFactory());

    m_properties["Start"] = m_dblManager->addProperty("StartX");
    m_dblManager->setDecimals(m_properties["Start"], NUM_DECIMALS);
    m_properties["End"] = m_dblManager->addProperty("EndX");
    m_dblManager->setDecimals(m_properties["End"], NUM_DECIMALS);

    m_msdTree->addProperty(m_properties["Start"]);
    m_msdTree->addProperty(m_properties["End"]);

    m_plots["MSDPlot"] = new QwtPlot(m_parentWidget);
    uiForm().msd_plot->addWidget(m_plots["MSDPlot"]);

    // Cosmetics
    m_plots["MSDPlot"]->setAxisFont(QwtPlot::xBottom, m_parentWidget->font());
    m_plots["MSDPlot"]->setAxisFont(QwtPlot::yLeft, m_parentWidget->font());
    m_plots["MSDPlot"]->setCanvasBackground(Qt::white);

    m_rangeSelectors["MSDRange"] = new MantidWidgets::RangeSelector(m_plots["MSDPlot"]);

    connect(m_rangeSelectors["MSDRange"], SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_rangeSelectors["MSDRange"], SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));

    connect(uiForm().msd_dsSampleInput, SIGNAL(dataReady(const QString&)), this, SLOT(newDataLoaded(const QString&)));
    connect(uiForm().msd_pbSingle, SIGNAL(clicked()), this, SLOT(singleFit()));
    connect(uiForm().msd_spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(plotInput()));

    connect(uiForm().msd_spSpectraMin, SIGNAL(valueChanged(int)), this, SLOT(specMinChanged(int)));
    connect(uiForm().msd_spSpectraMax, SIGNAL(valueChanged(int)), this, SLOT(specMaxChanged(int)));
  }

  void MSDFit::run()
  {
    QString pyInput =
    "from IndirectDataAnalysis import msdfit\n"
    "startX = " + QString::number(m_dblManager->value(m_properties["Start"])) +"\n"
    "endX = " + QString::number(m_dblManager->value(m_properties["End"])) +"\n"
    "specMin = " + uiForm().msd_spSpectraMin->text() + "\n"
    "specMax = " + uiForm().msd_spSpectraMax->text() + "\n"
    "input = '" + uiForm().msd_dsSampleInput->getCurrentDataName() + "'\n";

    if ( uiForm().msd_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( uiForm().msd_ckPlot->isChecked() ) pyInput += "plot = True\n";
    else pyInput += "plot = False\n";

    if ( uiForm().msd_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput +=
      "msdfit(input, startX, endX, spec_min=specMin, spec_max=specMax, Save=save, Verbose=verbose, Plot=plot)\n";

    QString pyOutput = runPythonCode(pyInput);
    UNUSED_ARG(pyOutput);

    // Set the result workspace for Python script export
    QString dataName = uiForm().msd_dsSampleInput->getCurrentDataName();
    m_pythonExportWsName = dataName.left(dataName.lastIndexOf("_")).toStdString() + "_msd";
  }

  void MSDFit::singleFit()
  {
    if(!validate())
      return;

    QString pyInput =
      "from IndirectDataAnalysis import msdfit\n"
      "startX = " + QString::number(m_dblManager->value(m_properties["Start"])) +"\n"
      "endX = " + QString::number(m_dblManager->value(m_properties["End"])) +"\n"
      "specMin = " + uiForm().msd_spPlotSpectrum->text() + "\n"
      "specMax = " + uiForm().msd_spPlotSpectrum->text() + "\n"
      "input = '" + uiForm().msd_dsSampleInput->getCurrentDataName() + "'\n";

    if ( uiForm().msd_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    pyInput +=
      "output = msdfit(input, startX, endX, spec_min=specMin, spec_max=specMax, Save=False, Verbose=verbose, Plot=False)\n"
      "print output \n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
    plotFit(pyOutput);
  }

  bool MSDFit::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample input", uiForm().msd_dsSampleInput);

    auto range = std::make_pair(m_dblManager->value(m_properties["Start"]), m_dblManager->value(m_properties["End"]));
    uiv.checkValidRange("a range", range);

    int specMin = uiForm().msd_spSpectraMin->value();
    int specMax = uiForm().msd_spSpectraMax->value();
    auto specRange = std::make_pair(specMin, specMax+1);
    uiv.checkValidRange("spectrum range", specRange);

    QString errors = uiv.generateErrorMessage();
    showMessageBox(errors);

    return errors.isEmpty();
  }

  void MSDFit::loadSettings(const QSettings & settings)
  {
    uiForm().msd_dsSampleInput->readSettings(settings.group());
  }

  void MSDFit::plotFit(QString wsName)
  {
    if(Mantid::API::AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    {
      //read the fit from the workspace
      auto groupWs = Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::WorkspaceGroup>(wsName.toStdString());
      auto ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(groupWs->getItem(0));
      plotMiniPlot(ws, 1, "MSDPlot", "MSDFitCurve");
      QPen fitPen(Qt::red, Qt::SolidLine);
      m_curves["MSDFitCurve"]->setPen(fitPen);
      replot("MSDPlot");
    }
  }

  /**
   * Called when new data has been loaded by the data selector.
   *
   * Configures ranges for spin boxes before raw plot is done.
   *
   * @param wsName Name of new workspace loaded
   */
  void MSDFit::newDataLoaded(const QString wsName)
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsName.toStdString());
    int maxSpecIndex = static_cast<int>(ws->getNumberHistograms()) - 1;

    uiForm().msd_spPlotSpectrum->setMaximum(maxSpecIndex);
    uiForm().msd_spPlotSpectrum->setMinimum(0);
    uiForm().msd_spPlotSpectrum->setValue(0);

    uiForm().msd_spSpectraMin->setMaximum(maxSpecIndex);
    uiForm().msd_spSpectraMin->setMinimum(0);

    uiForm().msd_spSpectraMax->setMaximum(maxSpecIndex);
    uiForm().msd_spSpectraMax->setMinimum(0);
    uiForm().msd_spSpectraMax->setValue(maxSpecIndex);

    plotInput();
  }

  void MSDFit::plotInput()
  {
    QString wsname = uiForm().msd_dsSampleInput->getCurrentDataName();

    if(!AnalysisDataService::Instance().doesExist(wsname.toStdString()))
    {
      g_log.error("No workspace loaded, cannot create preview plot.");
      return;
    }

    auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsname.toStdString());

    int wsIndex = uiForm().msd_spPlotSpectrum->value();
    plotMiniPlot(ws, wsIndex, "MSDPlot", "MSDDataCurve");

    try
    {
      const std::pair<double, double> range = getCurveRange("MSDDataCurve");
      m_rangeSelectors["MSDRange"]->setRange(range.first, range.second);

      // Replot
      replot("MSDPlot");
    }
    catch(std::invalid_argument & exc)
    {
      showMessageBox(exc.what());
    }

    m_currentWsName = wsname;

    // Remove the old fit curve
    removeCurve("MSDFitCurve");
    replot("MSDPlot");
  }

  /**
   * Handles the user entering a new minimum spectrum index.
   *
   * Prevents the user entering an overlapping spectra range.
   *
   * @param value Minimum spectrum index
   */
  void MSDFit::specMinChanged(int value)
  {
    uiForm().msd_spSpectraMax->setMinimum(value);
  }

  /**
   * Handles the user entering a new maximum spectrum index.
   *
   * Prevents the user entering an overlapping spectra range.
   *
   * @param value Maximum spectrum index
   */
  void MSDFit::specMaxChanged(int value)
  {
    uiForm().msd_spSpectraMin->setMaximum(value);
  }

  void MSDFit::minChanged(double val)
  {
    m_dblManager->setValue(m_properties["Start"], val);
  }

  void MSDFit::maxChanged(double val)
  {
    m_dblManager->setValue(m_properties["End"], val);
  }

  void MSDFit::updateRS(QtProperty* prop, double val)
  {
    if ( prop == m_properties["Start"] ) m_rangeSelectors["MSDRange"]->setMinimum(val);
    else if ( prop == m_properties["End"] ) m_rangeSelectors["MSDRange"]->setMaximum(val);
  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
