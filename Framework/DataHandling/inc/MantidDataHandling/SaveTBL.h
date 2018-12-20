#ifndef MANTID_DATAHANDLING_SAVETBL_H_
#define MANTID_DATAHANDLING_SAVETBL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveTBL SaveTBL.h DataHandling/SaveTBL.h

Saves a table workspace to a reflectometry tbl format ascii file.
Rows are 17 cells long and this save algorithm will throw if the workspace has
stitch groups of longer than 3 runs.

Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SaveTBL : public API::Algorithm {
public:
  /// Default constructor
  SaveTBL();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveTBL"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a table workspace to a reflectometry tbl format ascii file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadTBL"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
  /// Writes a value to the file
  template <class T>
  void writeVal(const T &val, std::ofstream &file, bool endsep = true,
                bool endline = false);
  void writeColumnNames(std::ofstream &file,
                        std::vector<std::string> const &columnHeadings);
  /// the separator
  const char m_sep;
  // populates the map and vector containing grouping information
  void findGroups(API::ITableWorkspace_sptr ws);
  /// Map the separator options to their string equivalents
  std::map<int, std::vector<size_t>> m_stichgroups;
  std::vector<size_t> m_nogroup;
};
} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SAVETBL_H_  */
