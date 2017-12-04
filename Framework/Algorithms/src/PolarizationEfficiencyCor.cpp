#include "MantidAlgorithms/PolarizationEfficiencyCor.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"

#include <Eigen/Dense>

namespace {
namespace Prop {
constexpr char *FLIPPERS{"Flippers"};
constexpr char *EFFICIENCIES{"Efficiencies"};
constexpr char *INPUT_WS{"InputWorkspace"};
constexpr char *OUTPUT_WS{"OutputWorkspace"};
}
namespace Flippers {
constexpr char *OffOff{"00"};
constexpr char *OffOn{"01"};
constexpr char *OnOff{"10"};
constexpr char *OnOn{"11"};
}

}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PolarizationEfficiencyCor)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PolarizationEfficiencyCor::name() const { return "PolarizationEfficiencyCor"; }

/// Algorithm's version for identification. @see Algorithm::version
int PolarizationEfficiencyCor::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PolarizationEfficiencyCor::category() const {
  return "Reflectometry";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PolarizationEfficiencyCor::summary() const {
  return "Corrects a group of polarization analysis workspaces for polarizer and analyzer efficiencies.";
}

size_t PolarizationEfficiencyCor::WorkspaceMap::size() const noexcept {
  return mmWS ? 1 : 0 + mpWS ? 1 : 0 + pmWS ? 1 : 0 + ppWS ? 1 : 0;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PolarizationEfficiencyCor::init() {
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::WorkspaceGroup>>(Prop::INPUT_WS, "",
                                                             Kernel::Direction::Input),
      "A group of workspaces to be corrected.");
  const std::vector<std::string> defaultFlippers{{Flippers::OffOff, Flippers::OffOn, Flippers::OnOff, Flippers::OnOn}};
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<std::string>>(Prop::FLIPPERS, defaultFlippers, boost::make_shared<Kernel::ArrayLengthValidator<double>>(2, 4)),
        "A list of flipper configurations (accepted values: 00, 01, 10 and 11) corresponding to each input workspace.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::WorkspaceGroup>>(Prop::OUTPUT_WS, "",
                                                             Kernel::Direction::Output),
      "A group of polarization efficiency corrected workspaces.");
  declareProperty(
        Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::EFFICIENCIES, "", Kernel::Direction::Input),
        "A workspace containing the efficiency factors P1, P2, F1 and F2 as histograms");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PolarizationEfficiencyCor::exec() {
  const auto inputs = mapInputsToDirections();
  const EfficiencyMap efficiencies = efficiencyFactors();
  WorkspaceMap outputs;
  switch (inputs.size()) {
  case 2:
    break;
  case 3:
    outputs = threeInputCorrections(inputs, efficiencies);
    break;
  case 4:
    outputs = fullCorrections(inputs, efficiencies);
  }
  setProperty(Prop::OUTPUT_WS, groupOutput(outputs));
}

API::WorkspaceGroup_sptr PolarizationEfficiencyCor::groupOutput(const WorkspaceMap &outputs) {
  const std::string outWSName = getProperty(Prop::OUTPUT_WS);
  std::vector<std::string> names;
  if (outputs.mmWS) {
    names.emplace_back(outWSName + "_++");
    API::AnalysisDataService::Instance().addOrReplace(names.back(), outputs.mmWS);
  }
  if (outputs.mpWS) {
    names.emplace_back(outWSName + "_-+");
    API::AnalysisDataService::Instance().addOrReplace(names.back(), outputs.mpWS);
  }
  if (outputs.pmWS) {
    names.emplace_back(outWSName + "_+-");
    API::AnalysisDataService::Instance().addOrReplace(names.back(), outputs.pmWS);
  }
  if (outputs.ppWS) {
    names.emplace_back(outWSName + "_++");
    API::AnalysisDataService::Instance().addOrReplace(names.back(), outputs.ppWS);
  }
  auto group = createChildAlgorithm("GroupWorkspaces");
  group->initialize();
  group->setProperty("InputWorkspaces", names);
  group->setProperty("OutputWorkspace", outWSName);
  group->execute();
  API::WorkspaceGroup_sptr outWS = group->getProperty("OutputWorkspace");
  return outWS;
}

