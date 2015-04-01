#include "MantidGeometry/Crystal/Group.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>

using Mantid::Geometry::Group;

using namespace boost::python;

void export_Group()
{
  enum_<Group::CoordinateSystem>("CoordinateSystem")
          .value("Orthogonal", Group::Orthogonal)
          .value("Hexagonal", Group::Hexagonal);

  class_<Group, boost::noncopyable>("Group", no_init)
          .def("coordinateSystem", &Group::getCoordinateSystem);
}

