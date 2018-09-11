#include "MantidPythonInterface/kernel/Registry/PropertyManagerFactory.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

#include <boost/make_shared.hpp>
#include <boost/python/extract.hpp>

using boost::python::extract;
using boost::python::handle;
using boost::python::object;

namespace Mantid {
using Kernel::Direction;
using Kernel::PropertyManager;

namespace PythonInterface {
namespace Registry {

/**
 * @param mapping A Python dictionary instance
 * @return A new C++ PropertyManager instance
 */
boost::shared_ptr<Kernel::PropertyManager>
createPropertyManager(const boost::python::dict &mapping) {
  auto pmgr = boost::make_shared<PropertyManager>();
#if PY_MAJOR_VERSION >= 3
  object view(mapping.attr("items")());
  object itemIter(handle<>(PyObject_GetIter(view.ptr())));
#else
  object itemIter(mapping.attr("iteritems")());
#endif
  auto length = len(mapping);
  for (ssize_t i = 0; i < length; ++i) {
    const object keyValue(handle<>(PyIter_Next(itemIter.ptr())));
    const std::string cppkey = extract<std::string>(keyValue[0])();
    pmgr->declareProperty(PropertyWithValueFactory::create(cppkey, keyValue[1],
                                                           Direction::Input));
  }
  return pmgr;
}
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid
