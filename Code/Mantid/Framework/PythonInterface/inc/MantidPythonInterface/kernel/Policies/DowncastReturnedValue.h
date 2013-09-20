#ifndef MANITD_PYTHONINTERFACE_DOWNCASTRETURNEDVALUE_H_
#define MANITD_PYTHONINTERFACE_DOWNCASTRETURNEDVALUE_H_
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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#include "MantidPythonInterface/kernel/Registry/DowncastRegistry.h"
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <boost/python/detail/prefix.hpp>
#include <boost/python/to_python_value.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace Mantid
{
  //---------------------------------------------------------------------------
  // Forward declaration
  //---------------------------------------------------------------------------
  namespace Kernel
  {
    class DataItem;
  }
  namespace PythonInterface
  {

    ///@cond
    namespace
    {
      /**
       * Converts a T object to a Python object and performs an downcast if it can.
       * Only used for DataItem's at the moment
       */
      template<class T>
      struct ToPythonValueWithDowncast
      {
        inline PyObject* operator()(const T & p) const
        {
          PyObject *pyo = boost::python::to_python_value<T>()(p);
          const PyTypeObject * derivedType = Registry::getDerivedType(pyo);
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

    } // end <anonymous>
    ///@endcond

    namespace Policies
    {
      /**
       * Implements the downcast_shared_ptr return_value_policy.
       * This defines the required an internal type apply::type
       * used by the return_value_policy mechanism
       */
      struct DowncastReturnedValue
      {
        template <class T>
        struct apply
        {
          typedef typename boost::mpl::if_c<
              boost::mpl::or_<boost::is_convertible<T, boost::shared_ptr<Kernel::DataItem> >,
                              boost::is_convertible<T, boost::weak_ptr<Kernel::DataItem> > >::value
              , ToPythonValueWithDowncast<T>
              , boost::python::to_python_value<T>
              >::type type;
        };
      };

    } // Policies
  }
} // namespace Mantid::PythonInterface

#endif /* MANITD_PYTHONINTERFACE_DOWNCASTRETURNEDVALUE_H_ */
