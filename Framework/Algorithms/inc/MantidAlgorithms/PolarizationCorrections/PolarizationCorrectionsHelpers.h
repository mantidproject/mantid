// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DllConfig.h"
#include <optional>

namespace Mantid::Algorithms {
namespace PolarizationCorrectionsHelpers {
MANTID_ALGORITHMS_DLL API::MatrixWorkspace_sptr workspaceForSpinState(API::WorkspaceGroup_sptr group,
                                                                      const std::string &spinStateOrder,
                                                                      const std::string &targetSpinState);
MANTID_ALGORITHMS_DLL std::optional<size_t> indexOfWorkspaceForSpinState(const std::vector<std::string> &spinStateOrder,
                                                                         std::string targetSpinState);
MANTID_ALGORITHMS_DLL std::vector<std::string> splitSpinStateString(const std::string &spinStates);
} // namespace PolarizationCorrectionsHelpers

namespace FlipperConfigurations {
auto constexpr OFF_ON = "01";
auto constexpr ON_OFF = "10";
auto constexpr OFF_OFF = "00";
auto constexpr ON_ON = "11";
auto constexpr OFF = "0";
auto constexpr ON = "1";
} // namespace FlipperConfigurations

namespace SpinStateConfigurationsFredrikze {
auto constexpr PARA_ANTI = "pa";
auto constexpr ANTI_PARA = "ap";
auto constexpr PARA_PARA = "pp";
auto constexpr ANTI_ANTI = "aa";
auto constexpr PARA = "p";
auto constexpr ANTI = "a";
} // namespace SpinStateConfigurationsFredrikze

namespace SpinStateConfigurationsWildes {
auto constexpr MINUS_PLUS = "-+";
auto constexpr PLUS_MINUS = "+-";
auto constexpr MINUS_MINUS = "--";
auto constexpr PLUS_PLUS = "++";
auto constexpr MINUS = "-";
auto constexpr PLUS = "+";
} // namespace SpinStateConfigurationsWildes

namespace SpinStatesORSO {
/*
 * Polarization constants and helper methods to support the Reflectometry ORSO file format
 */
auto constexpr PP = "pp";
auto constexpr PM = "pm";
auto constexpr MP = "mp";
auto constexpr MM = "mm";
auto constexpr PO = "po";
auto constexpr MO = "mo";

auto constexpr LOG_NAME = "spin_state_ORSO";

MANTID_ALGORITHMS_DLL const std::string getORSONotationForSpinState(const std::string &spinState);
MANTID_ALGORITHMS_DLL void addORSOLogForSpinState(const Mantid::API::MatrixWorkspace_sptr &ws,
                                                  const std::string &spinState);
} // namespace SpinStatesORSO
} // namespace Mantid::Algorithms