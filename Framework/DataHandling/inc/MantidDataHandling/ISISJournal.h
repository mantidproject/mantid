// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include <map>
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {
namespace ISISJournal {
/**
  Defines functions to aid in fetching ISIS specific run information from
  journal files
 */

using ISISJournalTags = std::vector<std::string>;
using ISISJournalFilters = std::map<std::string, std::string>;
using ISISJournalData = std::map<std::string, std::string>;

std::vector<ISISJournalData> DLLExport
getRunDataFromFile(std::string const &fileContents,
                   ISISJournalTags const &tags = ISISJournalTags(),
                   ISISJournalFilters const &filters = ISISJournalFilters());

std::vector<ISISJournalData> DLLExport
getRunData(std::string const &instrument, std::string const &cycle,
           ISISJournalTags const &tags = ISISJournalTags(),
           ISISJournalFilters const &filters = ISISJournalFilters());

std::vector<std::string>
    DLLExport getCycleListFromFile(std::string const &fileContents);

std::vector<std::string> DLLExport getCycleList(std::string const &instrument);

} // namespace ISISJournal
} // namespace DataHandling
} // namespace Mantid
