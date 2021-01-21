// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/Property.h"
#include <memory>

namespace Mantid {
namespace API {

/** LogFilterGenerator : utility to generate a LogFilter, to filter by running
  status or period

  This was refactored out of MantidUI::importNumSeriesLog
*/
class MANTID_API_DLL LogFilterGenerator {
public:
  /// Types of filter that can be used
  enum class FilterType { None, Status, Period, StatusAndPeriod };

  /// Constructor taking workspace
  LogFilterGenerator(const FilterType filterType, const Mantid::API::MatrixWorkspace_const_sptr &workspace);

  /// Constructor taking run object
  LogFilterGenerator(const FilterType filterType, const Mantid::API::Run &run);

  /// Generate log filter from given workspace and log name
  std::unique_ptr<Mantid::Kernel::LogFilter> generateFilter(const std::string &logName) const;

private:
  /// Filter log by "running" status
  void filterByStatus(Mantid::Kernel::LogFilter *filter) const;
  /// Filter log by period
  void filterByPeriod(Mantid::Kernel::LogFilter *filter) const;
  /// Get log data from workspace
  Mantid::Kernel::Property *getLogData(const std::string &logName, bool warnIfNotFound = true) const;
  /// Type of filter
  const FilterType m_filterType;
  /// Run object containing logs
  const Mantid::API::Run m_run;
};

} // namespace API
} // namespace Mantid
