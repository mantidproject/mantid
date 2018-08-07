#ifndef MANTID_ICAT_ICAT3CATALOG_H_
#define MANTID_ICAT_ICAT3CATALOG_H_

#include "MantidAPI/ICatalog.h"
#include "MantidAPI/ICatalogInfoService.h"
#include "MantidICat/CatalogSearchParam.h"
#include "MantidICat/ICat3/ICat3ErrorHandling.h"
#include "MantidICat/ICat3/ICat3Helper.h"

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
  ~ICat3Catalog() override;
  /// login to isis catalog
  API::CatalogSession_sptr login(const std::string &username,
                                 const std::string &password,
                                 const std::string &endpoint,
                                 const std::string &facility) override;
  /// logout from isis catalog
  void logout() override;
  /// search isis data
  void search(const CatalogSearchParam &inputs,
              Mantid::API::ITableWorkspace_sptr &ws_sptr, const int &offset,
              const int &limit) override;
  /// Obtain the number of results returned by the search method.
  int64_t getNumberOfSearchResults(const CatalogSearchParam &inputs) override;
  /// logged in user's investigations search
  void myData(Mantid::API::ITableWorkspace_sptr &mydataws_sptr) override;
  /// get datasets
  void getDataSets(const std::string &investigationId,
                   Mantid::API::ITableWorkspace_sptr &datasetsws_sptr) override;
  /// get datafiles
  void
  getDataFiles(const std::string &investigationId,
               Mantid::API::ITableWorkspace_sptr &datafilesws_sptr) override;
  /// get instruments list
  void listInstruments(std::vector<std::string> &instruments) override;
  /// get investigationtypes list
  void listInvestigationTypes(std::vector<std::string> &invstTypes) override;
  /// get file location strings
  const std::string getFileLocation(const long long &fileID) override;
  /// get urls
  const std::string getDownloadURL(const long long &fileID) override;
  /// get URL of where to PUT (publish) files.
  const std::string
  getUploadURL(const std::string &investigationID,
               const std::string &createFileName,
               const std::string &dataFileDescription) override;
  /// keep alive
  void keepAlive() override;
  /// Obtains the investigations that the user can publish to and saves related
  /// information to a workspace.
  API::ITableWorkspace_sptr getPublishInvestigations() override;

private:
  /// The helper class that accesses ICAT functionality.
  CICatHelper *m_helper;
};
} // namespace ICat
} // namespace Mantid

#endif
