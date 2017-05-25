#include "MantidAPI/IWorkspacePropertyWithIndex.h"
#include <boost/python/class.hpp>

void export_IWorkspacePropertyWithIndex() {
  using namespace boost::python;
  using Mantid::API::IWorkspacePropertyWithIndex;

  class_<IWorkspacePropertyWithIndex, boost::noncopyable>(
      "IWorkspacePropertyWithIndex", no_init);
}
