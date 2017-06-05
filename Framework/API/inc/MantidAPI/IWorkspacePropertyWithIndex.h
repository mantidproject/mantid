#ifndef MANTID_API_IWORKSPACEPROPERTYWITHINDEX_H_
#define MANTID_API_IWORKSPACEPROPERTYWITHINDEX_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Workspace_fwd.h"

namespace Mantid {
namespace Kernel {
template <class T> class ArrayProperty;
}
namespace API {

class IndexTypeProperty;

/** An interface that is implemented by WorkspacePropertyWithIndex.
Used for non templated workspace operations.

@author Lamar Moore, STFC
@date 23/05/2017

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class IWorkspacePropertyWithIndex {
public:
  virtual Kernel::ArrayProperty<int> &mutableIndexListProperty() = 0;

  virtual const Kernel::ArrayProperty<int> &indexListProperty() const = 0;

  virtual IndexTypeProperty &mutableIndexTypeProperty() = 0;

  virtual const IndexTypeProperty &indexTypeProperty() const = 0;
  /// Virtual destructor
  virtual ~IWorkspacePropertyWithIndex() = default;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IWORKSPACEPROPERTYWITHINDEX_H_*/
