// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFit.h"
#include "ConvFitDataPresenter.h"
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
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

ConvFit::ConvFit(QWidget *parent)
    : IndirectFitAnalysisTab(new ConvFitModel, parent),
      m_uiForm(new Ui::ConvFit) {
  m_uiForm->setupUi(parent);
  m_convFittingModel = dynamic_cast<ConvFitModel *>(fittingModel());
  setPlotView(m_uiForm->pvFitPlotView);
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setOutputOptionsView(m_uiForm->ovOutputOptionsView);
  m_uiForm->fitPropertyBrowser->setFunctionTemplateBrowser(
      new ConvTemplateBrowser);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);
  auto dataPresenter = std::make_unique<ConvFitDataPresenter>(
      m_convFittingModel, m_uiForm->fitDataView);
  connect(
      dataPresenter.get(),
      SIGNAL(
          modelResolutionAdded(std::string const &, TableDatasetIndex const &)),
      this,
      SLOT(setModelResolution(std::string const &, TableDatasetIndex const &)));
  setFitDataPresenter(std::move(dataPresenter));

  setEditResultVisible(true);
  setStartAndEndHidden(false);
}

void ConvFit::setupFitTab() {
  setConvolveMembers(true);

  // Initialise fitTypeStrings
  m_fitStrings["None"] = "";
  m_fitStrings["One Lorentzian"] = "1L";
  m_fitStrings["Two Lorentzians"] = "2L";
  m_fitStrings["InelasticDiffSphere"] = "IDS";
  m_fitStrings["InelasticDiffRotDiscreteCircle"] = "IDC";
  m_fitStrings["ElasticDiffSphere"] = "EDS";
  m_fitStrings["ElasticDiffRotDiscreteCircle"] = "EDC";
  m_fitStrings["StretchedExpFT"] = "SFT";
  m_fitStrings["Teixeira Water"] = "TxWater";

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

  // Instrument resolution
  m_properties["InstrumentResolution"] =
      m_dblManager->addProperty("InstrumentResolution");

  // Post Plot and Save
  connect(m_uiForm->pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
}

void ConvFit::setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  IndirectFitAnalysisTab::setupFit(fitAlgorithm);
}

EstimationDataSelector ConvFit::getEstimationDataSelector() const {
  return
      [](const MantidVec &, const MantidVec &) -> DataForParameterEstimation {
        return DataForParameterEstimation{};
      };
}

void ConvFit::setModelResolution(const std::string &resolutionName) {
  setModelResolution(resolutionName, TableDatasetIndex{0});
}

void ConvFit::setModelResolution(const std::string &resolutionName,
                                 TableDatasetIndex index) {
  const auto resolution =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          resolutionName);
  m_convFittingModel->setResolution(resolution, index);
  auto fitResolutions = m_convFittingModel->getResolutionsForFit();
  m_uiForm->fitPropertyBrowser->setModelResolution(fitResolutions);
  setModelFitFunction();
}

void ConvFit::setStartAndEndHidden(bool hidden) {
  m_uiForm->fitDataView->setStartAndEndHidden(hidden);
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

void ConvFit::runClicked() { runTab(); }

void ConvFit::setRunIsRunning(bool running) {
  m_uiForm->pbRun->setText(running ? "Running..." : "Run");
}

void ConvFit::setRunEnabled(bool enable) {
  m_uiForm->pbRun->setEnabled(enable);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
