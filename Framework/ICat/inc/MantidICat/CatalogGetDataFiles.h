#ifndef MANTID_ICAT_CATALOGGETDATAFILES_H_
#define MANTID_ICAT_CATALOGGETDATAFILES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
 CatalogGetDataFiles obtains a list of datafiles and related information for an
investigation.

Required Properties:

<UL>
 <LI> InvestigationId - The id of the investigation to use for searching.</LI>
 <LI> OutputWorkspace - The workspace to store the datafile information.</LI>
</UL>

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
class MANTID_ICAT_DLL CatalogGetDataFiles : public API::Algorithm {
public:
  /// Constructor
  CatalogGetDataFiles() : API::Algorithm() {}
  /// Destructor
  ~CatalogGetDataFiles() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogGetDataFiles"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Obtains information of the datafiles associated to a specific "
           "investigation.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CatalogDownloadDataFiles", "CatalogGetDataSets", "CatalogLogin"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Catalog";
  }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
};
} // namespace ICat
} // namespace Mantid
#endif
