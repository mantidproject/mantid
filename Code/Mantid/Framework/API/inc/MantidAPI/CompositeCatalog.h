#ifndef MANTID_ICAT_COMPOSITECATALOG_H_
#define MANTID_ICAT_COMPOSITECATALOG_H_

#include "MantidAPI/ICatalog.h"

namespace Mantid {
namespace API {
/**
 CompositeCatalog is responsible for storing and performing options on multiple
 catalogues.

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 18/02/2014
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
class MANTID_API_DLL CompositeCatalog : public ICatalog {
public:
  /// Constructor
  CompositeCatalog();
  /// Adds a catalog to the list of catalogs (m_catalogs)
  void add(const ICatalog_sptr catalog);
  /// Log the user into the catalog system.
  virtual CatalogSession_sptr login(const std::string &username,
                                    const std::string &password,
                                    const std::string &endpoint,
                                    const std::string &facility);
  /// Log the user out of the catalog system.
  virtual void logout();
  /// Search the catalog for data.
  virtual void search(const ICat::CatalogSearchParam &inputs,
                      ITableWorkspace_sptr &outputws, const int &offset,
                      const int &limit);
  /// Obtain the number of results returned by the search method.
  virtual int64_t
  getNumberOfSearchResults(const ICat::CatalogSearchParam &inputs);
  /// Show the logged in user's investigations search results.
  virtual void myData(ITableWorkspace_sptr &outputws);
  /// Get datasets.
  virtual void getDataSets(const std::string &investigationId,
                           ITableWorkspace_sptr &outputws);
  /// Get datafiles
  virtual void getDataFiles(const std::string &investigationId,
                            ITableWorkspace_sptr &outputws);
  /// Get instruments list
  virtual void listInstruments(std::vector<std::string> &instruments);
  /// Get investigationtypes list
  virtual void listInvestigationTypes(std::vector<std::string> &invstTypes);
  /// Keep current session alive
  virtual void keepAlive();

private:
  std::list<ICatalog_sptr> m_catalogs;
};
}
}
#endif /* MANTID_ICAT_COMPOSITECATALOG_H_ */
