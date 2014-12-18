#ifndef MANTID_PYTHONINTERFACE_DOWNCASTREGISTRY_H_
#define MANTID_PYTHONINTERFACE_DOWNCASTREGISTRY_H_
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
#include "MantidPythonInterface/kernel/Registry/DowncastDataItem.h"
#include <string>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
// We currently only expose up to the API level in Python. Due to the
// inner workings of boost::python this means that if a DataItem_sptr or
// Workspace_sptr is returned from a particular method then it is not
// automatically converted to the most derived pointer that boost::python
// knows about.
//
// In order for returned objects to be of any use in Python then they must be
// cast to the highest-type that has been exposed, i.e a Workspace2D should be
// return as a MatrixWorkspace or a MaskWorkspace should be returned as an
// IMaskWorkspace. Here we define a registry that allows the required mappings
// to be defined and used.
//
// The mappings are between the string returned by the id() method and a simple
// templated DowncastDataItem converter class.

/**
 * A simple static class with methods to subscribe and retrieve the relevant
 * DowncastDataItem object
 */
class DLLExport DowncastRegistry {
public:
  /**
   * Create an entry in the registry for a type given by the template type
   * that will be identified by the id string given
   * @param id string that will be returned by the concrete types id() method
   */
  template <typename CastedType> static void subscribe(const std::string &id) {
    subscribe(id, new DowncastToType<CastedType>());
  }
  /// Retrieve a registered casting object
  static const DowncastDataItem &retrieve(const std::string &id);

private:
  /// Implementation for the templated subscribe for a given id. Keeps impl out
  /// of header
  static void subscribe(const std::string &id, const DowncastDataItem *caster);
};
}
}
}

#endif /* MANTID_PYTHONINTERFACE_DOWNCASTREGISTRY_H_ */
