#include "MantidAlgorithms/PolarizationEfficiencyCor.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"

#include <boost/math/special_functions/pow.hpp>
#include <Eigen/Dense>

namespace {
namespace Prop {
constexpr char *ANALYZER{"Analyzer"};
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

void checkInputExists(const Mantid::API::MatrixWorkspace_sptr &ws, const std::string &tag) {
  if (!ws) {
    throw std::runtime_error("A workspace designated as " + tag + " is missing in inputs.");
  }
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
  return (mmWS ? 1 : 0) + (mpWS ? 1 : 0) + (pmWS ? 1 : 0) + (ppWS ? 1 : 0);
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
      Kernel::make_unique<Kernel::ArrayProperty<std::string>>(Prop::FLIPPERS, defaultFlippers, boost::make_shared<Kernel::ArrayLengthValidator<std::string>>(1, 4)),
        "A list of flipper configurations (accepted values: 00, 01, 10 and 11) corresponding to each input workspace.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::WorkspaceGroup>>(Prop::OUTPUT_WS, "",
                                                             Kernel::Direction::Output),
      "A group of polarization efficiency corrected workspaces.");
  declareProperty(
        Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::EFFICIENCIES, "", Kernel::Direction::Input),
        "A workspace containing the efficiency factors P1, P2, F1 and F2 as histograms");
  declareProperty(Prop::ANALYZER, true, "True if analyzer was used during the experiment, false otherwise.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PolarizationEfficiencyCor::exec() {
  const bool analyzer = getProperty(Prop::ANALYZER);
  const auto inputs = mapInputsToDirections();
  checkConsistentNumberHistograms(inputs);
  const EfficiencyMap efficiencies = efficiencyFactors();
  checkConsistentX(inputs, efficiencies);
  WorkspaceMap outputs;
  switch (inputs.size()) {
  case 1:
    outputs = directBeamCorrections(inputs, efficiencies);
    break;
  case 2:
    if (analyzer) {
      outputs = twoInputCorrections(inputs, efficiencies);
    } else {
      outputs = analyzerlessCorrections(inputs, efficiencies);
    }
    break;
  case 3:
    outputs = threeInputCorrections(inputs, efficiencies);
    break;
  case 4:
    outputs = fullCorrections(inputs, efficiencies);
  }
  setProperty(Prop::OUTPUT_WS, groupOutput(outputs));
}

std::map<std::string, std::string> PolarizationEfficiencyCor::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_const_sptr factorWS = getProperty(Prop::EFFICIENCIES);
  const auto &factorAxis = factorWS->getAxis(1);
  if (!factorAxis) {
    issues[Prop::EFFICIENCIES] = "The workspace is missing a vertical axis.";
  } else if (!factorAxis->isText()) {
    issues[Prop::EFFICIENCIES] = "The vertical axis in the workspace is not text axis.";
  } else if (factorWS->getNumberHistograms() < 4) {
    issues[Prop::EFFICIENCIES] = "The workspace should contain at least 4 histograms.";
  } else {
    std::vector<std::string> tags{{"P1", "P2", "F1", "F2"}};
    for (size_t i = 0; i != factorAxis->length(); ++i) {
      const auto label = factorAxis->label(i);
      auto found = std::find(tags.begin(), tags.end(), label);
      if (found != tags.cend()) {
        std::swap(tags.back(), *found);
        tags.pop_back();
      }
    }
    if (!tags.empty()) {
      issues[Prop::EFFICIENCIES] = "A histogram labeled " + tags.front() + " is missing from the workspace.";
    }
  }
  API::WorkspaceGroup_const_sptr inGroup = getProperty(Prop::INPUT_WS);
  const std::vector<std::string> flippers = getProperty(Prop::FLIPPERS);
  if (static_cast<size_t>(inGroup->getNumberOfEntries()) != flippers.size()) {
    issues[Prop::FLIPPERS] = "The number of flipper configurations does not match the number of input workspaces";
  } else {
    std::vector<std::string> configurations{{Flippers::OffOff, Flippers::OffOn, Flippers::OnOff, Flippers::OnOn}};
    for (const auto &c : flippers) {
      const auto found = std::find(configurations.cbegin(), configurations.cend(), c);
      if (found == configurations.cend()) {
        issues[Prop::EFFICIENCIES] = "Unknown flipper configuration: " + *found + ". Use 00, 01, 10 or 11 only.";
        break;
      }
    }
  }
  return issues;
}

