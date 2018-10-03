// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_
#define MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include <string>

namespace Mantid {
namespace DataHandling {
/** Extracts the start and the end time from a Nexus file.
 */
Mantid::Types::Core::DateAndTime DLLExport
extractStartTime(const std::string &filename);
Mantid::Types::Core::DateAndTime DLLExport
extractEndTime(const std::string &filename);
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_STARTANDENDTIMEFROMNEXUSFILEEXTRACTOR_H_ */
