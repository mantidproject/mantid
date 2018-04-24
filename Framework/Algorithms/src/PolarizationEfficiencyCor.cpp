#include "MantidAlgorithms/PolarizationEfficiencyCor.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringTokenizer.h"

#include "MantidAPI/WorkspaceFactory.h"

#include <Eigen/Dense>
#include <boost/math/special_functions/pow.hpp>

namespace {

/// Property names.
namespace Prop {
static const std::string FLIPPERS{"Flippers"};
static const std::string POLARIZATION_ANALYSIS{"PolarizationAnalysis"};
static const std::string EFFICIENCIES{"Efficiencies"};
static const std::string INPUT_WORKSPACES{"InputWorkspaces"};
static const std::string INPUT_WORKSPACE_GROUP{"InputWorkspaceGroup"};
static const std::string OUTPUT_WORKSPACES{"OutputWorkspace"};
static const std::string CORRECTION_METHOD{"CorrectionMethod"};
} // namespace Prop

/// Flipper configurations.
namespace Flippers {
static const std::string Off{"0"};
static const std::string OffOff{"00"};
static const std::string OffOn{"01"};
static const std::string On{"1"};
static const std::string OnOff{"10"};
static const std::string OnOn{"11"};
} // namespace Flippers

namespace CorrectionMethod {
static const std::string WILDES{"Wildes"};
static const std::string FREDRIKZE{"Fredrikze"};
} // namespace CorrectionMethod

} // namespace

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PolarizationEfficiencyCor)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PolarizationEfficiencyCor::name() const {
  return "PolarizationEfficiencyCor";
}

/// Algorithm's version for identification. @see Algorithm::version
int PolarizationEfficiencyCor::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PolarizationEfficiencyCor::category() const {
  return "Reflectometry";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PolarizationEfficiencyCor::summary() const {
  return "Corrects a group of polarization analysis workspaces for polarizer "
         "and analyzer efficiencies.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PolarizationEfficiencyCor::init() {
  bool const allowMultiSelection = true;
  bool const isOptional = true;
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<std::string>>(
          Prop::INPUT_WORKSPACES, "",
          boost::make_shared<ADSValidator>(allowMultiSelection, isOptional),
          Kernel::Direction::Input),
      "A list of workspaces to be corrected corresponding to the "
      "flipper configurations.");

  declareProperty(Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      Prop::INPUT_WORKSPACE_GROUP, "", Kernel::Direction::Input,
                      PropertyMode::Optional),
                  "An input workspace to process.");

  const std::vector<std::string> methods{
      {CorrectionMethod::WILDES, CorrectionMethod::FREDRIKZE}};
  declareProperty(
      Prop::CORRECTION_METHOD, CorrectionMethod::WILDES,
      boost::make_shared<Kernel::ListValidator<std::string>>(methods),
      "Correction method: Wildes or Fredrikze.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::EFFICIENCIES, "", Kernel::Direction::Input),
      "A workspace containing the efficiency factors P1, P2, F1 and F2 as "
      "histograms");

  const std::string full = Flippers::OffOff + ", " + Flippers::OffOn + ", " +
                           Flippers::OnOff + ", " + Flippers::OnOn;
  const std::string missing01 =
      Flippers::OffOff + ", " + Flippers::OnOff + ", " + Flippers::OnOn;
  const std::string missing10 =
      Flippers::OffOff + ", " + Flippers::OffOn + ", " + Flippers::OnOn;
  const std::string missing0110 = Flippers::OffOff + ", " + Flippers::OnOn;
  const std::string noAnalyzer = Flippers::Off + ", " + Flippers::On;
  const std::string directBeam = Flippers::Off;
  const std::vector<std::string> setups{
      {"", full, missing01, missing10, missing0110, noAnalyzer, directBeam}};
  declareProperty(
      Prop::FLIPPERS, "",
      boost::make_shared<Kernel::ListValidator<std::string>>(setups),
      "Flipper configurations of the input workspaces.");

  std::vector<std::string> propOptions{"", "PA", "PNR"};
  declareProperty("PolarizationAnalysis", "",
                  boost::make_shared<StringListValidator>(propOptions),
                  "What Polarization mode will be used?\n"
                  "PNR: Polarized Neutron Reflectivity mode\n"
                  "PA: Full Polarization Analysis PNR-PA");

  declareProperty(Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      Prop::OUTPUT_WORKSPACES, "", Kernel::Direction::Output),
                  "A group of polarization efficiency corrected workspaces.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PolarizationEfficiencyCor::exec() {
  std::string const method = getProperty(Prop::CORRECTION_METHOD);
  if (method == CorrectionMethod::WILDES) {
    execWildes();
  } else {
    execFredrikze();
  }
}

