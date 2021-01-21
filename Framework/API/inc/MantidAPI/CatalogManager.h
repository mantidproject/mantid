// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ICatalog.h"
#include "MantidKernel/SingletonHolder.h"

#include <map>
#include <string>

namespace Mantid {
namespace API {
/**
 This class is a singleton and is responsible for creating, destroying, and
 managing catalogs.

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 26/02/2014
*/
class MANTID_API_DLL CatalogManagerImpl {
public:
  CatalogManagerImpl(const CatalogManagerImpl &) = delete;
  CatalogManagerImpl &operator=(const CatalogManagerImpl &) = delete;
  /// Creates a new catalog and session, and adds it to the activeCatalogs
  /// container.
  CatalogSession_sptr login(const std::string &username, const std::string &password, const std::string &endpoint,
                            const std::string &facility);
  /// Get a specific catalog using the sessionID.
  ICatalog_sptr getCatalog(const std::string &sessionID);
  /// Destroy a specific catalog (if session provided), otherwise destroys all
  /// active catalogs.
  void destroyCatalog(const std::string &sessionID);
  /// Obtains a list of the current active catalog sessions.
  std::vector<CatalogSession_sptr> getActiveSessions();
  /// Returns the number of active sessions
  size_t numberActiveSessions() const;

private:
  /// These methods are required to create a singleton.
  friend struct Kernel::CreateUsingNew<CatalogManagerImpl>;
  CatalogManagerImpl() = default;
  virtual ~CatalogManagerImpl() = default;

  // Holds a list of active catalogs and uses their sessionId as unique
  // identifier.
  std::map<CatalogSession_sptr, ICatalog_sptr> m_activeCatalogs;
};

using CatalogManager = Kernel::SingletonHolder<CatalogManagerImpl>;
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Kernel::SingletonHolder<Mantid::API::CatalogManagerImpl>;
}
} // namespace Mantid
