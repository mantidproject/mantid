#ifndef MANTID_PYTHONINTERFACE_DOWNCASTDATAITEM_H_
#define MANTID_PYTHONINTERFACE_DOWNCASTDATAITEM_H_
/**
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#include "MantidKernel/DataItem.h"
#include "MantidPythonInterface/kernel/WeakPtr.h"

#include <boost/python/detail/prefix.hpp> // for PyObject
#include <boost/python/to_python_value.hpp>

#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace Registry
    {
      /**
       * Interface class so that we can store derived objects in a map
       * It defines functions for converting shared_ptr<DataItem> into
       * Python objects of the required type
       */
      struct DLLExport DowncastDataItem
      {
        virtual ~DowncastDataItem() {}
        /// Convert a shared_ptr<DataItem> to a python object that holds a shared_ptr
        virtual PyObject* toPythonAsSharedPtr(const Kernel::DataItem_sptr & data) const = 0;
        /// Convert a shared_ptr<DataItem> to a python object that holds a weak_ptr
        virtual PyObject* toPythonAsWeakPtr(const Kernel::DataItem_sptr & data) const = 0;
      };

      /**
       * Implementation of DowncastDataItem interface
       * @tparam CastedType The final type that the input item should be cast to
       */
      template<typename CastedType>
      struct DLLExport DowncastToType : public DowncastDataItem
      {
        typedef boost::shared_ptr<CastedType> CastedType_sptr;
        typedef boost::weak_ptr<CastedType> CastedType_wptr;

        /**
         * Convert a shared_ptr<DataItem> to a python object that holds a shared_ptr<CastedType>
         * @param data The original C++ DataItem pointer
         * @returns A new PyObject holding the requested data
         */
        PyObject* toPythonAsSharedPtr(const Kernel::DataItem_sptr & data) const
        {
          using namespace boost::python;
          typedef to_python_value<CastedType_sptr> ToSharedValue;
          // boost python handles NULL pointers by converting them to None objects
          return ToSharedValue()(boost::dynamic_pointer_cast<CastedType>(data));
        }
        /**
         * Convert a shared_ptr<DataItem> to a python object that holds a weak_ptr<CastedType>
         * @param data The original C++ DataItem pointer
         * @returns A new PyObject holding the requested data
         */
        PyObject* toPythonAsWeakPtr(const Kernel::DataItem_sptr & data) const
        {
          using namespace boost::python;
          typedef to_python_value<CastedType_wptr> ToWeakValue;
          return ToWeakValue()(CastedType_wptr(boost::dynamic_pointer_cast<CastedType>(data)));
        }
      };

    } //namespace Registry
  } //namespace PythonInterface
} //namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_DOWNCASTDATAITEM_H_ */