PolarizationEfficiencyCor::EfficiencyMap PolarizationEfficiencyCor::efficiencyFactors() {
  // TODO validate efficiencies.
  EfficiencyMap e;
  API::MatrixWorkspace_const_sptr factorWS = getProperty(Prop::EFFICIENCIES);
  const auto &vertAxis = factorWS->getAxis(1);
  for (size_t i = 0; i != vertAxis->length(); ++i) {
    const auto label = vertAxis->label(i);
    if (label == "P1") {
      e.P1 = &factorWS->y(i);
    } else if (label == "P2") {
      e.P2 = &factorWS->y(i);
    } else if (label == "F1") {
      e.F1 = &factorWS->y(i);
    } else if (label == "F2") {
      e.F2 = &factorWS->y(i);
    }
    // Ignore other histograms such as 'Phi' in ILL's efficiency ws.
  }
  return e;
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::threeInputCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  WorkspaceMap fullInputs = inputs;
  if (!inputs.mpWS) {
    solve01(fullInputs, efficiencies);
  } else {
    solve10(fullInputs, efficiencies);
  }
  return fullCorrections(fullInputs, efficiencies);
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::fullCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  WorkspaceMap outputs;
  // TODO check if history is retained with create().
  outputs.mmWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.mmWS);
  outputs.mpWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.mpWS);
  outputs.pmWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.pmWS);
  outputs.ppWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.ppWS);
  const size_t nHisto = inputs.mmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &mmY = inputs.mmWS->y(wsIndex);
    const auto &mpY = inputs.mpWS->y(wsIndex);
    const auto &pmY = inputs.pmWS->y(wsIndex);
    const auto &ppY = inputs.ppWS->y(wsIndex);
    auto &mmYOut = outputs.mmWS->mutableY(wsIndex);
    auto &mpYOut = outputs.mpWS->mutableY(wsIndex);
    auto &pmYOut = outputs.pmWS->mutableY(wsIndex);
    auto &ppYOut = outputs.ppWS->mutableY(wsIndex);
    for (size_t binIndex = 0; binIndex < mmY.size(); ++binIndex) {
      // Note that F1 and F2 correspond to 1-F1 and 1-F2 in [Wildes, 1999].
      const auto F1 = (*efficiencies.F1)[binIndex];
      const auto F2 = (*efficiencies.F2)[binIndex];
      const auto P1 = (*efficiencies.P1)[binIndex];
      const auto P2 = (*efficiencies.P2)[binIndex];
      // These are inverted forms of the efficiency matrices.
      const auto diag1 = 1. / F1;
      const auto off1 = (F1 - 1.) * diag1;
      Eigen::Matrix4d F1m;
      F1m <<   1.,   0.,    0.,    0.,
               0.,   1.,    0.,    0.,
             off1,   0., diag1,    0.,
               0., off1,    0., diag1;
      const auto diag2 = 1. / F2;
      const auto off2 = (F2 - 1.) * diag2;
      Eigen::Matrix4d F2m;
      F2m <<   1.,    0.,   0.,    0.,
             off2, diag2,   0.,    0.,
               0.,    0.,   1.,    0.,
               0.,    0., off2, diag2;
      const auto diag3 = (P1 - 1.) / (2. * P1 - 1.);
      const auto off3 = P1 / (2. * P1 - 1);
      Eigen::Matrix4d P1m;
      P1m << diag3,     0,  off3,     0,
                 0, diag3,     0,  off3,
              off3,     0, diag3,     0,
                 0,  off3,     0, diag3;
      const auto diag4 = (P2 - 1.) / (2. * P2 - 1.);
      const auto off4 = P2 / (2. * P2 - 1.);
      Eigen::Matrix4d P2m;
      P2m << diag4,  off4,    0.,    0.,
              off4, diag4,    0.,    0.,
                0.,    0., diag4,  off4,
                0.,    0.,  off4, diag4;
      const Eigen::Vector4d intensities(ppY[binIndex], pmY[binIndex], mpY[binIndex], mmY[binIndex]);
      const auto corrected = P2m * P1m * F2m * F1m * intensities;
      ppYOut[binIndex] = corrected[0];
      pmYOut[binIndex] = corrected[1];
      mpYOut[binIndex] = corrected[2];
      mmYOut[binIndex] = corrected[3];
    }
  }
  return outputs;
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::mapInputsToDirections() {
  API::WorkspaceGroup_const_sptr inGroup = getProperty(Prop::INPUT_WS);
  // TODO: Validate dirs in validateInputs().
  const std::vector<std::string> flippers = getProperty(Prop::FLIPPERS);
  WorkspaceMap inputs;
  for (size_t i = 0; i < flippers.size(); ++i) {
    auto ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(inGroup->getItem(i));
    if (!ws) {
      throw std::runtime_error("One of the input workspaces doesn't seem to be a MatrixWorkspace.");
    }
    const auto &f = flippers[i];
    if (f == Flippers::OnOn) {
      inputs.mmWS = ws;
    } else if (f == Flippers::OnOff) {
      inputs.mpWS = ws;
    } else if (f == Flippers::OffOn) {
      inputs.pmWS = ws;
    } else if (f == Flippers::OffOff) {
      inputs.ppWS = ws;
    } else {
      throw std::runtime_error(std::string{"Unknown entry in "} + Prop::FLIPPERS);
    }
  }
  return inputs;
}

void PolarizationEfficiencyCor::solve01(WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace Mantid::DataObjects;
  inputs.pmWS = create<Workspace2D>(*inputs.mpWS);
  const auto &F1 = *efficiencies.F1;
  const auto &F2 = *efficiencies.F2;
  const auto &P1 = *efficiencies.P1;
  const auto &P2 = *efficiencies.P2;
  const auto nHisto = inputs.pmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &I00 = inputs.ppWS->y(wsIndex);
    const auto &I10 = inputs.mpWS->y(wsIndex);
    const auto &I11 = inputs.mpWS->y(wsIndex);
    const auto a = 2. * P1 - 1;
    const auto b = P1 - P2;
    const auto divisor = F1 * a - b;
    const auto c = I00 - I10;
    inputs.pmWS->mutableY(wsIndex) = F1 * I00 * a - (I11 + c) * b - F2 * c * (2. * P2 - 1) / divisor;
  }
}

void PolarizationEfficiencyCor::solve10(WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  inputs.mpWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.pmWS);
  const auto &F1 = *efficiencies.F1;
  const auto &F2 = *efficiencies.F2;
  const auto &P1 = *efficiencies.P1;
  const auto &P2 = *efficiencies.P2;
  const auto nHisto = inputs.mpWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &I00 = inputs.ppWS->y(wsIndex);
    const auto &I01 = inputs.pmWS->y(wsIndex);
    const auto &I11 = inputs.mpWS->y(wsIndex);
    const auto a = 2. * P2 - 1;
    const auto b = P1 - P2;
    const auto divisor = F2 * a + b;
    const auto c = I01 - I00;
    inputs.mpWS->mutableY(wsIndex) = F1 * c * (2. * P1 - 1) + (I11 - c) * b + F2 * I00 * a / divisor;
  }
}
} // namespace Algorithms
} // namespace Mantid
