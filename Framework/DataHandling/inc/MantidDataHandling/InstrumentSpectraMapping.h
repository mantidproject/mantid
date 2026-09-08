// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"

#include <string>

namespace Mantid {
namespace API {
class Algorithm;
class MatrixWorkspace;
} // namespace API
namespace Kernel {
class Logger;
}
namespace DataHandling {

/// The specific instrument parameter and target value that enables the correction
MANTID_DATAHANDLING_DLL extern const std::string SPECTRA_MAP_SOURCE;
MANTID_DATAHANDLING_DLL extern const std::string SPECTRA_MAP_FROM_INSTRUMENT;

/**
 * Corrects spectrum-to-detector mappings for files that use detector IDs not recognized by the instrument definition
 * file. If an unknown detector ID is found, it maps the spectrum to a detector ID that matches its spectrum number
 * instead.
 *
 * This behavior is enabled by setting the SPECTRA_MAP_SOURCE parameter to SPECTRA_MAP_FROM_INSTRUMENT,
 * and only applies if unknown detector IDs are present. Valid mappings are left untouched.
 *
 * The OSIRIS silicon analyser is the case this was written for and currently the only user: its electronics read
 * each pixel out as eight hardware elements, which they sum into one spectrum while still recording all eight
 * element numbers in the file's table.
 *
 * @param workspace :: the workspace to correct (must already hold the instrument)
 * @param log :: logger used to report what was corrected
 * @return whether any spectrum was changed
 */
MANTID_DATAHANDLING_DLL bool correctSpectraMapping(API::MatrixWorkspace &workspace, Kernel::Logger &log);

/**
 * Applies correctSpectraMapping to whichever workspaces a loader declared: its output, every period of a
 * multiperiod group, and any separately loaded monitor workspace.
 *
 * @param loader :: the loader that has just finished executing
 * @param log :: logger used to report what was corrected
 */
MANTID_DATAHANDLING_DLL void correctLoadedWorkspaces(API::Algorithm &loader, Kernel::Logger &log);

} // namespace DataHandling
} // namespace Mantid
