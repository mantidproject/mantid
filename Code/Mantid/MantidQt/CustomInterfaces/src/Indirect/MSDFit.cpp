#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomInterfaces/Indirect/MSDFit.h"
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
    m_uiForm.setupUi(parent);
  }

  void MSDFit::setup()
  {
    // Tree Browser
    m_msdTree = new QtTreePropertyBrowser();
    m_uiForm.properties->addWidget(m_msdTree);

    m_msdTree->setFactoryForManager(m_dblManager, m_dblEdFac);

    m_properties["Start"] = m_dblManager->addProperty("StartX");
    m_dblManager->setDecimals(m_properties["Start"], NUM_DECIMALS);
    m_properties["End"] = m_dblManager->addProperty("EndX");
    m_dblManager->setDecimals(m_properties["End"], NUM_DECIMALS);

    m_msdTree->addProperty(m_properties["Start"]);
    m_msdTree->addProperty(m_properties["End"]);

    m_rangeSelectors["MSDRange"] = new MantidWidgets::RangeSelector(m_uiForm.ppPlot);

    connect(m_rangeSelectors["MSDRange"], SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_rangeSelectors["MSDRange"], SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));

    connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString&)), this, SLOT(newDataLoaded(const QString&)));
    connect(m_uiForm.pbSingleFit, SIGNAL(clicked()), this, SLOT(singleFit()));
    connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(plotInput()));

    connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this, SLOT(specMinChanged(int)));
    connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this, SLOT(specMaxChanged(int)));
  }

  void MSDFit::run()
  {
    QString pyInput =
    "from IndirectDataAnalysis import msdfit\n"
    "startX = " + QString::number(m_dblManager->value(m_properties["Start"])) +"\n"
    "endX = " + QString::number(m_dblManager->value(m_properties["End"])) +"\n"
    "specMin = " + m_uiForm.spSpectraMin->text() + "\n"
    "specMax = " + m_uiForm.spSpectraMax->text() + "\n"
    "input = '" + m_uiForm.dsSampleInput->getCurrentDataName() + "'\n";

    if ( m_uiForm.ckPlot->isChecked() ) pyInput += "plot = True\n";
    else pyInput += "plot = False\n";

    if ( m_uiForm.ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput +=
      "msdfit(input, startX, endX, spec_min=specMin, spec_max=specMax, Save=save, Plot=plot)\n";

    QString pyOutput = runPythonCode(pyInput);
    UNUSED_ARG(pyOutput);

    // Set the result workspace for Python script export
    QString dataName = m_uiForm.dsSampleInput->getCurrentDataName();
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
      "specMin = " + m_uiForm.spPlotSpectrum->text() + "\n"
      "specMax = " + m_uiForm.spPlotSpectrum->text() + "\n"
      "input = '" + m_uiForm.dsSampleInput->getCurrentDataName() + "'\n";

    pyInput +=
      "output = msdfit(input, startX, endX, spec_min=specMin, spec_max=specMax, Save=False, Plot=False)\n"
      "print output \n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
    plotFit(pyOutput);
  }

  bool MSDFit::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample input", m_uiForm.dsSampleInput);

    auto range = std::make_pair(m_dblManager->value(m_properties["Start"]), m_dblManager->value(m_properties["End"]));
    uiv.checkValidRange("a range", range);

    int specMin = m_uiForm.spSpectraMin->value();
    int specMax = m_uiForm.spSpectraMax->value();
    auto specRange = std::make_pair(specMin, specMax+1);
    uiv.checkValidRange("spectrum range", specRange);

    QString errors = uiv.generateErrorMessage();
    showMessageBox(errors);

    return errors.isEmpty();
  }

  void MSDFit::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsSampleInput->readSettings(settings.group());
  }

  void MSDFit::plotFit(QString wsName)
  {
    if(Mantid::API::AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    {
      // Get the workspace
      auto groupWs = Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::WorkspaceGroup>(wsName.toStdString());
      auto ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(groupWs->getItem(0));

      // Remove the old fit
      m_uiForm.ppPlot->removeSpectrum("Fit");

      // Plot the new fit
      m_uiForm.ppPlot->addSpectrum("Fit", ws, 1, Qt::red);
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

    m_uiForm.spPlotSpectrum->setMaximum(maxSpecIndex);
    m_uiForm.spPlotSpectrum->setMinimum(0);
    m_uiForm.spPlotSpectrum->setValue(0);

    m_uiForm.spSpectraMin->setMaximum(maxSpecIndex);
    m_uiForm.spSpectraMin->setMinimum(0);

    m_uiForm.spSpectraMax->setMaximum(maxSpecIndex);
    m_uiForm.spSpectraMax->setMinimum(0);
    m_uiForm.spSpectraMax->setValue(maxSpecIndex);

    plotInput();
  }

  void MSDFit::plotInput()
  {
    m_uiForm.ppPlot->clear();

    QString wsname = m_uiForm.dsSampleInput->getCurrentDataName();

    if(!AnalysisDataService::Instance().doesExist(wsname.toStdString()))
    {
      g_log.error("No workspace loaded, cannot create preview plot.");
      return;
    }

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname.toStdString());

    int wsIndex = m_uiForm.spPlotSpectrum->value();
    m_uiForm.ppPlot->addSpectrum("Sample", ws, wsIndex);

    try
    {
      QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
      m_rangeSelectors["MSDRange"]->setRange(range.first, range.second);
    }
    catch(std::invalid_argument & exc)
    {
      showMessageBox(exc.what());
    }

    m_currentWsName = wsname;
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
    m_uiForm.spSpectraMax->setMinimum(value);
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
    m_uiForm.spSpectraMin->setMaximum(value);
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