void PolarizationEfficiencyCor::checkConsistentNumberHistograms(const WorkspaceMap &inputs) {
  size_t nHist{0};
  bool nHistValid{false};
  // A local helper function to check the number of histograms.
  auto checkNHist = [&nHist, &nHistValid](const API::MatrixWorkspace_sptr &ws, const std::string &tag) {
    if (nHistValid) {
      if (nHist != ws->getNumberHistograms()) {
        throw std::runtime_error("Number of histograms mismatch in " + tag);
      }
    } else {
      nHist = ws->getNumberHistograms();
      nHistValid = true;
    }
  };
  if (inputs.mmWS) {
    checkNHist(inputs.mmWS, Flippers::OffOff);
  }
  if (inputs.mpWS) {
    checkNHist(inputs.mpWS, Flippers::OffOn);
  }
  if (inputs.pmWS) {
    checkNHist(inputs.pmWS, Flippers::OnOff);
  }
  if (inputs.ppWS) {
    checkNHist(inputs.ppWS, Flippers::OnOn);
  }
}

void PolarizationEfficiencyCor::checkConsistentX(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  // Compare everything to F1 efficiency.
  const auto &F1x = efficiencies.F1->x();
  // A local helper function to check a HistogramX against F1.
  auto checkX = [&F1x](const HistogramData::HistogramX &x, const std::string &tag) {
    if (x.size() != F1x.size()) {
      throw std::runtime_error("Mismatch of histogram lengths between F1 and " + tag + '.');
    }
    for (size_t i = 0; i != x.size(); ++i) {
      if (x[i] != F1x[i]) {
        throw std::runtime_error("Mismatch of X data between F1 and " + tag + '.');
      }
    }
  };
  const auto &F2x = efficiencies.F2->x();
  checkX(F2x, "F2");
  const auto &P1x = efficiencies.P1->x();
  checkX(P1x, "P1");
  const auto &P2x = efficiencies.P2->x();
  checkX(P2x, "P2");
  // A local helper function to check an input workspace against F1.
  auto checkWS = [&F1x, &checkX](const API::MatrixWorkspace_sptr &ws, const std::string &tag) {
    const auto nHist = ws->getNumberHistograms();
    for (size_t i = 0; i != nHist; ++i) {
      checkX(ws->x(i), tag);
    }
  };
  if (inputs.mmWS) {
    checkWS(inputs.mmWS, Flippers::OffOff);
  }
  if (inputs.mpWS) {
    checkWS(inputs.mpWS, Flippers::OffOn);
  }
  if (inputs.pmWS) {
    checkWS(inputs.pmWS, Flippers::OnOff);
  }
  if (inputs.ppWS) {
    checkWS(inputs.ppWS, Flippers::OnOn);
  }
}


API::WorkspaceGroup_sptr PolarizationEfficiencyCor::groupOutput(const WorkspaceMap &outputs) {
  const std::string outWSName = getProperty(Prop::OUTPUT_WS);
  std::vector<std::string> names;
  if (outputs.mmWS) {
    names.emplace_back(outWSName + "_--");
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
  EfficiencyMap e;
  API::MatrixWorkspace_const_sptr factorWS = getProperty(Prop::EFFICIENCIES);
  const auto &vertAxis = factorWS->getAxis(1);
  for (size_t i = 0; i != vertAxis->length(); ++i) {
    const auto label = vertAxis->label(i);
    if (label == "P1") {
      e.P1 = &factorWS->getSpectrum(i);
    } else if (label == "P2") {
      e.P2 = &factorWS->getSpectrum(i);
    } else if (label == "F1") {
      e.F1 = &factorWS->getSpectrum(i);
    } else if (label == "F2") {
      e.F2 = &factorWS->getSpectrum(i);
    }
    // Ignore other histograms such as 'Phi' in ILL's efficiency ws.
  }
  return e;
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::directBeamCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  checkInputExists(inputs.ppWS, Flippers::OffOff);
  WorkspaceMap outputs;
  outputs.ppWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.ppWS);
  const size_t nHisto = inputs.ppWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &ppY = inputs.ppWS->y(wsIndex);
    const auto &ppE = inputs.ppWS->e(wsIndex);
    auto &ppYOut = outputs.ppWS->mutableY(wsIndex);
    auto &ppEOut = outputs.ppWS->mutableE(wsIndex);
    for (size_t binIndex = 0; binIndex < ppY.size(); ++binIndex) {
      const auto P1 = efficiencies.P1->y()[binIndex];
      const auto P2 = efficiencies.P2->y()[binIndex];
      const double f = 1. - P1 - P2 + 2. * P1 * P2;
      ppYOut[binIndex] = ppY[binIndex] / f;
      const auto P1E = efficiencies.P1->e()[binIndex];
      const auto P2E = efficiencies.P2->e()[binIndex];
      const auto e1 = pow<2>(P1E * (2. * P1 - 1.) / pow<2>(f) * ppY[binIndex]);
      const auto e2 = pow<2>(P2E * (2. * P2 - 1.) / pow<2>(f) * ppY[binIndex]);
      const auto e3 = pow<2>(ppE[binIndex] / f);
      const auto errorSum = std::sqrt(e1 + e2 + e3);
      ppEOut[binIndex] = errorSum;
    }
  }
  return outputs;
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::analyzerlessCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  checkInputExists(inputs.mmWS, Flippers::OnOn);
  checkInputExists(inputs.ppWS, Flippers::OffOff);
  WorkspaceMap outputs;
  outputs.mmWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.mmWS);
  outputs.ppWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.ppWS);
  const size_t nHisto = inputs.mmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &mmY = inputs.mmWS->y(wsIndex);
    const auto &mmE = inputs.mmWS->e(wsIndex);
    const auto &ppY = inputs.ppWS->y(wsIndex);
    const auto &ppE = inputs.ppWS->e(wsIndex);
    auto &mmYOut = outputs.mmWS->mutableY(wsIndex);
    auto &mmEOut = outputs.mmWS->mutableE(wsIndex);
    auto &ppYOut = outputs.ppWS->mutableY(wsIndex);
    auto &ppEOut = outputs.ppWS->mutableE(wsIndex);
    for (size_t binIndex = 0; binIndex < mmY.size(); ++binIndex) {
      const auto F1 = efficiencies.F1->y()[binIndex];
      const auto P1 = efficiencies.P1->y()[binIndex];
      Eigen::Matrix2d F1m;
      F1m <<             1.,      0.,
             (F1 - 1.) / F1, 1. / F1;
      const double divisor = (2. * P1 - 1.);
      const double diag = (P1 - 1.) / divisor;
      const double off = P1 / divisor;
      Eigen::Matrix2d P1m;
      P1m << diag,  off,
              off, diag;
      const Eigen::Vector2d intensities(ppY[binIndex], mmY[binIndex]);
      const auto PFProduct = P1m * F1m;
      const auto corrected = PFProduct * intensities;
      ppYOut = corrected[0];
      mmYOut = corrected[1];
      const auto F1E = efficiencies.F1->e()[binIndex];
      const auto P1E = efficiencies.P1->e()[binIndex];
      const auto elemE1 = -1. / pow<2>(F1) * F1E;
      Eigen::Matrix2d F1Em;
      F1Em <<      0.,     0.,
              -elemE1, elemE1;
      const auto elemE2 = 1. / pow<2>(divisor) * P1E;
      Eigen::Matrix2d P1Em;
      P1Em <<  elemE2, -elemE2,
              -elemE2,  elemE2;
      const Eigen::Vector2d errors(ppE[binIndex], mmE[binIndex]);
      const auto e1 = (F1Em * P1m * intensities).array();
      const auto e2 = (F1m * P1Em * intensities).array();
      const auto sqPFProduct = (PFProduct.array() * PFProduct.array()).matrix();
      const auto sqErrors = (errors.array() * errors.array()).matrix();
      const auto e3 = (sqPFProduct * sqErrors).array();
      const auto errorSum = (e1 * e1 + e2 * e2 + e3).sqrt();
      ppEOut[binIndex] = errorSum[0];
      mmEOut[binIndex] = errorSum[1];
    }
  }
  return outputs;
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::twoInputCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  checkInputExists(inputs.mmWS, Flippers::OnOn);
  checkInputExists(inputs.ppWS, Flippers::OffOff);
  WorkspaceMap fullInputs = inputs;
  fullInputs.mpWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.mmWS);
  fullInputs.pmWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.ppWS);
  const auto &F1 = efficiencies.F1->y();
  const auto &F1E = efficiencies.F1->e();
  const auto &F2 = efficiencies.F2->y();
  const auto &F2E = efficiencies.F2->e();
  const auto &P1 = efficiencies.P1->y();
  const auto &P1E = efficiencies.P1->e();
  const auto &P2 = efficiencies.P2->y();
  const auto &P2E = efficiencies.P2->e();
  const auto nHisto = inputs.mmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &I00 = inputs.ppWS->y(wsIndex);
    const auto &E00 = inputs.ppWS->e(wsIndex);
    const auto &I11 = inputs.mmWS->y(wsIndex);
    const auto &E11 = inputs.mmWS->e(wsIndex);
    auto &I01 = fullInputs.pmWS->mutableY(wsIndex);
    auto &E01 = fullInputs.pmWS->mutableE(wsIndex);
    auto &I10 = fullInputs.mpWS->mutableY(wsIndex);
    auto &E10 = fullInputs.mpWS->mutableE(wsIndex);
    for (size_t binIndex = 0; binIndex != I00.size(); ++binIndex) {
      const auto i00 = I00[binIndex];
      const auto i11 = I11[binIndex];
      const auto f1 = F1[binIndex];
      const auto f2 = F2[binIndex];
      const auto p1 = P1[binIndex];
      const auto p2 = P2[binIndex];
      const auto a = -1. + p1 + 2. * p2 - 2. * p1 * p2;
      const auto b = -1. + 2. * p1;
      const auto c = -1. + 2. * p2;
      const auto d = -1. + p2;
      // Case: 01
      const auto divisor = f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c);
      I01[binIndex] = (f2 * i11 * p1 * a - f1 * i00 * b * (-f2 * pow<2>(c) + pow<2>(f2 * c) + d * p2)) / divisor;
      // Error estimates.
      // Derivatives of the above with respect to i00, i11, f1, etc.
      const auto pmdi00 = -((f1 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) /
                            (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) + f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2.* p2))));
      const auto pmdi11 = (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2)) /
          (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) + f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
      const auto pmdf1 = -(((-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)) * (f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) - f1 * i00 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2))) /
                           pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) + f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) - (i00 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) /
          (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) + f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
      const auto pmdf2 = -(((f1 * (-1. + 2. * p1) * (-1. + p1 + p2) * (-1 + 2 * p2) + p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2)) * (f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) - f1 * i00 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) + pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2))) /
                           pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) + f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) + (-f1 * i00 * (-1. + 2. * p1) * (-pow<2>(1. - 2. * p2) + 2 * f2 * pow<2>(1. - 2. * p2)) +
                         i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2)) /
          (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) + f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
      const auto pmdp1 = -(((f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
                             f1 * i00 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) +
                                pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) * (f2 * p1 * (1. - 2. * p2) +
                             f1 * f2 * (-1. + 2. * p1) * (-1. + 2. * p2) +
                             f2 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
                             2. * f1 * ((1. - p2) * p2 +
                                f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) / pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
                           f1 * (-1. + 2. * p1) * ((1. - p2) * p2 +
                              f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) + (f2 * i11 * p1 * (1. - 2. * p2) +
                         f2 * i11 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
                         2. * f1 * i00 * (-f2 * pow<2>(1. - 2. * p2) +
                            pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) / (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
                         f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
      const auto pmdp2 = -(((f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
                             f1 * i00 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) +
                                pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) * (f2 * (2. - 2. * p1) * p1 +
                             f1 * (-1. + 2. * p1) * (1. - 2. * p2 + 2. * f2 * (-1. + p1 + p2) +
                                f2 * (-1. + 2. * p2)))) / pow<2>(f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
                           f1 * (-1. + 2. * p1) * ((1. - p2) * p2 +
                              f2 * (-1. + p1 + p2) * (-1. + 2. * p2)))) + (f2 * i11 * (2. - 2. * p1) * p1 -
                         f1 * i00 * (-1. + 2. * p1) * (-1. + 4. * f2 * (1. - 2. * p2) - 4. * pow<2>(f2) * (1. - 2. * p2) +
                            2. * p2)) / (f2 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) +
                         f1 * (-1. + 2. * p1) * ((1. - p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
      const auto e01_I00 = pow<2>(pmdi00 * E00[binIndex]);
      const auto e01_I11 = pow<2>(pmdi11 * E11[binIndex]);
      const auto e01_F1 = pow<2>(pmdf1 * F1E[binIndex]);
      const auto e01_F2 = pow<2>(pmdf2 * F2E[binIndex]);
      const auto e01_P1 = pow<2>(pmdp1 * P1E[binIndex]);
      const auto e01_P2 = pow<2>(pmdp2 * P2E[binIndex]);
      E01[binIndex] = std::sqrt(e01_I00 + e01_I11 + e01_F1 + e01_F2 + e01_P1 + e01_P2);
      // Case: 10
      I10[binIndex] = (-pow<2>(f1) * f2 * i00 * pow<2>(b) * c + f2 * i00 * p1 * a + f1 * b * (-i11 * d * p2 + f2 * i00 * b * c)) / divisor;
      // Derivatives of the above with respect to i00, i11, f1, etc.
      const auto mpdi00 = (-pow<2>(f1) * f2 * pow<2>(b) * c + f1 * f2 * pow<2>(b) * c + f2 * p1 * a) / (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c));
      const auto mpdi11 = -((f1 * b * d * p2) / (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c)));
      const auto mpdf1 = (f2 * i11 * p1 * (-1. + p1 + 2. * p2 - 2. * p1 * p2) -
         f1 * i00 * (-1. + 2. * p1) * (-f2 * pow<2>(1. - 2. * p2) +
            pow<2>(f2) * pow<2>(1. - 2. * p2) + (-1. + p2) * p2)) / (f2 * p1 * (-1. + p1 + 2. * p2 -
            2. * p1 * p2) +
         f1 * (-1. + 2. * p1) * (-(-1 + p2) * p2 + f2 * (-1. + p1 + p2) * (-1. + 2. * p2)));
      const auto mpdf2 = -(((f1 * b * (p1 + d) * c + p1 * a) * (-pow<2>(f1) * f2 * i00 * pow<2>(b) * c + f2 * i00 * p1 * a + f1 * b * (-i11 * d * p2 + f2 * i00 * b * c))) /
                         pow<2>(f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c))) +
          (-pow<2>(f1) * i00 * pow<2>(b) * c + f1 * i00 * pow<2>(b) * c + i00 * p1 * a) /
          (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c));
      const auto mpdp1 = -(((-pow<2>(f1) * f2 * i00 * pow<2>(b) * c + f2 * i00 * p1 * a + f1 * b * (-i11 * d * p2 + f2 * i00 * b * c)) * (f2 * p1 * -c + f1 * f2 * b * c + f2 * a + 2. * f1 * (-d * p2 + f2 * (p1 + d) * c))) /
                         pow<2>(f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c))) +
          (f2 * i00 * p1 * -c + 4. * pow<2>(f1) * f2 * i00 * -b * c + 2. * f1 * f2 * i00 * b * c + f2 * i00 * a + 2. * f1 * (-i11 * d * p2 + f2 * i00 * b * c)) /
          (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c));
      const auto mpdp2 = -(((f2 * (2. - 2. * p1) * p1 + f1 * b * (1. - 2. * p2 + 2. * f2 * (p1 + d) + f2 * c)) * (-pow<2>(f1) * f2 * i00 * pow<2>(b) * c + f2 * i00 * p1 * a + f1 * b *(-i11 * d * p2 + f2 * i00 * b * c))) / pow<2>(f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c))) +
          (-2. * pow<2>(f1) * f2 * i00 * pow<2>(b) + f2 * i00 * (2. - 2. * p1) * p1 + f1 * b * (2. * f2 * i00 * b - i11 * d - i11 * p2)) /
          (f2 * p1 * a + f1 * b * (-d * p2 + f2 * (p1 + d) * c));
      const auto e10_I00 = pow<2>(mpdi00 * E00[binIndex]);
      const auto e10_I11 = pow<2>(mpdi11 * E11[binIndex]);
      const auto e10_F1 = pow<2>(mpdf1 * F1E[binIndex]);
      const auto e10_F2 = pow<2>(mpdf2 * F2E[binIndex]);
      const auto e10_P1 = pow<2>(mpdp1 * P1E[binIndex]);
      const auto e10_P2 = pow<2>(mpdp2 * P2E[binIndex]);
      E10[binIndex] = std::sqrt(e10_I00 + e10_I11 + e10_F1 + e10_F2 + e10_P1 + e10_P2);
    }
  }
  return fullCorrections(fullInputs, efficiencies);
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::threeInputCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  WorkspaceMap fullInputs = inputs;
  checkInputExists(inputs.mmWS, Flippers::OnOn);
  checkInputExists(inputs.ppWS, Flippers::OffOff);
  if (!inputs.mpWS) {
    checkInputExists(inputs.pmWS, Flippers::OffOn);
    solve10(fullInputs, efficiencies);
  } else {
    checkInputExists(inputs.mpWS, Flippers::OnOff);
    solve01(fullInputs, efficiencies);
  }
  return fullCorrections(fullInputs, efficiencies);
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::fullCorrections(const WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  using namespace boost::math;
  checkInputExists(inputs.mmWS, Flippers::OnOn);
  checkInputExists(inputs.mpWS, Flippers::OnOff);
  checkInputExists(inputs.pmWS, Flippers::OffOn);
  checkInputExists(inputs.ppWS, Flippers::OffOff);
  WorkspaceMap outputs;
  // TODO check if history is retained with create().
  outputs.mmWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.mmWS);
  outputs.mpWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.mpWS);
  outputs.pmWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.pmWS);
  outputs.ppWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.ppWS);
  const size_t nHisto = inputs.mmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &mmY = inputs.mmWS->y(wsIndex);
    const auto &mmE = inputs.mmWS->e(wsIndex);
    const auto &mpY = inputs.mpWS->y(wsIndex);
    const auto &mpE = inputs.mpWS->e(wsIndex);
    const auto &pmY = inputs.pmWS->y(wsIndex);
    const auto &pmE = inputs.pmWS->e(wsIndex);
    const auto &ppY = inputs.ppWS->y(wsIndex);
    const auto &ppE = inputs.ppWS->e(wsIndex);
    auto &mmYOut = outputs.mmWS->mutableY(wsIndex);
    auto &mmEOut = outputs.mmWS->mutableE(wsIndex);
    auto &mpYOut = outputs.mpWS->mutableY(wsIndex);
    auto &mpEOut = outputs.mpWS->mutableE(wsIndex);
    auto &pmYOut = outputs.pmWS->mutableY(wsIndex);
    auto &pmEOut = outputs.pmWS->mutableE(wsIndex);
    auto &ppYOut = outputs.ppWS->mutableY(wsIndex);
    auto &ppEOut = outputs.ppWS->mutableE(wsIndex);
    for (size_t binIndex = 0; binIndex < mmY.size(); ++binIndex) {
      // Note that F1 and F2 correspond to 1-F1 and 1-F2 in [Wildes, 1999].
      const auto F1 = efficiencies.F1->y()[binIndex];
      const auto F2 = efficiencies.F2->y()[binIndex];
      const auto P1 = efficiencies.P1->y()[binIndex];
      const auto P2 = efficiencies.P2->y()[binIndex];
      // These are inverted forms of the efficiency matrices.
      const auto diag1 = 1. / F1;
      const auto off1 = (F1 - 1.) / F1;
      Eigen::Matrix4d F1m;
      F1m <<   1.,   0.,    0.,    0.,
               0.,   1.,    0.,    0.,
             off1,   0., diag1,    0.,
               0., off1,    0., diag1;
      const auto diag2 = 1. / F2;
      const auto off2 = (F2 - 1.) / F2;
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
      const auto FProduct = F2m * F1m;
      const auto PProduct = P2m * P1m;
      const auto PFProduct = PProduct * FProduct;
      const auto corrected = PFProduct * intensities;
      ppYOut[binIndex] = corrected[0];
      pmYOut[binIndex] = corrected[1];
      mpYOut[binIndex] = corrected[2];
      mmYOut[binIndex] = corrected[3];
      const auto F1E = efficiencies.F1->e()[binIndex];
      const auto F2E = efficiencies.F2->e()[binIndex];
      const auto P1E = efficiencies.P1->e()[binIndex];
      const auto P2E = efficiencies.P2->e()[binIndex];
      // The error matrices here are element-wise algebraic derivatives of
      // the matrices above, multiplied by the error.
      const auto elemE1 = - 1. / pow<2>(F1) * F1E;
      Eigen::Matrix4d F1Em;
      F1Em <<      0.,      0.,     0.,     0.,
                   0.,      0.,     0.,     0.,
              -elemE1,      0., elemE1,     0.,
                   0., -elemE1,     0., elemE1;
      const auto elemE2 = - 1. / pow<2>(F2) * F2E;
      Eigen::Matrix4d F2Em;
      F2Em <<      0.,     0.,      0.,     0.,
              -elemE2, elemE2,      0.,     0.,
                   0.,     0.,      0.,     0.,
                   0.,     0., -elemE2, elemE2;
      const auto elemE3 = 1. / pow<2>(2. * P1 - 1.) * P1E;
      Eigen::Matrix4d P1Em;
      P1Em <<  elemE3,      0., -elemE3,      0.,
                   0.,  elemE3,      0., -elemE3,
              -elemE3,      0.,  elemE3,      0.,
                   0., -elemE3,      0.,  elemE3;
      const auto elemE4 = 1. / pow<2>(2. * P2 - 1.) * P2E;
      Eigen::Matrix4d P2Em;
      P2Em <<  elemE4, -elemE4,      0.,      0.,
              -elemE4,  elemE4,      0.,      0.,
                   0.,      0.,  elemE4, -elemE4,
                   0.,      0., -elemE4,  elemE4;
      const Eigen::Vector4d errors(ppE[binIndex], pmE[binIndex], mpE[binIndex], mmE[binIndex]);
      const auto e1 = (P2Em * P1m * FProduct * intensities).array();
      const auto e2 = (P2m * P1Em * FProduct * intensities).array();
      const auto e3 = (PProduct * F2Em * F1m * intensities).array();
      const auto e4 = (PProduct * F2m * F1Em * intensities).array();
      const auto sqPFProduct = (PFProduct.array() * PFProduct.array()).matrix();
      const auto sqErrors = (errors.array() * errors.array()).matrix();
      const auto e5 =  (sqPFProduct * sqErrors).array();
      const auto errorSum = (e1 * e1 + e2 * e2 + e3 * e3 + e4 * e4 + e5).sqrt();
      ppEOut[binIndex] = errorSum[0];
      pmEOut[binIndex] = errorSum[1];
      mpEOut[binIndex] = errorSum[2];
      mmEOut[binIndex] = errorSum[3];
    }
  }
  return outputs;
}

