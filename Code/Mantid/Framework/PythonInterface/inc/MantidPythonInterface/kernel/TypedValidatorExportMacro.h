#ifndef MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORT_MACRO_H_
#define MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORT_MACRO_H_
/*
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/TypedValidator.h"
#include <boost/python/class.hpp>

/**
 * Defines an export for a validator of the given type
 */
#define EXPORT_TYPEDVALIDATOR(Type) \
  boost::python::class_<Mantid::Kernel::TypedValidator<Type>,\
                        boost::python::bases<Mantid::Kernel::IValidator>,\
                        boost::noncopyable\
                       >("TypedValidator_"#Type, boost::python::no_init)\
    .def("isValid", &Mantid::Kernel::TypedValidator<Type>::isValid<Type>, \
          "Returns an empty string if the value is considered valid, otherwise a string defining the error is returned.")

#endif // MANTID_PYTHONINTERFACE_TYPEDVALIDATOREXPORT_MACRO_H_