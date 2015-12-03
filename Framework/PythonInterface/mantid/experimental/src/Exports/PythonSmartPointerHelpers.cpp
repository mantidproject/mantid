#include "MantidAPI/Workspace.h"

#include <boost/python/def.hpp>
#include <boost/weak_ptr.hpp>

namespace {
using namespace Mantid::API;
boost::shared_ptr<Workspace>
convertWeakToShared(const boost::weak_ptr<Workspace> &weak) {
  return boost::shared_ptr<Workspace>(weak);
}

void swap(boost::shared_ptr<Workspace> &a, boost::shared_ptr<Workspace> &b) {
  a.swap(b);
}
}

void export_PythonSmartPointerHelpers() {
  using namespace Mantid::API;
  using namespace boost::python;

  using boost::python::def;

  def("convert_weak_ptr_to_shared_ptr", &::convertWeakToShared);
  def("swap_shared_ptrs", &::swap);
}
