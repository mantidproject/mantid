// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ICAT_COMPOSITECATALOG_H_
#define MANTID_ICAT_COMPOSITECATALOG_H_

#include "MantidAPI/ICatalog.h"

#include <list>
#include <set>

namespace Mantid {
namespace API {
/**
 CompositeCatalog is responsible for storing and performing options on multiple
 catalogues.

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 18/02/2014
*/
class MANTID_API_DLL CompositeCatalog : public ICatalog {
public:
  /// Constructor
  CompositeCatalog();
  /// Adds a catalog to the list of catalogs (m_catalogs)
  void add(const ICatalog_sptr catalog);
  /// Log the user into the catalog system.
  CatalogSession_sptr login(const std::string &username,
                            const std::string &password,
                            const std::string &endpoint,
                            const std::string &facility) override;
  /// Log the user out of the catalog system.
  void logout() override;
  /// Search the catalog for data.
  void search(const ICat::CatalogSearchParam &inputs,
              ITableWorkspace_sptr &outputws, const int &offset,
              const int &limit) override;
  /// Obtain the number of results returned by the search method.
  int64_t
  getNumberOfSearchResults(const ICat::CatalogSearchParam &inputs) override;
  /// Show the logged in user's investigations search results.
  void myData(ITableWorkspace_sptr &outputws) override;
  /// Get datasets.
  void getDataSets(const std::string &investigationId,
                   ITableWorkspace_sptr &outputws) override;
  /// Get datafiles
  void getDataFiles(const std::string &investigationId,
                    ITableWorkspace_sptr &outputws) override;
  /// Get instruments list
  void listInstruments(std::vector<std::string> &instruments) override;
  /// Get investigationtypes list
  void listInvestigationTypes(std::vector<std::string> &invstTypes) override;
  /// Keep current session alive
  void keepAlive() override;

private:
  std::list<ICatalog_sptr> m_catalogs;
};
} // namespace API
} // namespace Mantid
#endif /* MANTID_ICAT_COMPOSITECATALOG_H_ */
