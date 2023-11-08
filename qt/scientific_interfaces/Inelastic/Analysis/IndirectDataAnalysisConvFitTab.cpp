// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisConvFitTab.h"
#include "ConvFitAddWorkspaceDialog.h"
#include "ConvFitDataPresenter.h"
#include "FunctionBrowser/ConvTemplateBrowser.h"
#include "IndirectFitPlotView.h"

using namespace Mantid::API;

namespace {

std::vector<std::string> CONVFIT_HIDDEN_PROPS = std::vector<std::string>(
    {"CreateOutput", "LogValue", "PassWSIndexToFunction", "OutputWorkspace", "Output", "PeakRadius", "PlotParameter"});

auto FUNCTION_STRINGS = std::unordered_map<std::string, std::string>({{"Lorentzian", "L"},
                                                                      {"StretchedExpFT", "SFT"},
                                                                      {"TeixeiraWaterSQE", "TxWater"},
                                                                      {"FickDiffusionSQE", "FickDiff"},
                                                                      {"ChudleyElliotSQE", "ChudElliot"},
                                                                      {"HallRossSQE", "HallRoss"},
                                                                      {"DiffRotDiscreteCircle", "DC"},
                                                                      {"ElasticDiffRotDiscreteCircle", "EDC"},
                                                                      {"InelasticDiffRotDiscreteCircle", "IDC"},
                                                                      {"DiffSphere", "DS"},
                                                                      {"ElasticDiffSphere", "EDS"},
                                                                      {"InelasticDiffSphere", "IDS"},
                                                                      {"IsoRotDiff", "IRD"},
                                                                      {"ElasticIsoRotDiff", "EIRD"},
                                                                      {"InelasticIsoRotDiff", "IIRD"}});

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisConvFitTab::IndirectDataAnalysisConvFitTab(QWidget *parent)
    : IndirectDataAnalysisTab(new ConvFitModel, new ConvTemplateBrowser, new ConvFitDataView, CONVFIT_HIDDEN_PROPS,
                              parent) {
  setConvolveMembers(true);
}

EstimationDataSelector IndirectDataAnalysisConvFitTab::getEstimationDataSelector() const {
  return [](const Mantid::MantidVec &, const Mantid::MantidVec &,
            const std::pair<double, double>) -> DataForParameterEstimation { return DataForParameterEstimation{}; };
}

void IndirectDataAnalysisConvFitTab::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto convDialog = dynamic_cast<ConvFitAddWorkspaceDialog const *>(dialog)) {
    m_dataPresenter->addWorkspace(convDialog->workspaceName(), convDialog->workspaceIndices());
    m_dataPresenter->setResolution(convDialog->resolutionName());
    m_fittingModel->addDefaultParameters();
  }
}

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
  for (auto fitFunctionName : FUNCTION_STRINGS) {
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

} // namespace MantidQt::CustomInterfaces::IDA
