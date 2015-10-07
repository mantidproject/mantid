#ifndef MANTID_ALGORITHMS_CONVERTTABLETOMATRIXWORKSPACE_H_
#define MANTID_ALGORITHMS_CONVERTTABLETOMATRIXWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Creates a single spectrum matrix workspace from some columns of a table
 workspace.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input table workspace. </LI>
 <LI> ColumnX - The column name for the X vector.
 <LI> ColumnY - The column name for the Y vector.
 <LI> ColumnE (optional) - The column name for the E vector. If not set the
 errors will be filled with 1s.
 <LI> OutputWorkspace - The name of the output matrix workspace. </LI>
 </UL>

 @author Roman Tolchenov, Tessella plc
 @date 25/01/2012

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
class DLLExport ConvertTableToMatrixWorkspace : public API::Algorithm {
public:
  /// (Empty) Constructor
  ConvertTableToMatrixWorkspace() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ConvertTableToMatrixWorkspace() {}
  /// Algorithm's name
  virtual const std::string name() const {
    return "ConvertTableToMatrixWorkspace";
  }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Creates a single spectrum matrix workspace from some columns of a "
           "table workspace.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility\\Workspaces"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTTABLETOMATRIXWORKSPACE_H_*/
