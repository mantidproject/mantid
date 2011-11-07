#ifndef MANTID_PYTHONINTERFACE_PROPERTY_HPP_
#define MANTID_PYTHONINTERFACE_PROPERTY_HPP_

/*
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

#include "MantidKernel/PropertyWithValue.h"
#include "MantidPythonInterface/kernel/PropertyMarshal.h"

#include <boost/python/bases.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/copy_const_reference.hpp>

/**
 * Define a macro to export PropertyWithValue template types
 */
#define EXPORT_PROP_W_VALUE(type)   \
  class_<Mantid::Kernel::PropertyWithValue<type>, \
     boost::python::bases<Mantid::Kernel::Property>, boost::noncopyable>("PropertyWithValue_"#type, boost::python::no_init) \
     .add_property("value", Mantid::PythonInterface::PropertyMarshal::value) \
     .add_property("value_as_declared", make_function(&Mantid::Kernel::PropertyWithValue<type>::operator(), \
       boost::python::return_value_policy<boost::python::copy_const_reference>())) \
   ;

#endif /* MANTID_PYTHONINTERFACE_PROPERTY_HPP_ */
