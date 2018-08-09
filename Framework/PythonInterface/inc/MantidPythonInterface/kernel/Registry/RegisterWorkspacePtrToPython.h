#ifndef MANTID_PYTHONINTERFACE_DATEITEMINTERFACE_H_
#define MANTID_PYTHONINTERFACE_DATEITEMINTERFACE_H_
/**
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"
#include "MantidPythonInterface/kernel/WeakPtr.h"

#include <boost/python/register_ptr_to_python.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
/**
 * Encapsulates the registration required for an interface type T
 * that sits on top of a Kernel::DataItem object. The constructor
 * does 3 things:
 *    - Calls register_ptr_to_python<boost::shared_ptr<T>>
 *    - Calls register_ptr_to_python<boost::weak_ptr<T>>
 *    - Registers a new PropertyValueHandler for a boost::shared_ptr<T>
 */
template <typename IType> struct DLLExport RegisterWorkspacePtrToPython {
  using IType_sptr = boost::shared_ptr<IType>;
  using IType_wptr = boost::weak_ptr<IType>;
  /// Constructor
  RegisterWorkspacePtrToPython() {
    using namespace boost::python;
    using namespace Registry;

    register_ptr_to_python<IType_sptr>();
    register_ptr_to_python<IType_wptr>();
    // properties can only ever store pointers to these
    TypeRegistry::subscribe<TypedPropertyValueHandler<IType_sptr>>();
  }
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_DATEITEMINTERFACE_H_ */
