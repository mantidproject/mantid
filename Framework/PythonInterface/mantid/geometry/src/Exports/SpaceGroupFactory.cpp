#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

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
}

void export_SpaceGroupFactory() {

  class_<SpaceGroupFactoryImpl, boost::noncopyable>("SpaceGroupFactoryImpl",
                                                    no_init)
      .def("isSubscribedSymbol", &isSubscribedSymbol,
           "Returns true if the space group the supplied symbol is subscribed.")
      .def("isSubscribedNumber", &isSubscribedNumber,
           "Returns true if a space group with the given number is subscribed.")
      .def("createSpaceGroup", &createSpaceGroup, "Creates a space group.")
      .def("getAllSpaceGroupSymbols", &allSpaceGroupSymbols,
           "Returns all subscribed space group symbols.")
      .def("getAllSpaceGroupNumbers",
           &SpaceGroupFactoryImpl::subscribedSpaceGroupNumbers,
           "Returns all subscribed space group numbers.")
      .def("subscribedSpaceGroupSymbols", &spaceGroupSymbolsForNumber,
           "Returns all space group symbols that are registered under the "
           "given number.")
      .def("getSpaceGroupsForPointGroup", &spaceGroupSymbolsForPointGroup)
      .def("Instance", &SpaceGroupFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the SpaceGroupFactory singleton")
      .staticmethod("Instance");
}
