#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

namespace
{
    using namespace Mantid::PythonInterface;

    std::vector<std::string> allSpaceGroupSymbols(SpaceGroupFactoryImpl &self)
    {
        return self.subscribedSpaceGroupSymbols();
    }

    std::vector<std::string> spaceGroupSymbolsForNumber(SpaceGroupFactoryImpl &self, size_t number)
    {
        return self.subscribedSpaceGroupSymbols(number);
    }

    bool isSubscribedSymbol(SpaceGroupFactoryImpl &self, const std::string &symbol)
    {
        return self.isSubscribed(symbol);
    }

    bool isSubscribedNumber(SpaceGroupFactoryImpl &self, size_t number)
    {
        return self.isSubscribed(number);
    }

    SpaceGroup_sptr createSpaceGroup(SpaceGroupFactoryImpl &self, const std::string &symbol)
    {
        SpaceGroup_const_sptr spaceGroup = self.createSpaceGroup(symbol);

        return boost::const_pointer_cast<SpaceGroup>(spaceGroup);
    }

}

void export_SpaceGroupFactory()
{

    class_<SpaceGroupFactoryImpl,boost::noncopyable>("SpaceGroupFactoryImpl", no_init)
            .def("isSubscribedSymbol", &isSubscribedSymbol)
            .def("isSubscribedNumber", &isSubscribedNumber)
            .def("createSpaceGroup", &createSpaceGroup)
            .def("allSubscribedSpaceGroupSymbols", &allSpaceGroupSymbols)
            .def("subscribedSpaceGroupSymbols", &spaceGroupSymbolsForNumber)
            .def("subscribedSpaceGroupNumbers", &SpaceGroupFactoryImpl::subscribedSpaceGroupNumbers)
            .def("Instance", &SpaceGroupFactory::Instance, return_value_policy<reference_existing_object>(),
                 "Returns a reference to the SpaceGroupFactory singleton")
            .staticmethod("Instance")
            ;
}

