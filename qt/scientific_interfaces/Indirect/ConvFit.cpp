#include "ConvFit.h"
#include "ConvFitDataPresenter.h"

#include "../General/UserInputValidator.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"

#include <QDoubleValidator>
#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ConvFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

ConvFit::ConvFit(QWidget *parent)
    : IndirectFitAnalysisTab(new ConvFitModel, parent),
      m_uiForm(new Ui::ConvFit) {
  m_uiForm->setupUi(parent);
  m_convFittingModel = dynamic_cast<ConvFitModel *>(fittingModel());

  setFitDataPresenter(Mantid::Kernel::make_unique<ConvFitDataPresenter>(
      m_convFittingModel, m_uiForm->fitDataView));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
}

void ConvFit::setupFitTab() {
  setDefaultPeakType("Lorentzian");
  setConvolveMembers(true);

  setSampleWSSuffices({"_red", "_sqw"});
  setSampleFBSuffices({"_red.nxs", "_sqw.nxs"});
  setResolutionWSSuffices({"_res", "_red", "_sqw"});
  setResolutionFBSuffices({"_res.nxs", "_red.nxs", "_sqw.nxs"});

  // Initialise fitTypeStrings
  m_fitStrings["None"] = "";
  m_fitStrings["One Lorentzian"] = "1L";
  m_fitStrings["Two Lorentzians"] = "2L";
  m_fitStrings["InelasticDiffSphere"] = "IDS";
  m_fitStrings["InelasticDiffRotDiscreteCircle"] = "IDC";
  m_fitStrings["ElasticDiffSphere"] = "EDS";
  m_fitStrings["ElasticDiffRotDiscreteCircle"] = "EDC";
  m_fitStrings["StretchedExpFT"] = "SFT";
  m_fitStrings["TeixeiraWaterSQE"] = "Teixeria1L";

  auto &functionFactory = FunctionFactory::Instance();
  auto lorentzian = functionFactory.createFunction("Lorentzian");
  auto teixeiraWater = functionFactory.createFunction("TeixeiraWaterSQE");

  auto elasticDiffSphere = functionFactory.createFunction("ElasticDiffSphere");
  auto inelasticDiffSphere =
      functionFactory.createFunction("InelasticDiffSphere");

  auto elasticDiffRotDiscCircle =
      functionFactory.createFunction("ElasticDiffRotDiscreteCircle");
  auto inelasticDiffRotDiscCircle =
      functionFactory.createFunction("InelasticDiffRotDiscreteCircle");

  auto stretchedExpFT = functionFactory.createFunction("StretchedExpFT");

  auto deltaFunction = functionFactory.createFunction("DeltaFunction");

  addCheckBoxFunctionGroup("Use Delta Function", {deltaFunction});

  addComboBoxFunctionGroup("One Lorentzian", {lorentzian});
  addComboBoxFunctionGroup("Two Lorentzians", {lorentzian, lorentzian});
  addComboBoxFunctionGroup("Teixeira Water", {teixeiraWater});
  addComboBoxFunctionGroup("InelasticDiffSphere", {inelasticDiffSphere});
  addComboBoxFunctionGroup("InelasticDiffRotDiscreteCircle",
                           {inelasticDiffRotDiscCircle});
  addComboBoxFunctionGroup("ElasticDiffSphere", {elasticDiffSphere});
  addComboBoxFunctionGroup("ElasticDiffRotDiscreteCircle",
                           {elasticDiffRotDiscCircle});
  addComboBoxFunctionGroup("StretchedExpFT", {stretchedExpFT});

  // Set available background options
  setBackgroundOptions({"None", "FlatBackground", "LinearBackground"});

  addBoolCustomSetting("ExtractMembers", "Extract Members");
  addOptionalDoubleSetting("TempCorrection", "Temp. Correction",
                           "UseTempCorrection", "Use Temp. Correction");
  setCustomSettingChangesFunction("TempCorrection", true);
  setCustomSettingChangesFunction("UseTempCorrection", true);

  // Instrument resolution
  m_properties["InstrumentResolution"] =
      m_dblManager->addProperty("InstrumentResolution");

  // Post Plot and Save
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
}

void ConvFit::setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  if (boolSettingValue("UseTempCorrection"))
    m_convFittingModel->setTemperature(doubleSettingValue("TempCorrection"));
  else
    m_convFittingModel->setTemperature(boost::none);
  fitAlgorithm->setProperty("ExtractMembers",
                            boolSettingValue("ExtractMembers"));
  IndirectFitAnalysisTab::setupFit(fitAlgorithm);
}

void ConvFit::setModelResolution(const QString &resolutionName) {
  const auto name = resolutionName.toStdString();
  const auto resolution =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
  m_convFittingModel->setResolution(resolution, 0);
  setModelFitFunction();
}

/**
 * Handles saving the workspace when save is clicked
 */
void ConvFit::saveClicked() { IndirectFitAnalysisTab::saveResult(); }

/**
 * Handles plotting the workspace when plot is clicked
 */
void ConvFit::plotClicked() {
  setPlotResultIsPlotting(true);
  IndirectFitAnalysisTab::plotResult(m_uiForm->cbPlotType->currentText());
  setPlotResultIsPlotting(false);
}

void ConvFit::updatePlotOptions() {
  IndirectFitAnalysisTab::updatePlotOptions(m_uiForm->cbPlotType);
}

void ConvFit::fitFunctionChanged() {
  m_convFittingModel->setFitTypeString(fitTypeString());
}

/**
 * Generate a string to describe the fit type selected by the user.
 * Used when naming the resultant workspaces.
 *
 * Assertions used to guard against any future changes that don't take
 * workspace naming into account.
 *
 * @returns the generated QString.
 */
std::string ConvFit::fitTypeString() const {
  std::string fitType;

  if (numberOfCustomFunctions("DeltaFunction") > 0)
    fitType += "Delta";

  fitType += m_fitStrings[selectedFitType()];

  return fitType;
}

void ConvFit::setRunEnabled(bool enabled) {
  m_uiForm->pbRun->setEnabled(enabled);
}

void ConvFit::setPlotResultEnabled(bool enabled) {
  m_uiForm->pbPlot->setEnabled(enabled);
  m_uiForm->cbPlotType->setEnabled(enabled);
}

void ConvFit::setFitSingleSpectrumEnabled(bool enabled) {
  m_uiForm->pvFitPlotView->enableFitSingleSpectrum(enabled);
}

void ConvFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

void ConvFit::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
  setRunEnabled(!running);
  setFitSingleSpectrumEnabled(!running);
}

void ConvFit::setPlotResultIsPlotting(bool plotting) {
  m_uiForm->pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setPlotResultEnabled(!plotting);
}

void ConvFit::runClicked() { runTab(); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
