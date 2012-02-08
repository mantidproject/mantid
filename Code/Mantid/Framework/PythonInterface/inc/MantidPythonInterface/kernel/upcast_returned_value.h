#ifndef MANITD_PYTHONINTERFACE_UPCASTRETURNEDVALUE_H_
#define MANITD_PYTHONINTERFACE_UPCASTRETURNEDVALUE_H_
/**
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
#include <boost/python/detail/prefix.hpp>
#include <boost/python/to_python_value.hpp>

namespace Mantid
{
  namespace PythonInterface
  {

    ///@cond
    namespace detail
    {
      /**
       * Converts a T object to a Python object and performs an upcast if it can
       */
      template<class T>
      struct to_python_value_with_upcast
      {
        inline PyObject* operator()(const T & p) const
        {
          PyObject *pyo = boost::python::to_python_value<T>()(p);
          PyTypeObject *const derivedType = TypeRegistry::getDerivedType(pyo);
          if( derivedType )
          {
            PyObject_SetAttrString(pyo, "__class__", (PyObject*)derivedType);
          }
          return pyo;
        }

        inline PyTypeObject const* get_pytype() const
        {
          return boost::python::converter::registered<T>::converters.to_python_target_type();
        }

      };

      /**
       * Template to prevent compilation unless the function
       * returns a shared_ptr
       */
      template <class R>
      struct upcast_returned_value_requires_return
#if defined(__GNUC__) && __GNUC__ >= 3 || defined(__EDG__)
      {}
#endif
      ;
    } // namespace detail
    ///@endcond

    /**
     * Implements the upcast_shared_ptr return_value_policy.
     * This defines the required an internal type apply::type
     * used by the return_value_policy mechanism
     */
    struct upcast_returned_value
    {
      template <class T>
      struct apply
      {
        typedef detail::to_python_value_with_upcast<T> type;
      };
    };

  }
} // namespace Mantid::PythonInterface

#endif /* MANITD_PYTHONINTERFACE_UPCASTRETURNEDVALUE_H_ */
