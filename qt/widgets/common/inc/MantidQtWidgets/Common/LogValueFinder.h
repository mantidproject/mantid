// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidKernel/Statistics.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/** MDFLogValueFinder : Holds a collection of workspace names, and finds log
  values from them
*/
class EXPORT_OPT_MANTIDQT_COMMON LogValueFinder {
public:
  /// Constructor
  explicit LogValueFinder(std::vector<std::string> wsNames);
  /// Get log names from workspaces
  std::vector<std::string> getLogNames() const;
  /// Get log value from workspace position in list
  double getLogValue(const std::string &logName, const Mantid::Kernel::Math::StatisticType &function, int index) const;
  /// Get log value from workspace name
  double getLogValue(const std::string &logName, const Mantid::Kernel::Math::StatisticType &function,
                     const std::string &wsName) const;

private:
  /// Workspace names
  const std::vector<std::string> m_wsNames;
};

} // namespace MantidWidgets
} // namespace MantidQt
