#include "MantidGeometry/Instrument/CompAssembly.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::CompAssembly;
using Mantid::Geometry::Component;
using Mantid::Geometry::ICompAssembly;
using namespace boost::python;

/**
 * Enables boost.python to automatically "cast" an object up to the
 * appropriate CompAssembly leaf type
 */
void export_CompAssembly() {
  class_<CompAssembly, bases<ICompAssembly, Component>, boost::noncopyable>(
      "CompAssembly", no_init);
}
