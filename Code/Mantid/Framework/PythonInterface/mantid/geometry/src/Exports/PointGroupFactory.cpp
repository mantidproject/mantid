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
            .def("exists", &PointGroupFactoryImpl::isSubscribed)
            .def("createPointGroup", &PointGroupFactoryImpl::createPointGroup)
            .def("createPointGroupFromSpaceGroup", &getPointGroupFromSpaceGroup)
            .def("createPointGroupFromSpaceGroupSymbol", &getPointGroupFromSpaceGroupSymbol)
            .def("getAllPointGroupSymbols", &PointGroupFactoryImpl::getAllPointGroupSymbols)
            .def("getPointGroupSymbols", &PointGroupFactoryImpl::getPointGroupSymbols)
            .def("Instance", &PointGroupFactory::Instance, return_value_policy<reference_existing_object>(),
                 "Returns a reference to the PointGroupFactory singleton")
            .staticmethod("Instance")
            ;
}

