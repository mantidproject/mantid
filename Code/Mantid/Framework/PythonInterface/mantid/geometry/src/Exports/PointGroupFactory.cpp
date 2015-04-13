#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

namespace {
    PointGroup_sptr getPointGroupFromSpaceGroup(PointGroupFactoryImpl & self, const SpaceGroup &group)
    {
        return self.createPointGroupFromSpaceGroup(group);
    }

    PointGroup_sptr getPointGroupFromSpaceGroupSymbol(PointGroupFactoryImpl & self, const std::string &group)
    {
        return self.createPointGroupFromSpaceGroup(SpaceGroupFactory::Instance().createSpaceGroup(group));
    }
}

void export_PointGroupFactory()
{

    class_<PointGroupFactoryImpl,boost::noncopyable>("PointGroupFactoryImpl", no_init)
            .def("isSubscribed", &PointGroupFactoryImpl::isSubscribed, "Returns true of the point group with the given symbol is subscribed.")
            .def("createPointGroup", &PointGroupFactoryImpl::createPointGroup, "Creates a point group if registered.")
            .def("createPointGroupFromSpaceGroup", &getPointGroupFromSpaceGroup, "Creates the point group that corresponds to the given space group.")
            .def("createPointGroupFromSpaceGroupSymbol", &getPointGroupFromSpaceGroupSymbol, "Creates a point group directly from the space group symbol.")
            .def("getAllPointGroupSymbols", &PointGroupFactoryImpl::getAllPointGroupSymbols, "Returns all registered point group symbols.")
            .def("getPointGroupSymbols", &PointGroupFactoryImpl::getPointGroupSymbols, "Returns all point groups registered for the given crystal system.")
            .def("Instance", &PointGroupFactory::Instance, return_value_policy<reference_existing_object>(),
                 "Returns a reference to the PointGroupFactory singleton")
            .staticmethod("Instance")
            ;
}

