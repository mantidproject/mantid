// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisConvFitTab.h"
#include "ConvFitAddWorkspaceDialog.h"
#include "ConvFitDataPresenter.h"
#include "IndirectFitPlotView.h"
#include "IndirectFunctionBrowser/ConvTemplateBrowser.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"

#include <QDoubleValidator>
#include <QMenu>

using namespace Mantid;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ConvFit");

std::vector<std::string> CONVFIT_HIDDEN_PROPS =
    std::vector<std::string>({"CreateOutput", "LogValue", "PassWSIndexToFunction", "OutputWorkspace",
                              "IgnoreInvalidData", "Output", "PeakRadius", "PlotParameter"});
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataAnalysisConvFitTab::IndirectDataAnalysisConvFitTab(QWidget *parent)
    : IndirectFitAnalysisTab(new ConvFitModel, parent), m_uiForm(new Ui::IndirectFitTab) {
  m_uiForm->setupUi(parent);
  m_convFittingModel = dynamic_cast<ConvFitModel *>(getFittingModel());
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);
  m_uiForm->dockArea->m_fitPropertyBrowser->setFunctionTemplateBrowser(new ConvTemplateBrowser);
  setFitPropertyBrowser(m_uiForm->dockArea->m_fitPropertyBrowser);
  m_uiForm->dockArea->m_fitPropertyBrowser->setHiddenProperties(CONVFIT_HIDDEN_PROPS);

  m_uiForm->dockArea->setFitDataView(new ConvFitDataView(m_uiForm->dockArea));
  auto dataPresenter =
      std::make_unique<ConvFitDataPresenter>(m_convFittingModel->getFitDataModel(), m_uiForm->dockArea->m_fitDataView);
  setFitDataPresenter(std::move(dataPresenter));
  setPlotView(m_uiForm->dockArea->m_fitPlotView);

  setEditResultVisible(true);
}

void IndirectDataAnalysisConvFitTab::setupFitTab() {
  setConvolveMembers(true);

  // Initialise fitTypeStrings
  m_fitStrings["Lorentzian"] = "L";
  m_fitStrings["StretchedExpFT"] = "SFT";
  m_fitStrings["TeixeiraWaterSQE"] = "TxWater";
  m_fitStrings["DiffRotDiscreteCircle"] = "DC";
  m_fitStrings["ElasticDiffRotDiscreteCircle"] = "EDC";
  m_fitStrings["InelasticDiffRotDiscreteCircle"] = "IDC";
  m_fitStrings["DiffSphere"] = "DS";
  m_fitStrings["ElasticDiffSphere"] = "EDS";
  m_fitStrings["InelasticDiffSphere"] = "IDS";
  m_fitStrings["IsoRotDiff"] = "IRD";
  m_fitStrings["ElasticIsoRotDiff"] = "EIRD";
  m_fitStrings["InelasticIsoRotDiff"] = "IIRD";

  auto &functionFactory = FunctionFactory::Instance();
  auto lorentzian = functionFactory.createFunction("Lorentzian");
  auto stretchedExpFT = functionFactory.createFunction("StretchedExpFT");
  auto teixeiraWater = functionFactory.createFunction("TeixeiraWaterSQE");

  auto diffSphere = functionFactory.createFunction("DiffSphere");
  auto elasticDiffSphere = functionFactory.createFunction("ElasticDiffSphere");
  auto inelasticDiffSphere = functionFactory.createFunction("InelasticDiffSphere");

  auto diffRotDiscCircle = functionFactory.createFunction("DiffRotDiscreteCircle");
  auto elasticDiffRotDiscCircle = functionFactory.createFunction("ElasticDiffRotDiscreteCircle");
  auto inelasticDiffRotDiscCircle = functionFactory.createFunction("InelasticDiffRotDiscreteCircle");

  auto isoRotDiff = functionFactory.createFunction("IsoRotDiff");
  auto elasticIsoRotDiff = functionFactory.createFunction("ElasticIsoRotDiff");
  auto inelasticIsoRotDiff = functionFactory.createFunction("InelasticIsoRotDiff");

  auto deltaFunction = functionFactory.createFunction("DeltaFunction");

  // Instrument resolution
  m_properties["InstrumentResolution"] = m_dblManager->addProperty("InstrumentResolution");

  // Post Plot and Save
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
}

void IndirectDataAnalysisConvFitTab::setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  IndirectFitAnalysisTab::setupFit(fitAlgorithm);
}

EstimationDataSelector IndirectDataAnalysisConvFitTab::getEstimationDataSelector() const {
  return [](const MantidVec &, const MantidVec &, const std::pair<double, double>) -> DataForParameterEstimation {
    return DataForParameterEstimation{};
  };
}

void IndirectDataAnalysisConvFitTab::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto convDialog = dynamic_cast<ConvFitAddWorkspaceDialog const *>(dialog)) {
    m_dataPresenter->addWorkspace(convDialog->workspaceName(), convDialog->workspaceIndices());
    m_dataPresenter->setResolution(convDialog->resolutionName());
    m_convFittingModel->addDefaultParameters();
  }
}

void IndirectDataAnalysisConvFitTab::fitFunctionChanged() { m_convFittingModel->setFitTypeString(getFitTypeString()); }

/**
 * Generate a string to describe the fit type selected by the user.
 * Used when naming the resultant workspaces.
 *
 * Assertions used to guard against any future changes that don't take
 * workspace naming into account.
 *
 * @returns the generated string.
 */
std::string IndirectDataAnalysisConvFitTab::getFitTypeString() const {
  std::string fitType;
  for (auto fitFunctionName : m_fitStrings) {
    auto occurances = getNumberOfCustomFunctions(fitFunctionName.first);
    if (occurances > 0) {
      fitType += std::to_string(occurances) + fitFunctionName.second;
    }
  }

  if (getNumberOfCustomFunctions("DeltaFunction") > 0) {
    fitType += "Delta";
  }

  return fitType;
}

void IndirectDataAnalysisConvFitTab::runClicked() { runTab(); }

void IndirectDataAnalysisConvFitTab::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void IndirectDataAnalysisConvFitTab::setRunEnabled(bool enable) { m_uiForm->pbRun->setEnabled(enable); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
