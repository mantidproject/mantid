#ifndef MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_
#define MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/System.h"
#include <typeinfo>

namespace Mantid {
namespace PythonInterface {

namespace Registry {
//-----------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------
struct PropertyValueHandler;

/**
 * The values that are held within a given C++ property type all have a
 * fixed type, required by static typing. This means that when passing
 * a value for a property from Python there must be a match between between
 * the types.
 *
 * This class defines a registry of mappings between a C++ type T and a
 * PropertyValueHandler object that is able to extract (or attempt to extract)
 * the correct C++ type for that property from a given Python object.
 */
class DLLExport TypeRegistry {
public:
  /// Register handlers for basic C++ types into the registry
  static void registerBuiltins();
  /// Subscribe a handler object for given template type
  template <typename HandlerType> static void subscribe() {
    subscribe(typeid(typename HandlerType::HeldType), new HandlerType);
  }
  /// Subscribe a handler object for a given typeinfo
  static void subscribe(const std::type_info &typeInfo,
                        PropertyValueHandler *handler);
  /// Lookup a handler base on a given type_info object
  static const PropertyValueHandler &retrieve(const std::type_info &typeInfo);
};
}
}
}

#endif /* MANTID_PYTHONINTERFACE_TYPEREGISTRY_H_*/
