#ifndef MANTID_DATAHANDLING_SAVEREFLTBL_H_
#define MANTID_DATAHANDLING_SAVEREFLTBL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveReflTBL SaveReflTBL.h DataHandling/SaveReflTBL.h

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
class DLLExport SaveReflTBL : public API::Algorithm {
public:
  /// Default constructor
  SaveReflTBL();
  /// Destructor
  ~SaveReflTBL() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SaveReflTBL"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Saves a table workspace to a reflectometry tbl format ascii file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method
  void exec();
  /// Writes a value to the file
  void writeVal(std::string &val, std::ofstream &file, bool endsep = true,
                bool endline = false);
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

#endif /*  MANTID_DATAHANDLING_SAVEREFLTBL_H_  */
