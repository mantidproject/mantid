#ifndef MANTID_ICAT_CATALOGLISTINVESTIGATIONTYPES_H_
#define MANTID_ICAT_CATALOGLISTINVESTIGATIONTYPES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {

/**
 This algorithm obtains a list of investigation types from the catalog.

 @author Sofia Antony, STFC Rutherford Appleton Laboratory
 @date 12/08/2010
 Copyright &copy; 2010 STFC Rutherford Appleton Laboratory & NScD Oak Ridge
 National Laboratory

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
class MANTID_ICAT_DLL CatalogListInvestigationTypes : public API::Algorithm {
public:
  /// constructor
  CatalogListInvestigationTypes() : API::Algorithm() {}
  /// destructor
  ~CatalogListInvestigationTypes() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override {
    return "CatalogListInvestigationTypes";
  }
  /// Summary of algorithms purpose.
  const std::string summary() const override {
    return "Obtains a list of investigation types for active catalogs.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CatalogListInstruments"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Catalog";
  }

private:
  /// Overwrites Algorithm init method.
  void init() override;
  /// Overwrites Algorithm exec method
  void exec() override;
};
} // namespace ICat
} // namespace Mantid
#endif
