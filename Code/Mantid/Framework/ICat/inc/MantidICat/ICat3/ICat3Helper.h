#ifndef MANTID_ICAT_SEARCHHELPER_H_
#define MANTID_ICAT_SEARCHHELPER_H_

#include "MantidICat/ICat3/GSoapGenerated/ICat3ICATPortBindingProxy.h"
#include "MantidICat/CatalogSearchParam.h"
#include "MantidAPI/CatalogSession.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace Mantid {
namespace ICat {
/**  CatalogSearchHelper is a utility class used in Mantid ICat3 based
 information catalog
 class to connect  ICat API services using the gsoap generated proxy class and
 retrieve data from ICat services.

 @author Sofia Antony, ISIS Rutherford Appleton Laboratory
 @date 07/07/2010
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

class CICatHelper {
public:
  /// constructor
  CICatHelper();

  /// search method
  int doSearch(ICat3::ICATPortBindingProxy &icat,
               boost::shared_ptr<ICat3::ns1__searchByAdvanced> &request,
               ICat3::ns1__searchByAdvancedResponse &response);

  /// calls getInvestigationIncludes api's
  void getDataFiles(long long invId, ICat3::ns1__investigationInclude include,
                    API::ITableWorkspace_sptr &responsews_sptr);

  /// this method calls Icat api getInvestigationIncludes and returns datasets
  /// for the given investigation id.
  void doDataSetsSearch(long long invId,
                        ICat3::ns1__investigationInclude include,
                        API::ITableWorkspace_sptr &responsews_sptr);

  /// This method lists the isntruments
  void listInstruments(std::vector<std::string> &instruments);

  /// This method lists the investigation types
  void listInvestigationTypes(std::vector<std::string> &investTypes);

  /// This method disconnects last connected  session from icat DB
  int doLogout();

  /// This method does investigations data search for logged in user
  void doMyDataSearch(API::ITableWorkspace_sptr &ws_sptr);

  /// do advanced search
  void doAdvancedSearch(const CatalogSearchParam &inputs,
                        API::ITableWorkspace_sptr &outputws, const int &offset,
                        const int &limit);

  /// Obtain the number of results returned by the doAdvancedSearch method.
  int64_t getNumberOfSearchResults(const CatalogSearchParam &inputs);

  // do login
  API::CatalogSession_sptr doLogin(const std::string &username,
                                   const std::string &password,
                                   const std::string &endpoint,
                                   const std::string &facility);

  /// get the url of the given file id
  const std::string getdownloadURL(const long long &fileId);

  /// get location of data file  or download method
  const std::string getlocationString(const long long &fileid);

private:
  /// This method saves the response data of search by run number to table
  /// workspace
  void saveSearchRessults(
      const ICat3::ns1__searchByAdvancedPaginationResponse &response,
      API::ITableWorkspace_sptr &outputws);

  /// this method saves investigation include response to a table workspace
  void saveInvestigationIncludesResponse(
      const ICat3::ns1__getInvestigationIncludesResponse &response,
      API::ITableWorkspace_sptr &outputws);

  /// This method saves Datasets to a table workspace
  void
  saveDataSets(const ICat3::ns1__getInvestigationIncludesResponse &response,
               API::ITableWorkspace_sptr &outputws);

  /// This method saves the myinvestigations data to a table workspace
  void saveMyInvestigations(
      const ICat3::ns1__getMyInvestigationsIncludesResponse &response,
      API::ITableWorkspace_sptr &outputws);

  /// save investigations
  void saveInvestigations(
      const std::vector<ICat3::ns1__investigation *> &investigations,
      API::ITableWorkspace_sptr &outputws);

  /// Builds search query based on user input and stores query in related ICAT
  /// class.
  ICat3::ns1__advancedSearchDetails *
  buildSearchQuery(const CatalogSearchParam &inputs);

  // Defines the SSL authentication scheme.
  void setSSLContext(ICat3::ICATPortBindingProxy &icat);

  // Sets the soap-endpoint & SSL context for the given ICAT proxy.
  void setICATProxySettings(ICat3::ICATPortBindingProxy &icat);

  /** This is a template method to save data to table workspace
   * @param input :: pointer to input value
   * @param t :: table row reference
   */
  template <class T>
  void savetoTableWorkspace(const T *input, Mantid::API::TableRow &t) {
    if (input != 0) {
      t << *input;

    } else {
      t << "";
    }
  }
  // Stores the session details for a specific catalog.
  API::CatalogSession_sptr m_session;
};
}
}
#endif
