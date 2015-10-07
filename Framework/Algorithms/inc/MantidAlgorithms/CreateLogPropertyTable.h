#ifndef MANTID_ALGORITHMS_CREATELOGPROPERTYTABLE_H_
#define MANTID_ALGORITHMS_CREATELOGPROPERTYTABLE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/Algorithm.h"

#include <string>

namespace Mantid {
namespace Algorithms {
/**

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CreateLogPropertyTable : public API::Algorithm {
public:
  /// (Empty) Constructor
  CreateLogPropertyTable() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~CreateLogPropertyTable() {}

  /// Algorithm's name for identification
  virtual const std::string name() const { return "CreateLogPropertyTable"; };
  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Utility;PythonAlgorithms";
  }

  /// Algorithm's summary
  virtual const std::string summary() const {
    return "  Takes a list of workspaces and a list of log property names.  "
           "For each workspace, the Run info is inspected and "
           "all log property values are used to populate a resulting output "
           "TableWorkspace.";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATELOGPROPERTYTABLE_H_ */
