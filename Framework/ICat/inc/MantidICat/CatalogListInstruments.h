#ifndef MANTID_ICAT_CATALOGLISTINSTRUMENTS_H_
#define MANTID_ICAT_CATALOGLISTINSTRUMENTS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
  This algorithm obtains a list of instruments types from the catalog.

  @author Sofia Antony, STFC Rutherford Appleton Laboratory
  @date 09/07/2010
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_ICAT_DLL CatalogListInstruments : public API::Algorithm {
public:
  /// constructor
  CatalogListInstruments() : API::Algorithm() {}
  /// destructor
  ~CatalogListInstruments() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogListInstruments"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override {
    return "Lists the name of instruments from all catalogs or a specific "
           "catalog based on session information.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CatalogListInvestigationTypes"};
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
}
}
#endif
