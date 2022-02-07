// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include <map>
#include <string>
#include <vector>

namespace Mantid {
namespace API {
/**
 * IJournal: Interface for classes that fetch information from journal files
 */
class MANTID_API_DLL IJournal {
public:
  using RunData = std::map<std::string, std::string>;

  IJournal() = default;
  virtual ~IJournal();
  IJournal(IJournal const &rhs) = delete;
  IJournal(IJournal &&rhs);
  IJournal const &operator=(IJournal const &rhs) = delete;
  IJournal &operator=(IJournal &&rhs);

  /// Get the list of cycle names
  virtual std::vector<std::string> getCycleNames() = 0;
  /// Get data for runs that match the given filters
  virtual std::vector<RunData> getRuns(std::vector<std::string> const &valuesToLookup = {},
                                       RunData const &filters = RunData()) = 0;
};
} // namespace API
} // namespace Mantid
