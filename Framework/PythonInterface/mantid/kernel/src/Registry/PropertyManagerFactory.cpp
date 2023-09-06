// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/kernel/Registry/PropertyManagerFactory.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

#include <boost/python/extract.hpp>
#include <memory>

using boost::python::extract;
using boost::python::handle;
using boost::python::object;

namespace Mantid {
using Kernel::Direction;
using Kernel::PropertyManager;

namespace PythonInterface::Registry {

/**
 * @param mapping A Python dictionary instance
 * @return A new C++ PropertyManager instance
 */
std::shared_ptr<Kernel::PropertyManager> createPropertyManager(const boost::python::dict &mapping) {
  auto pmgr = std::make_shared<PropertyManager>();
  object view(mapping.attr("items")());
  object itemIter(handle<>(PyObject_GetIter(view.ptr())));
  auto length = len(mapping);
  for (boost::python::ssize_t i = 0; i < length; ++i) {
    const object keyValue(handle<>(PyIter_Next(itemIter.ptr())));
    const std::string cppkey = extract<std::string>(keyValue[0])();
    pmgr->declareProperty(PropertyWithValueFactory::create(cppkey, keyValue[1], Direction::Input));
  }
  return pmgr;
}
} // namespace PythonInterface::Registry
} // namespace Mantid