PolarizationEfficiencyCor::WorkspaceMap PolarizationEfficiencyCor::mapInputsToDirections() {
  API::WorkspaceGroup_const_sptr inGroup = getProperty(Prop::INPUT_WS);
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
  const auto &F1 = efficiencies.F1->y();
  const auto &F2 = efficiencies.F2->y();
  const auto &P1 = efficiencies.P1->y();
  const auto &P2 = efficiencies.P2->y();
  const auto nHisto = inputs.pmWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &I00 = inputs.ppWS->y(wsIndex);
    auto &I01 = inputs.pmWS->mutableY(wsIndex);
    const auto &I10 = inputs.mpWS->y(wsIndex);
    const auto &I11 = inputs.mmWS->y(wsIndex);
    for (size_t binIndex = 0; binIndex != I00.size(); ++binIndex) {
      const auto f1 = F1[binIndex];
      const auto f2 = F2[binIndex];
      const auto p1 = P1[binIndex];
      const auto p2 = P2[binIndex];
      const auto i00 = I00[binIndex];
      const auto i10 = I10[binIndex];
      const auto i11 = I11[binIndex];
      I01[binIndex] = (f1 * i00 * (-1. + 2. * p1) - (i00 - i10 + i11) * (p1 - p2) -
                       f2 * (i00 - i10) * (-1. + 2. * p2)) / (-p1 + f1 * (-1. + 2. * p1) + p2);
      // The errors are left to zero.
    }
  }
}

