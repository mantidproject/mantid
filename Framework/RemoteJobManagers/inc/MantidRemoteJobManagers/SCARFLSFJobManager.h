// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGER_H
#define MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGER_H

#include "MantidKernel/DllConfig.h"
#include "MantidRemoteJobManagers/LSFJobManager.h"

namespace Mantid {
namespace RemoteJobManagers {
/**
SCARFLSFJobManager implements a remote job manager that knows how to
talk to the Platform LSF web service at the SCARF cluster. This is in
principle a generic Platform LSF web service, except for the
authentication mechanism.
*/
class DLLExport SCARFLSFJobManager
    : public Mantid::RemoteJobManagers::LSFJobManager {
public:
  void authenticate(const std::string &username,
                    const std::string &password) override;

  /// Ping the server to see if it is alive and responding.
  virtual bool ping();

  void logout(const std::string &username = std::string()) override;

private:
  std::string guessJobSubmissionAppName(const std::string &runnablePath,
                                        const std::string &jobOptions) override;

  /// helper to encode uri components (SCARF username / passwords)
  static std::string urlComponentEncode(const std::string &in);

  static std::string g_pingPath;
  static std::string g_logoutPath;
  static std::string g_pingBaseURL;
};

} // namespace RemoteJobManagers
} // namespace Mantid

#endif // MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGER_H
