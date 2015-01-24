#ifndef MANTID_ALGORITHMS_RENAMEWORKSPACE_H_
#define MANTID_ALGORITHMS_RENAMEWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Renames a workspace to a different name in the data service.
    If the same name is provided for input and output then the algorithm will
   fail with an error.
    The renaming is implemented as a removal of the original workspace from the
   data service
    and re-addition under the new name.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the to rename the workspace to </LI>
    </UL>

    Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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

    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport RenameWorkspace : public API::Algorithm {
public:
  /// Default constructor
  RenameWorkspace() : API::Algorithm(){};
  /// Destructor
  virtual ~RenameWorkspace(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "RenameWorkspace"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const { return "Rename the Workspace."; }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Utility\\Workspaces"; }

private:
  const std::string workspaceMethodName() const { return "rename"; }
  const std::string workspaceMethodInputProperty() const {
    return "InputWorkspace";
  }

  // Overridden Algorithm methods
  void init();
  void exec();

  bool processGroups();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_RENAMEWORKSPACE_H_*/
