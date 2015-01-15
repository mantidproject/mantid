#ifndef MANTID_ICAT_CATALOGMANAGERIMPL_H_
#define MANTID_ICAT_CATALOGMANAGERIMPL_H_

#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/ICatalog.h"

namespace Mantid {
namespace API {
/**
 This class is a singleton and is responsible for creating, destroying, and
 managing catalogs.

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 26/02/2014
 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL CatalogManagerImpl {
public:
  /// Creates a new catalog and session, and adds it to the activeCatalogs
  /// container.
  CatalogSession_sptr login(const std::string &username,
                            const std::string &password,
                            const std::string &endpoint,
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
  CatalogManagerImpl();
  CatalogManagerImpl(const CatalogManagerImpl &);
  CatalogManagerImpl &operator=(const CatalogManagerImpl &);
  virtual ~CatalogManagerImpl();

  // Holds a list of active catalogs and uses their sessionId as unique
  // identifier.
  std::map<CatalogSession_sptr, ICatalog_sptr> m_activeCatalogs;
};

#ifdef _WIN32
template class MANTID_API_DLL Kernel::SingletonHolder<CatalogManagerImpl>;
#endif
typedef MANTID_API_DLL Kernel::SingletonHolder<CatalogManagerImpl>
    CatalogManager;
}
}
#endif /* MANTID_ICAT_CATALOGMANAGERIMPL_H_ */