void PolarizationEfficiencyCor::solve10(WorkspaceMap &inputs, const EfficiencyMap &efficiencies) {
  inputs.mpWS = DataObjects::create<DataObjects::Workspace2D>(*inputs.pmWS);
  const auto &F1 = efficiencies.F1->y();
  const auto &F2 = efficiencies.F2->y();
  const auto &P1 = efficiencies.P1->y();
  const auto &P2 = efficiencies.P2->y();
  const auto nHisto = inputs.mpWS->getNumberHistograms();
  for (size_t wsIndex = 0; wsIndex != nHisto; ++wsIndex) {
    const auto &I00 = inputs.ppWS->y(wsIndex);
    const auto &I01 = inputs.pmWS->y(wsIndex);
    auto &I10 = inputs.mpWS->mutableY(wsIndex);
    const auto &I11 = inputs.mmWS->y(wsIndex);
    for (size_t binIndex = 0; binIndex != I00.size(); ++binIndex) {
      const auto f1 = F1[binIndex];
      const auto f2 = F2[binIndex];
      const auto p1 = P1[binIndex];
      const auto p2 = P2[binIndex];
      const auto i00 = I00[binIndex];
      const auto i01 = I01[binIndex];
      const auto i11 = I11[binIndex];
      I10[binIndex] = (-f1 * (i00 - i01) * (-1. + 2. * p1) + (i00 - i01 + i11) * (p1 - p2) +
                       f2 * i00 * (-1. + 2. * p2)) / (p1 - p2 + f2 * (-1. + 2. * p2));
      // The errors are left to zero.
    }
  }
}
} // namespace Algorithms
} // namespace Mantid
