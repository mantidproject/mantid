#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidGeometry/Objects/IObject.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IObject;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IObject)

void export_IObject() {
  register_ptr_to_python<boost::shared_ptr<IObject>>();

  class_<IObject, boost::noncopyable>("IObject", no_init);
}
