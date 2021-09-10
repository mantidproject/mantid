// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <Poco/ActiveMethod.h>
#include <string>

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/** ErrorReporter : The error reporter is responsible for sending error reports
 */

class MANTID_KERNEL_DLL ErrorReporter {
public:
  /// Constructor
  ErrorReporter(const std::string &application, const Types::Core::time_duration &startTime,
                const std::string &exitCode, bool share);
  /// Constructor
  ErrorReporter(const std::string &application, const Types::Core::time_duration &startTime,
                const std::string &exitCode, bool share, const std::string &name, const std::string &email,
                const std::string &textBox);
  /// Constructor
  ErrorReporter(const std::string &application, const Types::Core::time_duration &startTime,
                const std::string &exitCode, bool share, const std::string &name, const std::string &email,
                const std::string &textBox, const std::string &stacktrace);
  /// Sends an error report
  int sendErrorReport();
  /// Generates an error string in json format
  virtual std::string generateErrorMessage() const;

protected:
  /// Sends report using Internet Helper
  virtual int sendReport(const std::string &message, const std::string &url);

private:
  const std::string m_application;
  const std::string m_exitCode;
  const Types::Core::time_duration m_upTime;
  const bool m_share;
  const std::string m_name;
  const std::string m_email;
  const std::string m_textbox;
  std::string m_url;
  const std::string m_stacktrace;
};

} // namespace Kernel
} // namespace Mantid
