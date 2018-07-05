#include "IqtFit.h"

#include "../General/UserInputValidator.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include <boost/algorithm/string/predicate.hpp>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IqtFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IqtFit::IqtFit(QWidget *parent)
    : IndirectFitAnalysisTab(new IqtFitModel, parent),
      m_uiForm(new Ui::IqtFit) {
  m_uiForm->setupUi(parent);
  m_iqtFittingModel = dynamic_cast<IqtFitModel *>(fittingModel());

  setFitDataPresenter(Mantid::Kernel::make_unique<IndirectFitDataPresenter>(
      m_iqtFittingModel, m_uiForm->fitDataView));
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
}

void IqtFit::setupFitTab() {
  setSampleWSSuffices({"_iqt"});
  setSampleFBSuffices({"_iqt.nxs"});

  // Create custom function groups
  auto &functionFactory = FunctionFactory::Instance();
  const auto exponential = functionFactory.createFunction("ExpDecay");
  const auto stretchedExponential =
      functionFactory.createFunction("StretchExp");
  addSpinnerFunctionGroup("Exponential", {exponential}, 0, 2);
  addCheckBoxFunctionGroup("Stretched Exponential", {stretchedExponential});

  // Add custom settings
  addBoolCustomSetting("ConstrainIntensities", "Constrain Intensities");
  addBoolCustomSetting("ConstrainBeta", "Make Beta Global");
  addBoolCustomSetting("ExtractMembers", "Extract Members");
  setCustomSettingEnabled("ConstrainBeta", false);
  setCustomSettingEnabled("ConstrainIntensities", false);

  // Set available background options
  setBackgroundOptions({"None", "FlatBackground"});

  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotResult()));
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));

  connect(this, SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
          SLOT(parameterUpdated(const Mantid::API::IFunction *)));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
  connect(this, SIGNAL(customBoolChanged(const QString &, bool)), this,
          SLOT(customBoolUpdated(const QString &, bool)));
}

void IqtFit::fitFunctionChanged() {
  if (numberOfCustomFunctions("StretchExp") > 0) {
    setCustomSettingEnabled("ConstrainBeta", true);
  } else {
    setCustomBoolSetting("ConstrainBeta", false);
    setCustomSettingEnabled("ConstrainBeta", false);
  }
  setConstrainIntensitiesEnabled(m_iqtFittingModel->canConstrainIntensities());
  m_iqtFittingModel->setFitTypeString(fitTypeString());
}

void IqtFit::setConstrainIntensitiesEnabled(bool enabled) {
  setCustomSettingEnabled("ConstrainIntensities", enabled);
  if (!enabled)
    setCustomBoolSetting("ConstrainIntensities", false);
  else if (boolSettingValue("ConstrainIntensities")) {
    if (m_iqtFittingModel->setConstrainIntensities(true))
      updateTies();
  }
}

void IqtFit::customBoolUpdated(const QString &key, bool value) {
  if (key == "Constrain Intensities") {
    if (m_iqtFittingModel->setConstrainIntensities(value))
      updateTies();
  }
}

std::string IqtFit::fitTypeString() const {
  const auto numberOfExponential = numberOfCustomFunctions("ExpDecay");
  const auto numberOfStretched = numberOfCustomFunctions("StretchExp");

  if (numberOfExponential > 0)
    return std::to_string(numberOfExponential) + "E";

  if (numberOfStretched > 0)
    return std::to_string(numberOfStretched) + "S";

  return "";
}

void IqtFit::updatePlotOptions() {
  IndirectFitAnalysisTab::updatePlotOptions(m_uiForm->cbPlotType);
}

void IqtFit::setPlotResultEnabled(bool enabled) {
  m_uiForm->pbPlot->setEnabled(enabled);
  m_uiForm->cbPlotType->setEnabled(enabled);
}

void IqtFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

void IqtFit::setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  fitAlgorithm->setProperty("ExtractMembers",
                            boolSettingValue("ExtractMembers"));
  IndirectFitAnalysisTab::setupFit(fitAlgorithm);
}

void IqtFit::plotResult() {
  IndirectFitAnalysisTab::plotResult(m_uiForm->cbPlotType->currentText());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
