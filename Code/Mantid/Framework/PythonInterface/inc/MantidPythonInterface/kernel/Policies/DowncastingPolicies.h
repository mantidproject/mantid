#ifndef MANITD_PYTHONINTERFACE_TOWEAKPTRWITHDOWNCASTIMPL_H_
#define MANITD_PYTHONINTERFACE_TOWEAKPTRWITHDOWNCASTIMPL_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

#include <boost/python/detail/prefix.hpp> // for PyObject

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace Mantid
{

  namespace PythonInterface
  {

    //--------------------------------------------------------------------------------
    // Implementations
    //--------------------------------------------------------------------------------
    ///@cond
    namespace // anonymous namespace with the implementation
    {
      template<typename ArgType>
      struct AsWeakPtr
      {
        static PyObject * apply(const Registry::DowncastDataItem & caster,
                                const ArgType & p)
        {
          return caster.toPythonAsWeakPtr(p);
        }
      };
      template<typename ArgType>
      struct AsSharedPtr
      {
        static PyObject * apply(const Registry::DowncastDataItem & caster,
                                const ArgType & p)
        {
          return caster.toPythonAsSharedPtr(p);
        }
      };

      /**
       * Converts a ArgType object to a Python object and performs an downcast if it can.
       * Only used for DataItem's at the moment so ArgType will always equal DataItem_sptr
       * but the template is kept for possible extension in the future
       */
      template<typename ArgType, typename CasterType>
      struct DowncastImpl
      {
        inline PyObject* operator()(const ArgType & p) const
        {
          if(!p) Py_RETURN_NONE;
          return CasterType::apply(Registry::DowncastRegistry::retrieve(p->id()), p);
        }

        inline PyTypeObject const* get_pytype() const
        {
          return boost::python::converter::registered<ArgType>::converters.to_python_target_type();
        }
      };

    } // end <anonymous>
    ///@endcond

    namespace Policies
    {
      /**
       * NOTE: These only workspace for functions/methods returning a shared_ptr<T>
       *       where T is convertible to a Kernel::DataItem as it requires
       *       the presence of an id() method
       */

      //--------------------------------------------------------------------------------
      // ToWeakPtrWithDowncast
      //--------------------------------------------------------------------------------
      /**
       * This defines the structure as required by boost::python.
       * If T is convertible to a shared_ptr<DataItem> then it calls
       * ToWeakPtrWithDownastImpl or else just return the value as is
       */
      struct ToWeakPtrWithDowncast
      {
        template <class T>
        struct apply
        {
          // if convertible to shared_ptr<DataItem> then call ToWeakPtrWithDownCastImpl or else
          // just return the value as is
          typedef typename boost::mpl::if_c<
              boost::is_convertible<T, boost::shared_ptr<Kernel::DataItem> >::value,
              DowncastImpl<T, AsWeakPtr<T> >,
              boost::python::to_python_value<T>
              >::type type;
        };
      };

      //--------------------------------------------------------------------------------
      // ToSharedPtrWithDowncast
      //--------------------------------------------------------------------------------
      /**
       * This defines the structure as required by boost::python.
       * If T is convertible to a shared_ptr<DataItem> then it calls
       * ToSharedPtrWithDowncastImpl or else just return the value as is
       */
      struct ToSharedPtrWithDowncast
      {
        template <class T>
        struct apply
        {
          // if convertible to shared_ptr<DataItem> then call ToWeakPtrWithDownCastImpl or else
          // just return the value as is
          typedef typename boost::mpl::if_c<
              boost::is_convertible<T, boost::shared_ptr<Kernel::DataItem> >::value,
              DowncastImpl<T,AsSharedPtr<T> >,
              boost::python::to_python_value<T>
              >::type type;
        };
      };


    } // Policies
  }
} // namespace Mantid::PythonInterface

#endif /* MANITD_PYTHONINTERFACE_TOWEAKPTRWITHDOWNCASTIMPL_H_ */