//----------------------------------------------------------------------------------------------
void PolarizationEfficiencyCor::execWildes() {
  checkWildesProperties();
  std::vector<std::string> workspaces = getWorkspaceNameList();
  MatrixWorkspace_sptr efficiencies = getProperty(Prop::EFFICIENCIES);
  auto alg = createChildAlgorithm("PolarizationCorrectionWildes");
  alg->initialize();
  alg->setProperty("InputWorkspaces", workspaces);
  alg->setProperty("Efficiencies", efficiencies);
  if (!isDefault(Prop::FLIPPERS)) {
    alg->setPropertyValue("Flippers", getPropertyValue(Prop::FLIPPERS));
  }
  auto out = getPropertyValue(Prop::OUTPUT_WORKSPACES);
  alg->setPropertyValue("OutputWorkspace", out
                        );
  alg->execute();
  API::WorkspaceGroup_sptr outWS = alg->getProperty("OutputWorkspace");
  setProperty(Prop::OUTPUT_WORKSPACES, outWS);
}

//----------------------------------------------------------------------------------------------
void PolarizationEfficiencyCor::execFredrikze() {
  checkFredrikzeProperties();
  WorkspaceGroup_sptr group = getWorkspaceGroup();
  MatrixWorkspace_sptr efficiencies = getProperty(Prop::EFFICIENCIES);
  auto alg = createChildAlgorithm("PolarizationCorrectionFredrikze");
  alg->initialize();
  alg->setProperty("InputWorkspace", group);
  alg->setProperty("Efficiencies", efficiencies);
  if (!isDefault(Prop::POLARIZATION_ANALYSIS)) {
    alg->setPropertyValue("PolarizationAnalysis",
                          getPropertyValue(Prop::POLARIZATION_ANALYSIS));
  }
  alg->setPropertyValue("OutputWorkspace",
                        getPropertyValue(Prop::OUTPUT_WORKSPACES));
  alg->execute();
  API::WorkspaceGroup_sptr outWS = alg->getProperty("OutputWorkspace");
  setProperty(Prop::OUTPUT_WORKSPACES, outWS);
}


//----------------------------------------------------------------------------------------------
/** Check that the inputs workspaces are set.
 */
void PolarizationEfficiencyCor::checkWorkspaces() const {
  if (isDefault(Prop::INPUT_WORKSPACES) &&
      isDefault(Prop::INPUT_WORKSPACE_GROUP)) {
    throw std::invalid_argument("Input workspaces are missing. Either a "
                                "workspace group or a list of workspace names "
                                "must be given.");
  }
  if (!isDefault(Prop::INPUT_WORKSPACES) &&
      !isDefault(Prop::INPUT_WORKSPACE_GROUP)) {
    throw std::invalid_argument("Input workspaces must be given either as a "
                                "workspace group or a list of names.");
  }
}

//----------------------------------------------------------------------------------------------
/** Check that the inputs for the Wildes are correct and consistent.
 */
void PolarizationEfficiencyCor::checkWildesProperties() const {
  checkWorkspaces();

  if (!isDefault(Prop::POLARIZATION_ANALYSIS)) {
    throw std::invalid_argument(
        "Property PolarizationAnalysis canot be used with the Wildes method.");
  }
}

//----------------------------------------------------------------------------------------------
/** Check that the inputs for the Fredrikze method are correct and consistent.
 */
void PolarizationEfficiencyCor::checkFredrikzeProperties() const {
  checkWorkspaces();

  if (!isDefault(Prop::FLIPPERS)) {
    throw std::invalid_argument(
        "Property Flippers canot be used with the Fredrikze method.");
  }
}

//----------------------------------------------------------------------------------------------
/** Get the input workspaces as a list of names.
 */
std::vector<std::string>  PolarizationEfficiencyCor::getWorkspaceNameList() const {
  std::vector<std::string> names;
  if (!isDefault(Prop::INPUT_WORKSPACES)) {
    names = getProperty(Prop::INPUT_WORKSPACES);
  } else {
    WorkspaceGroup_sptr group = getProperty(Prop::INPUT_WORKSPACE_GROUP);
    auto n = group->size();
    for(size_t i = 0; i < n; ++i) {
      auto ws = group->getItem(i);
      auto const name = ws->getName();
      if (name.empty()) {
        throw std::invalid_argument(
            "Workspace from the input workspace group is not stored in the "
            "Analysis Data Service which is required by the Wildes method.");
      }
      names.push_back(name);
    }
  }
  return names;
}

//----------------------------------------------------------------------------------------------
/** Get the input workspaces as a workspace group.
 */
API::WorkspaceGroup_sptr PolarizationEfficiencyCor::getWorkspaceGroup() const {
  WorkspaceGroup_sptr group;
  if (!isDefault(Prop::INPUT_WORKSPACE_GROUP)) {
    group = getProperty(Prop::INPUT_WORKSPACE_GROUP);
  } else {
    throw std::invalid_argument("Input workspaces are required to be in a workspace group.");
  }
  return group;
}

} // namespace Algorithms
} // namespace Mantid
