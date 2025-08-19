// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/DetermineSpinStateOrder.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/PolSANSWorkspaceValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

#include <algorithm>
#include <sstream>

namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

Kernel::Logger g_log("DetermineSpinStateOrder");

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DetermineSpinStateOrder)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string DetermineSpinStateOrder::name() const { return "DetermineSpinStateOrder"; }

/// Algorithm's version for identification. @see Algorithm::version
int DetermineSpinStateOrder::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DetermineSpinStateOrder::category() const { return "SANS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string DetermineSpinStateOrder::summary() const {
  return "Takes a workspace group of Polarised SANS run periods and returns a string (e.g '11, 10, 01, 00') of their "
         "corresponding spin states in Wildes notation.";
}

const std::vector<std::string> DetermineSpinStateOrder::seeAlso() const { return {"AssertSpinStateOrder"}; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DetermineSpinStateOrder::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<API::WorkspaceGroup>>(
          "InputWorkspace", "", Direction::Input, std::make_shared<Mantid::API::PolSANSWorkspaceValidator>()),
      "A Polarised SANS run from either LARMOR or ZOOM (group workspace with 4 periods).");
  declareProperty("SpinFlipperLogName", std::string(""),
                  "Name of the log contained in the InputWorkspace which holds the flipper current (can be inferred if "
                  "data is from LARMOR or ZOOM).",
                  Direction::Input);
  declareProperty("SpinFlipperAverageCurrent", EMPTY_DBL(),
                  "Expected average current for the spin slipper over all periods. Used to determine if a particular "
                  "period has the flipper active or not (can be inferred if data is from LARMOR or ZOOM).",
                  Direction::Input);
  declareProperty("SpinStates", std::string(""),
                  "A comma-seperated string of the spin states of each of the run periods e.g '11, 10, 01, 00'",
                  Direction::Output);
}

void validateGroupItem(API::MatrixWorkspace_sptr const &workspace, std::map<std::string, std::string> &errorList,
                       const std::string &spinFlipperLogName = "") {
  if (spinFlipperLogName != "" && !workspace->run().hasProperty(spinFlipperLogName)) {
    errorList["InputWorkspace"] =
        "All input workspaces must contain the provided spin flipper log: " + spinFlipperLogName + ".";
  }
}

std::map<std::string, std::string> DetermineSpinStateOrder::validateInputs() {
  std::map<std::string, std::string> helpMessages;
  API::WorkspaceGroup_const_sptr wsGroup = getProperty("InputWorkspace");

  for (const API::Workspace_sptr &ws : wsGroup->getAllItems()) {
    const auto groupItem = std::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
    validateGroupItem(groupItem, helpMessages, getPropertyValue("SpinFlipperLogName"));
    if (!helpMessages.empty()) {
      return helpMessages;
    }
  }

  if (isDefault("SpinFlipperLogName") || isDefault("SpinFlipperAverageCurrent")) {
    const auto firstItem = std::dynamic_pointer_cast<API::MatrixWorkspace>(wsGroup->getItem(0));
    const auto instrument = firstItem->getInstrument()->getName();

    if (instrument == "LARMOR") {
      m_spinFlipperLogName =
          isDefault("SpinFlipperLogName") ? "FlipperCurrent" : getPropertyValue("SpinFlipperLogName");
      m_rfStateCondition =
          isDefault("SpinFlipperAverageCurrent") ? 4.0 : std::stod(getPropertyValue("SpinFlipperAverageCurrent"));
    } else if (instrument == "ZOOM") {
      m_spinFlipperLogName = isDefault("SpinFlipperLogName") ? "Spin_flipper" : getPropertyValue("SpinFlipperLogName");
      m_rfStateCondition =
          isDefault("SpinFlipperAverageCurrent") ? 0.0 : std::stod(getPropertyValue("SpinFlipperAverageCurrent"));
    } else {
      helpMessages["InputWorkspace"] = "Sub workspaces must be data from either LARMOR or ZOOM when SpinFlipperLogName "
                                       "or SpinFlipperAverageCurrent are not provided";
    }
  }

  return helpMessages;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DetermineSpinStateOrder::exec() {
  API::WorkspaceGroup_const_sptr wsGroup = getProperty("InputWorkspace");
  if (!isDefault("SpinFlipperLogName") && !isDefault("SpinFlipperAverageCurrent")) {
    m_spinFlipperLogName = getPropertyValue("SpinFlipperLogName");
    m_rfStateCondition = std::stod(getPropertyValue("SpinFlipperAverageCurrent"));
  }

  const double averageTrans = averageTransmission(wsGroup);
  std::vector<std::string> spinStatesOrder;

  for (const API::Workspace_sptr &ws : wsGroup->getAllItems()) {
    const auto groupItem = std::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
    const auto sfLog =
        dynamic_cast<const Kernel::TimeSeriesProperty<double> *>(groupItem->run().getLogData(m_spinFlipperLogName));
    if (!sfLog) {
      throw std::runtime_error(m_spinFlipperLogName + " was not a TimeSeriesProperty.");
    }
    const auto sfLogValues = sfLog->filteredValuesAsVector();
    const double rfState =
        std::accumulate(sfLogValues.cbegin(), sfLogValues.cend(), 0.0) / static_cast<double>(sfLogValues.size());
    const double heState = *std::max_element(groupItem->readY(0).cbegin(), groupItem->readY(0).cend()) - averageTrans;

    if (rfState > m_rfStateCondition) {
      if (heState < 0) {
        spinStatesOrder.push_back("10");
      } else {
        spinStatesOrder.push_back("11");
      }
    } else {
      if (heState < 0) {
        spinStatesOrder.push_back("01");
      } else {
        spinStatesOrder.push_back("00");
      }
    }
  }

  const std::string spinStates = Kernel::Strings::join(spinStatesOrder.cbegin(), spinStatesOrder.cend(), ",");
  std::stringstream msg;
  msg << "Determined the following spin state order for " << wsGroup->getName() << ": " << spinStates;
  g_log.notice(msg.str());
  setProperty("SpinStates", spinStates);
}

double DetermineSpinStateOrder::averageTransmission(API::WorkspaceGroup_const_sptr const &wsGroup) const {
  const auto workspaces = wsGroup->getAllItems();

  double total =
      std::accumulate(workspaces.cbegin(), workspaces.cend(), 0.0, [](double total, const API::Workspace_sptr ws) {
        const auto groupItem = std::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
        return total + *std::max_element(groupItem->readY(0).cbegin(), groupItem->readY(0).cend());
      });

  return total / static_cast<double>(workspaces.size());
}

} // namespace Algorithms
} // namespace Mantid
