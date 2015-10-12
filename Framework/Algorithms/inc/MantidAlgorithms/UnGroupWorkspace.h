#ifndef MANTID_ALGORITHM_UNGROUP_H_
#define MANTID_ALGORITHM_UNGROUP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Takes the name of a group workspace as input and ungroups the workspaces.

    Required Property:
    <UL>
    <LI> InputWorkspace- The name of the workspace to ungroup </LI>
     </UL>

    @author Sofia Antony
    @date 21/07/2009

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport UnGroupWorkspace : public API::Algorithm {
public:
  /// Default constructor
  UnGroupWorkspace() : API::Algorithm(){};
  /// Destructor
  virtual ~UnGroupWorkspace(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "UnGroupWorkspace"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Takes a group workspace as input and ungroups the workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "Transforms\\Grouping;Utility\\Workspaces";
  }

private:
  /// Overridden Init method
  void init();
  /// overridden execute method
  void exec();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_UNGROUP_H_*/
