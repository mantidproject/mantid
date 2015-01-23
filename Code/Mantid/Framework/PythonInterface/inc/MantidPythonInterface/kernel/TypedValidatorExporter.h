#ifndef MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORTER_H_
#define MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORTER_H_
/*
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
#include "MantidKernel/TypedValidator.h"
#include <boost/python/class.hpp>

namespace Mantid {
namespace PythonInterface {
/**
 * Declares a simple static struct to export a TypedValidator to Python
 * @tparam HeldType The type held within the validator
 */
template <typename Type> struct TypedValidatorExporter {
  static void define(const char *pythonClassName) {
    using namespace boost::python;
    using Mantid::Kernel::IValidator;
    using Mantid::Kernel::TypedValidator;

    class_<TypedValidator<Type>, bases<IValidator>, boost::noncopyable>(
        pythonClassName, no_init)
        .def("isValid", &IValidator::isValid<Type>,
             "Returns an empty string if the value is considered valid, "
             "otherwise a string defining the error is returned.");
  }
};

#define EXPORT_TYPEDVALIDATOR(Type)
}
}

#endif // MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORTER_H_
