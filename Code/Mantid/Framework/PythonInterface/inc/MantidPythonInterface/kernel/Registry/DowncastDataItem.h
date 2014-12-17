#ifndef MANTID_PYTHONINTERFACE_DOWNCASTDATAITEM_H_
#define MANTID_PYTHONINTERFACE_DOWNCASTDATAITEM_H_
/**
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/DataItem.h"
#include "MantidPythonInterface/kernel/WeakPtr.h"

#include <boost/python/extract.hpp>
#include <boost/python/object.hpp> // for PyObject
#include <boost/python/to_python_value.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
/**
 * Interface class so that we can store derived objects in a map
 * It defines functions for converting shared_ptr<DataItem> into
 * Python objects of the required type
 */
struct DLLExport DowncastDataItem {
  virtual ~DowncastDataItem() {}
  /// Convert a shared_ptr<DataItem> to a python object that holds a shared_ptr
  virtual PyObject *
  toPythonAsSharedPtr(const Kernel::DataItem_sptr &data) const = 0;
  /// Convert a shared_ptr<DataItem> to a python object that holds a weak_ptr
  virtual PyObject *
  toPythonAsWeakPtr(const Kernel::DataItem_sptr &data) const = 0;
  /// Convert a Python object to a DataItem_sptr
  virtual Kernel::DataItem_sptr
  fromPythonAsSharedPtr(const boost::python::object &data) const = 0;
};

/**
 * Implementation of DowncastDataItem interface
 * @tparam CastedType The final type that the input item should be cast to when
 * going to Python
 */
template <typename PythonType>
struct DLLExport DowncastToType : public DowncastDataItem {
  typedef boost::shared_ptr<PythonType> PythonType_sptr;
  typedef boost::weak_ptr<PythonType> PythonType_wptr;

  /**
   * Convert a shared_ptr<DataItem> to a python object that holds a
   * shared_ptr<CastedType>
   * @param data The original C++ DataItem pointer
   * @returns A new PyObject holding the requested data
   */
  PyObject *toPythonAsSharedPtr(const Kernel::DataItem_sptr &data) const {
    using namespace boost::python;
    typedef to_python_value<PythonType_sptr> ToSharedValue;
    // boost python handles NULL pointers by converting them to None objects
    return ToSharedValue()(boost::dynamic_pointer_cast<PythonType>(data));
  }
  /**
   * Convert a shared_ptr<DataItem> to a python object that holds a
   * weak_ptr<CastedType>
   * @param data The original C++ DataItem pointer
   * @returns A new PyObject holding the requested data
   */
  PyObject *toPythonAsWeakPtr(const Kernel::DataItem_sptr &data) const {
    using namespace boost::python;
    typedef to_python_value<PythonType_wptr> ToWeakValue;
    return ToWeakValue()(
        PythonType_wptr(boost::dynamic_pointer_cast<PythonType>(data)));
  }
  /**
   * Convert a Python object to a boost::shared_ptr<DataItem>
   * @param data A Python object that should be holding either a
   * boost::shared_ptr<CastedType>
   *             or a boost::weak_ptr<CastedType>
   */
  Kernel::DataItem_sptr
  fromPythonAsSharedPtr(const boost::python::object &data) const {
    using namespace boost::python;
    // If we can extract a weak pointer then we must construct the shared
    // pointer
    // from the weak pointer itself to ensure the new shared_ptr has the correct
    // use count.
    // The order is important as if we try the shared_ptr first then
    // boost::python will
    // just construct a brand new shared pointer for the object rather than
    // converting from the
    // stored weak one.
    extract<PythonType_wptr> weakPtr(data);
    if (weakPtr.check()) {
      return boost::dynamic_pointer_cast<Kernel::DataItem>(weakPtr().lock());
    } else {
      return boost::dynamic_pointer_cast<Kernel::DataItem>(
          extract<PythonType_sptr &>(data)());
    }
  }
};

} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_DOWNCASTDATAITEM_H_ */
