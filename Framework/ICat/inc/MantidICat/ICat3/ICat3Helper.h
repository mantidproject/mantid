// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CatalogSession.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/TableRow.h"
#include "MantidICat/CatalogSearchParam.h"
#include "MantidICat/ICat3/GSoapGenerated/ICat3ICATPortBindingProxy.h"

namespace Mantid {
namespace ICat {
/**  CatalogSearchHelper is a utility class used in Mantid ICat3 based
 information catalog
 class to connect  ICat API services using the gsoap generated proxy class and
 retrieve data from ICat services.

 @author Sofia Antony, ISIS Rutherford Appleton Laboratory
 @date 07/07/2010
 */

class CICatHelper {
public:
  /// constructor
  CICatHelper();

  /// search method
  int doSearch(ICat3::ICATPortBindingProxy &icat, std::shared_ptr<ICat3::ns1__searchByAdvanced> &request,
               ICat3::ns1__searchByAdvancedResponse &response);

  /// calls getInvestigationIncludes api's
  void getDataFiles(long long invstId, ICat3::ns1__investigationInclude include,
                    API::ITableWorkspace_sptr &responsews_sptr);

  /// this method calls Icat api getInvestigationIncludes and returns datasets
  /// for the given investigation id.
  void doDataSetsSearch(long long invstId, ICat3::ns1__investigationInclude include,
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
  void doAdvancedSearch(const CatalogSearchParam &inputs, API::ITableWorkspace_sptr &outputws, const int &offset,
                        const int &limit);

  /// Obtain the number of results returned by the doAdvancedSearch method.
  int64_t getNumberOfSearchResults(const CatalogSearchParam &inputs);

  // do login
  API::CatalogSession_sptr doLogin(const std::string &username, const std::string &password,
                                   const std::string &endpoint, const std::string &facility);

  /// get the url of the given file id
  const std::string getdownloadURL(const long long &fileId);

  /// get location of data file  or download method
  const std::string getlocationString(const long long &fileid);

private:
  /// This method saves the response data of search by run number to table
  /// workspace
  void saveSearchRessults(const ICat3::ns1__searchByAdvancedPaginationResponse &response,
                          API::ITableWorkspace_sptr &outputws);

  /// this method saves investigation include response to a table workspace
  void saveInvestigationIncludesResponse(const ICat3::ns1__getInvestigationIncludesResponse &response,
                                         API::ITableWorkspace_sptr &outputws);

  /// This method saves Datasets to a table workspace
  void saveDataSets(const ICat3::ns1__getInvestigationIncludesResponse &response, API::ITableWorkspace_sptr &outputws);

  /// This method saves the myinvestigations data to a table workspace
  void saveMyInvestigations(const ICat3::ns1__getMyInvestigationsIncludesResponse &response,
                            API::ITableWorkspace_sptr &outputws);

  /// save investigations
  void saveInvestigations(const std::vector<ICat3::ns1__investigation *> &investigations,
                          API::ITableWorkspace_sptr &outputws);

  /// Builds search query based on user input and stores query in related ICAT
  /// class.
  ICat3::ns1__advancedSearchDetails *buildSearchQuery(const CatalogSearchParam &inputs);

  // Defines the SSL authentication scheme.
  void setSSLContext(ICat3::ICATPortBindingProxy &icat);

  // Sets the soap-endpoint & SSL context for the given ICAT proxy.
  void setICATProxySettings(ICat3::ICATPortBindingProxy &icat);

  /** This is a template method to save data to table workspace
   * @param input :: pointer to input value
   * @param t :: table row reference
   */
  template <class T> void savetoTableWorkspace(const T *input, Mantid::API::TableRow &t) {
    if (input != nullptr) {
      t << *input;

    } else {
      t << "";
    }
  }
  // Stores the session details for a specific catalog.
  API::CatalogSession_sptr m_session;
};
} // namespace ICat
} // namespace Mantid