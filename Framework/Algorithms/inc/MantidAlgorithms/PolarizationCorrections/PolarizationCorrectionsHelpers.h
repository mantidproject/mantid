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
static const std::string OFF_ON = "01";
static const std::string ON_OFF = "10";
static const std::string OFF_OFF = "00";
static const std::string ON_ON = "11";
static const std::string OFF = "0";
static const std::string ON = "1";
} // namespace FlipperConfigurations

namespace SpinStateConfigurationsFredrikze {
static const std::string PARA_ANTI = "pa";
static const std::string ANTI_PARA = "ap";
static const std::string PARA_PARA = "pp";
static const std::string ANTI_ANTI = "aa";
static const std::string PARA = "p";
static const std::string ANTI = "a";
} // namespace SpinStateConfigurationsFredrikze

namespace SpinStateConfigurationsWildes {
static const std::string MINUS_PLUS = "-+";
static const std::string PLUS_MINUS = "+-";
static const std::string MINUS_MINUS = "--";
static const std::string PLUS_PLUS = "++";
static const std::string MINUS = "-";
static const std::string PLUS = "+";
} // namespace SpinStateConfigurationsWildes

namespace SpinStatesORSO {
/*
 * Polarization constants and helper methods to support the Reflectometry ORSO file format
 */
static const std::string PP = "pp";
static const std::string PM = "pm";
static const std::string MP = "mp";
static const std::string MM = "mm";
static const std::string PO = "po";
static const std::string MO = "mo";

static const std::string LOG_NAME = "spin_state_ORSO";

MANTID_ALGORITHMS_DLL const std::string &getORSONotationForSpinState(const std::string &spinState);
MANTID_ALGORITHMS_DLL void addORSOLogForSpinState(const Mantid::API::MatrixWorkspace_sptr &ws,
                                                  const std::string &spinState);
} // namespace SpinStatesORSO
} // namespace Mantid::Algorithms
