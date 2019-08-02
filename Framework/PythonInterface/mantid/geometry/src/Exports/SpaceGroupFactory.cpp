// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SpaceGroupFactoryImpl)

namespace {
using namespace Mantid::PythonInterface;

std::vector<std::string> allSpaceGroupSymbols(SpaceGroupFactoryImpl &self) {
  return self.subscribedSpaceGroupSymbols();
}

std::vector<std::string> spaceGroupSymbolsForNumber(SpaceGroupFactoryImpl &self,
                                                    size_t number) {
  return self.subscribedSpaceGroupSymbols(number);
}

std::vector<std::string>
spaceGroupSymbolsForPointGroup(SpaceGroupFactoryImpl &self,
                               const PointGroup_sptr &pointGroup) {
  return self.subscribedSpaceGroupSymbols(pointGroup);
}

bool isSubscribedSymbol(SpaceGroupFactoryImpl &self,
                        const std::string &symbol) {
  return self.isSubscribed(symbol);
}

bool isSubscribedNumber(SpaceGroupFactoryImpl &self, size_t number) {
  return self.isSubscribed(number);
}

SpaceGroup_sptr createSpaceGroup(SpaceGroupFactoryImpl &self,
                                 const std::string &symbol) {
  SpaceGroup_const_sptr spaceGroup = self.createSpaceGroup(symbol);
  return boost::const_pointer_cast<SpaceGroup>(spaceGroup);
}
} // namespace

void export_SpaceGroupFactory() {

  class_<SpaceGroupFactoryImpl, boost::noncopyable>("SpaceGroupFactoryImpl",
                                                    no_init)
      .def("isSubscribedSymbol", &isSubscribedSymbol,
           (arg("self"), arg("symbol")),
           "Returns true if the space group the supplied symbol is subscribed.")
      .def("isSubscribedNumber", &isSubscribedNumber,
           (arg("self"), arg("number")),
           "Returns true if a space group with the given number is subscribed.")
      .def("createSpaceGroup", &createSpaceGroup, (arg("self"), arg("symbol")),
           "Creates a space group.")
      .def("getAllSpaceGroupSymbols", &allSpaceGroupSymbols, arg("self"),
           "Returns all subscribed space group symbols.")
      .def("getAllSpaceGroupNumbers",
           &SpaceGroupFactoryImpl::subscribedSpaceGroupNumbers, arg("self"),
           "Returns all subscribed space group numbers.")
      .def("subscribedSpaceGroupSymbols", &spaceGroupSymbolsForNumber,
           (arg("self"), arg("number")),
           "Returns all space group symbols that are registered under the "
           "given number.")
      .def("getSpaceGroupsForPointGroup", &spaceGroupSymbolsForPointGroup,
           (arg("self"), arg("pointGroup")))
      .def("Instance", &SpaceGroupFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the SpaceGroupFactory singleton")
      .staticmethod("Instance");
}
