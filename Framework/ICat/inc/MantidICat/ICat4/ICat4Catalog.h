#ifndef MANTID_ICAT_ICAT4CATALOG_H_
#define MANTID_ICAT_ICAT4CATALOG_H_

#include "MantidAPI/ICatalog.h"
#include "MantidAPI/ICatalogInfoService.h"
#include "MantidAPI/TableRow.h"
#include "MantidICat/CatalogSearchParam.h"
#include "MantidICat/ICat4/GSoapGenerated/ICat4ICATPortBindingProxy.h"

namespace Mantid {
namespace ICat {
/**
This class is responsible for the implementation of ICat4 version based
information catalogs
@author Jay Rainey, ISIS Rutherford Appleton Laboratory
@date 12/08/2013
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
class ICat4Catalog : public Mantid::API::ICatalog,
                     public Mantid::API::ICatalogInfoService {
public:
  /// Constructor
  ICat4Catalog();
  /// Log the user into the catalog system.
  API::CatalogSession_sptr login(const std::string &username,
                                 const std::string &password,
                                 const std::string &endpoint,
                                 const std::string &facility) override;
  /// Log the user out of the catalog system.
  void logout() override;
  /// Search the catalog for data.
  void search(const CatalogSearchParam &inputs,
              Mantid::API::ITableWorkspace_sptr &outputws, const int &offset,
              const int &limit) override;
  /// Obtain the number of results returned by the search method.
  int64_t getNumberOfSearchResults(const CatalogSearchParam &inputs) override;
  /// Show the logged in user's investigations search results.
  void myData(Mantid::API::ITableWorkspace_sptr &outputws) override;
  /// Get datasets.
  void getDataSets(const std::string &investigationId,
                   Mantid::API::ITableWorkspace_sptr &outputws) override;
  /// Get datafiles
  void getDataFiles(const std::string &investigationId,
                    Mantid::API::ITableWorkspace_sptr &outputws) override;
  /// Get instruments list
  void listInstruments(std::vector<std::string> &instruments) override;
  /// Get investigationtypes list
  void listInvestigationTypes(std::vector<std::string> &invstTypes) override;
  /// Get the file location string(s) from archive.
  const std::string getFileLocation(const long long &fileID) override;
  /// Get the url(s) based on the fileID.
  const std::string getDownloadURL(const long long &fileID) override;
  /// get URL of where to PUT (publish) files.
  const std::string
  getUploadURL(const std::string &investigationID,
               const std::string &createFileName,
               const std::string &dataFileDescription) override;
  /// Obtains the investigations that the user can publish to and saves related
  /// information to a workspace.
  API::ITableWorkspace_sptr getPublishInvestigations() override;
  /// Keep current session alive
  void keepAlive() override;

private:
  // Ensures human friendly error messages are provided to the user.
  void throwErrorMessage(ICat4::ICATPortBindingProxy &icat);
  // Defines the SSL authentication scheme.
  void setSSLContext(ICat4::ICATPortBindingProxy &icat);
  // Saves "MyData" query result to output workspace.
  void saveInvestigations(std::vector<ICat4::xsd__anyType *> response,
                          API::ITableWorkspace_sptr &outputws);
  // Creates a search query string based on inputs provided by the user.
  std::string buildSearchQuery(const CatalogSearchParam &inputs);
  // Saves "DataFiles" result to output workspace.
  void saveDataFiles(std::vector<ICat4::xsd__anyType *> response,
                     API::ITableWorkspace_sptr &outputws);
  // Saves "DataSets" information to the output workspace.
  void saveDataSets(std::vector<ICat4::xsd__anyType *> response,
                    API::ITableWorkspace_sptr &outputws);
  // Convert a file size to human readable file format.
  std::string bytesToString(int64_t &fileSize);
  // Helper method that formats a given timestamp.
  std::string formatDateTime(const time_t &timestamp,
                             const std::string &format);
  // Search the archive & obtain the dataset ID based on the investigationID.
  int64_t getMantidDatasetId(const std::string &investigationID);
  // Creates a dataset for an investigation (based on ID) named 'mantid' if it
  // does not already exist.
  int64_t createMantidDataset(const std::string &investigationID);
  // Sets the soap-endpoint & SSL context for the given ICAT proxy.
  void setICATProxySettings(ICat4::ICATPortBindingProxy &icat);
  // Returns the results of a search against ICAT for a given query.
  std::vector<ICat4::xsd__anyType *>
  performSearch(ICat4::ICATPortBindingProxy &icat, std::string query);

  // Is the desired accessType allowed for a specific bean?
  template <class T>
  bool isAccessAllowed(const ICat4::ns1__accessType &accessType, T &bean);

  // Stores the session details for a specific catalog.
  API::CatalogSession_sptr m_session;

  /**
   * Template method to save data to table workspace
   * @param input :: Pointer to input value.
   * @param table :: Table row reference.
   */
  template <class T>
  void savetoTableWorkspace(T *input, Mantid::API::TableRow &table) {
    if (input) {
      table << *input;
    } else {
      table << "";
    }
  }
};
} // namespace ICat
} // namespace Mantid

#endif
