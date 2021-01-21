// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CatalogSession.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include <string>
#include <vector>

namespace Mantid {
namespace ICat {
class CatalogSearchParam;
}

namespace API {
/**
 This class creates an interface for information catalogs to support multiple
 facilities

 @author Sofia Antony, ISIS Rutherford Appleton Laboratory
 @date 23/09/2010
*/
class MANTID_API_DLL ICatalog {
public:
  /// Virtual destructor
  virtual ~ICatalog() = default;
  /// method to login to a catalog
  virtual CatalogSession_sptr login(const std::string &, const std::string &, const std::string &,
                                    const std::string &) = 0;
  /// logout from catalog
  virtual void logout() = 0;
  /// Search investigations
  virtual void search(const ICat::CatalogSearchParam &, ITableWorkspace_sptr &, const int &, const int &) = 0;
  /// Obtain the number of results returned by the search method.
  virtual int64_t getNumberOfSearchResults(const ICat::CatalogSearchParam &) = 0;
  /// search logged in users data
  virtual void myData(ITableWorkspace_sptr &) = 0;
  /// get datasets.
  virtual void getDataSets(const std::string &, ITableWorkspace_sptr &) = 0;
  /// get datafiles
  virtual void getDataFiles(const std::string &, ITableWorkspace_sptr &) = 0;
  ///  instrument list
  virtual void listInstruments(std::vector<std::string> &) = 0;
  /// get investigationtype lists
  virtual void listInvestigationTypes(std::vector<std::string> &) = 0;
  /// keep alive
  virtual void keepAlive() = 0;
};

using ICatalog_sptr = std::shared_ptr<ICatalog>;
using ICatalog_const_sptr = std::shared_ptr<const ICatalog>;
} // namespace API
} // namespace Mantid
