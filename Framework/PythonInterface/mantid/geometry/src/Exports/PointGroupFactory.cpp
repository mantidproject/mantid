// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidPythonInterface/core/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

namespace {
PointGroup_sptr getPointGroupFromSpaceGroup(PointGroupFactoryImpl &self,
                                            const SpaceGroup &group) {
  return self.createPointGroupFromSpaceGroup(group);
}

PointGroup_sptr getPointGroupFromSpaceGroupSymbol(PointGroupFactoryImpl &self,
                                                  const std::string &group) {
  return self.createPointGroupFromSpaceGroup(
      SpaceGroupFactory::Instance().createSpaceGroup(group));
}
} // namespace

void export_PointGroupFactory() {

  class_<PointGroupFactoryImpl, boost::noncopyable>("PointGroupFactoryImpl",
                                                    no_init)
      .def("isSubscribed", &PointGroupFactoryImpl::isSubscribed,
           (arg("self"), arg("hmSymbol")),
           "Returns true of the point group with the given symbol is "
           "subscribed.")
      .def("createPointGroup", &PointGroupFactoryImpl::createPointGroup,
           (arg("self"), arg("hmSymbol")),
           "Creates a point group if registered.")
      .def("createPointGroupFromSpaceGroup", &getPointGroupFromSpaceGroup,
           (arg("self"), arg("group")),
           "Creates the point group that corresponds to the given space group.")
      .def("createPointGroupFromSpaceGroupSymbol",
           &getPointGroupFromSpaceGroupSymbol, (arg("self"), arg("group")),
           "Creates a point group directly from the space group symbol.")
      .def("getAllPointGroupSymbols",
           &PointGroupFactoryImpl::getAllPointGroupSymbols, arg("self"),
           "Returns all registered point group symbols.")
      .def("getPointGroupSymbols", &PointGroupFactoryImpl::getPointGroupSymbols,
           (arg("self"), arg("crystalsystem")),
           "Returns all point groups registered for the given crystal system.")
      .def("Instance", &PointGroupFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the PointGroupFactory singleton")
      .staticmethod("Instance");
}
