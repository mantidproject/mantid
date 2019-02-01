// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_ERRORSERVICE_H_
#define MANTID_KERNEL_ERRORSERVICE_H_

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
  ErrorReporter(std::string application, Types::Core::time_duration startTime,
                std::string exitCode, bool share);
  /// Constructor
  ErrorReporter(std::string application, Types::Core::time_duration startTime,
                std::string exitCode, bool share, std::string name,
                std::string email, std::string textBox);
  /// Constructor
  ErrorReporter(std::string application, Types::Core::time_duration startTime,
                std::string exitCode, bool share, std::string name,
                std::string email, std::string textBox,
                std::string recoveryFile);
  /// Sends an error report
  int sendErrorReport();

protected:
  /// Generates an error string in json format
  virtual std::string generateErrorMessage();
  /// Sends report using Internet Helper
  virtual int sendReport(const std::string &message, const std::string &url);

private:
  const std::string m_application;

  /// Stores the exit code of mantid if it has crashed
  const std::string m_exitCode;
  /// The time mantid has been running
  const Types::Core::time_duration m_upTime;
  /// Whether to share additional information or not
  const bool m_share;
  /// User provided name
  const std::string m_name;
  /// User provided email
  const std::string m_email;
  /// User provided text box
  const std::string m_textbox;
  /// Target url
  std::string m_url;
  /// recovery file path
  const std::string m_recoveryFile;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_ERRORSERVICE_H_ */
