#ifndef MANTID_PYTHONINTERFACE_PROPERTYMANAGERFACTORY_H
#define MANTID_PYTHONINTERFACE_PROPERTYMANAGERFACTORY_H
/*
  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include <boost/python/dict.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Kernel {
class PropertyManager;
}
namespace PythonInterface {
namespace Registry {

/// Create a C++ PropertyMananager from a Python dictionary
boost::shared_ptr<Kernel::PropertyManager>
createPropertyManager(const boost::python::dict &mapping);
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif // MANTID_PYTHONINTERFACE_PROPERTYMANAGERFACTORY_H
