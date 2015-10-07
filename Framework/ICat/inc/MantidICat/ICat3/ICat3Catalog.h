#ifndef MANTID_ICAT_ICAT3CATALOG_H_
#define MANTID_ICAT_ICAT3CATALOG_H_

#include "MantidAPI/ICatalog.h"
#include "MantidAPI/ICatalogInfoService.h"
#include "MantidICat/ICat3/ICat3ErrorHandling.h"
#include "MantidICat/ICat3/ICat3Helper.h"
#include "MantidICat/CatalogSearchParam.h"

namespace Mantid {
namespace ICat {
/**
This class is responsible for the implementation of ICat3 version based
information catalogs
@author Sofia Antony, ISIS Rutherford Appleton Laboratory
@date 20/10/2010
Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class ICat3Catalog : public Mantid::API::ICatalog,
                     public Mantid::API::ICatalogInfoService {
public:
  /// constructor
  ICat3Catalog();
  /// destructor
  virtual ~ICat3Catalog();
  /// login to isis catalog
  virtual API::CatalogSession_sptr login(const std::string &username,
                                         const std::string &password,
                                         const std::string &endpoint,
                                         const std::string &facility);
  /// logout from isis catalog
  virtual void logout();
  /// search isis data
  virtual void search(const CatalogSearchParam &inputs,
                      Mantid::API::ITableWorkspace_sptr &ws_sptr,
                      const int &offset, const int &limit);
  /// Obtain the number of results returned by the search method.
  virtual int64_t getNumberOfSearchResults(const CatalogSearchParam &inputs);
  /// logged in user's investigations search
  virtual void myData(Mantid::API::ITableWorkspace_sptr &mydataws_sptr);
  /// get datasets
  virtual void getDataSets(const std::string &investigationId,
                           Mantid::API::ITableWorkspace_sptr &datasetsws_sptr);
  /// get datafiles
  virtual void
  getDataFiles(const std::string &investigationId,
               Mantid::API::ITableWorkspace_sptr &datafilesws_sptr);
  /// get instruments list
  virtual void listInstruments(std::vector<std::string> &instruments);
  /// get investigationtypes list
  virtual void listInvestigationTypes(std::vector<std::string> &invstTypes);
  /// get file location strings
  virtual const std::string getFileLocation(const long long &fileid);
  /// get urls
  virtual const std::string getDownloadURL(const long long &fileid);
  /// get URL of where to PUT (publish) files.
  virtual const std::string
  getUploadURL(const std::string &investigationID,
               const std::string &createFileName,
               const std::string &dataFileDescription);
  /// keep alive
  virtual void keepAlive();
  /// Obtains the investigations that the user can publish to and saves related
  /// information to a workspace.
  virtual API::ITableWorkspace_sptr getPublishInvestigations();

private:
  /// The helper class that accesses ICAT functionality.
  CICatHelper *m_helper;
};
}
}

#endif
