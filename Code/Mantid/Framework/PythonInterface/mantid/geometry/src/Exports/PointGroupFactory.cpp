#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

void export_PointGroupFactory()
{

    class_<PointGroupFactoryImpl,boost::noncopyable>("PointGroupFactoryImpl", no_init)
            .def("exists", &PointGroupFactoryImpl::isSubscribed)
            .def("createPointGroup", &PointGroupFactoryImpl::createPointGroup)
            .def("createPointGroupFromSpaceGroupSymbol", &PointGroupFactoryImpl::createPointGroupFromSpaceGroupSymbol)
            .def("getAllPointGroupSymbols", &PointGroupFactoryImpl::getAllPointGroupSymbols)
            .def("getPointGroupSymbols", &PointGroupFactoryImpl::getPointGroupSymbols)
            .def("Instance", &PointGroupFactory::Instance, return_value_policy<reference_existing_object>(),
                 "Returns a reference to the PointGroupFactory singleton")
            .staticmethod("Instance")
            ;
}

