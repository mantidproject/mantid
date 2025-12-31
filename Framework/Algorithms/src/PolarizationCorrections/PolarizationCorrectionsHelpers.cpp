// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/SpinStateHelpers.h"
#include "MantidKernel/StringTokenizer.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <vector>

namespace Mantid {
namespace Algorithms {
namespace PolarizationCorrectionsHelpers {

/**
 * Returns the workspace in the group associated with the given targetSpinState according to the order defined by
 * spinStateOrder.
 * @param group Workspace group containing spin states.
 * @param spinStateOrder The order of the different spin states within the group.
 * @param targetSpinState The spin state workspace to extract from the group.
 * @return The MatrixWorkspace containing the target spin state. nullptr if not present.
 */
API::MatrixWorkspace_sptr workspaceForSpinState(const API::WorkspaceGroup_sptr &group,
                                                const std::string &spinStateOrder, const std::string &targetSpinState) {
  const auto &spinStateOrderVec = Mantid::Kernel::SpinStateHelpers::splitSpinStateString(spinStateOrder);
  const auto &wsIndex =
      Mantid::Kernel::SpinStateHelpers::indexOfWorkspaceForSpinState(spinStateOrderVec, targetSpinState);
  if (!wsIndex.has_value()) {
    return nullptr;
  }
  return std::dynamic_pointer_cast<API::MatrixWorkspace>(group->getItem(wsIndex.value()));
}
} // namespace PolarizationCorrectionsHelpers

namespace SpinStatesORSO {
/*
 * For a given polarization spin state, return the corresponding Reflectometry ORSO file format notation.
 * @param spinState The spin state to find the ORSO notation for.
 * @return The ORSO notation that represents the given polarization spin state.
 * @throw std::invalid_argument if no corresponding ORSO notation can be found.
 */
const std::string &getORSONotationForSpinState(const std::string &spinState) {
  if (spinState == SpinStateConfigurationsWildes::PLUS_PLUS ||
      spinState == SpinStateConfigurationsFredrikze::PARA_PARA) {
    return SpinStatesORSO::PP;
  }

  if (spinState == SpinStateConfigurationsWildes::PLUS_MINUS ||
      spinState == SpinStateConfigurationsFredrikze::PARA_ANTI) {
    return SpinStatesORSO::PM;
  }

  if (spinState == SpinStateConfigurationsWildes::MINUS_PLUS ||
      spinState == SpinStateConfigurationsFredrikze::ANTI_PARA) {
    return SpinStatesORSO::MP;
  }

  if (spinState == SpinStateConfigurationsWildes::MINUS_MINUS ||
      spinState == SpinStateConfigurationsFredrikze::ANTI_ANTI) {
    return SpinStatesORSO::MM;
  }

  if (spinState == SpinStateConfigurationsWildes::PLUS || spinState == SpinStateConfigurationsFredrikze::PARA) {
    return SpinStatesORSO::PO;
  }

  if (spinState == SpinStateConfigurationsWildes::MINUS || spinState == SpinStateConfigurationsFredrikze::ANTI) {
    return SpinStatesORSO::MO;
  }

  throw std::invalid_argument("Cannot convert spin state " + spinState + " into ORSO notation.");
}

/*
 * Add a sample log entry for the given polarization spin state using the corresponding Reflectometry ORSO file format
 * notation.
 * @param spinState The spin state to add the ORSO spin state sample log for.
 * @throw std::invalid_argument if no corresponding ORSO spin state notation can be found.
 */
void addORSOLogForSpinState(const Mantid::API::MatrixWorkspace_sptr &ws, const std::string &spinState) {
  const auto &logValue = getORSONotationForSpinState(spinState);
  ws->mutableRun().addProperty(SpinStatesORSO::LOG_NAME, logValue, true);
}
} // namespace SpinStatesORSO
} // namespace Algorithms
} // namespace Mantid
