#ifndef MANTID_PYTHONINTERFACE_REGISTERSHAREDPTRTOPYTHON_H_
#define MANTID_PYTHONINTERFACE_REGISTERSHAREDPTRTOPYTHON_H_
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include <boost/shared_ptr.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/implicit.hpp>

/**
 * The standard boost::python register_ptr_to_python helper
 * does not register implicit conversions between
 * boost::shared_ptr<const T> and boost::shared_ptr<T>
 *
 * The macro registers a conversion for each shared_ptr and then
 * an implicit conversion between them both
 */
#define REGISTER_SHARED_PTR_TO_PYTHON(TYPE) \
  boost::python::register_ptr_to_python<boost::shared_ptr<TYPE> >();

#endif /* MANTID_PYTHONINTERFACE_REGISTERSHAREDPTRTOPYTHON_H_ */
