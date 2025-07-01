// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/DetermineSpinStateOrder.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Strings.h"

#include <algorithm>

namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

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
  return "Takes a workspace group of Polarised SANS run periods and returns a string (e.g '11, 10, 01, 00') of thier "
         "corresponding spin states in Wildes notation.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DetermineSpinStateOrder::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::WorkspaceGroup>>("InputWorkspace", "", Direction::Input),
                  "A Polarised SANS run from either LARMOR or ZOOM (group workspace with 4 periods).");
  declareProperty("SpinStates", std::string(""),
                  "A comma-seperated string of the spin states of each of the run periods e.g '11, 10, 01, 00'",
                  Direction::Output);
}

void validateGroupItem(API::MatrixWorkspace_sptr const &workspace, std::map<std::string, std::string> &errorList) {
  if (workspace == nullptr) {
    errorList["InputWorkspace"] = "All input workspaces must be of type MatrixWorkspace.";
    return;
  }

  Kernel::Unit_const_sptr unit = workspace->getAxis(0)->unit();
  if (unit->unitID() != "Wavelength") {
    errorList["InputWorkspace"] = "All input workspaces must be in units of Wavelength.";
    return;
  }

  if (workspace->getNumberHistograms() != 1) {
    errorList["InputWorkspace"] = "All input workspaces must contain a single histogram.";
    return;
  }

  if (!workspace->isHistogramData()) {
    errorList["InputWorkspace"] = "All input workspaces must be histogram data.";
  }
}

std::map<std::string, std::string> DetermineSpinStateOrder::validateInputs() {
  std::map<std::string, std::string> helpMessages;

  API::WorkspaceGroup_const_sptr wsGroup = getProperty("InputWorkspace");
  if (wsGroup->getNumberOfEntries() != 4) {
    helpMessages["InputWorkspace"] = "Input workspace group must have 4 entries (PA data)";
    return helpMessages;
  }

  for (const API::Workspace_sptr &ws : wsGroup->getAllItems()) {
    const auto groupItem = std::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
    validateGroupItem(groupItem, helpMessages);
    if (!helpMessages.empty()) {
      return helpMessages;
    }
  }

  const auto firstItem = std::dynamic_pointer_cast<API::MatrixWorkspace>(wsGroup->getItem(0));
  const auto instrument = firstItem->getInstrument()->getName();

  if (instrument == "LARMOR") {
    m_spinFlipperLogName = "FlipperCurrent";
    m_rfStateCondition = 4;
  } else if (instrument == "ZOOM") {
    m_spinFlipperLogName = "Spin_flipper";
    m_rfStateCondition = 0;
  } else {
    helpMessages["InputWorkspace"] = "Sub workspaces must be data from either LARMOR or ZOOM";
  }

  return helpMessages;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DetermineSpinStateOrder::exec() {
  API::WorkspaceGroup_const_sptr wsGroup = getProperty("InputWorkspace");

  const double averageTrans = averageTransmition(wsGroup);
  std::vector<std::string> spinStatesOrder;

  for (const API::Workspace_sptr &ws : wsGroup->getAllItems()) {
    const auto groupItem = std::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
    const auto sfLog =
        dynamic_cast<const Kernel::TimeSeriesProperty<double> *>(groupItem->run().getLogData(m_spinFlipperLogName));
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
  setProperty("SpinStates", spinStates);
}

double DetermineSpinStateOrder::averageTransmition(API::WorkspaceGroup_const_sptr const &wsGroup) const {
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
