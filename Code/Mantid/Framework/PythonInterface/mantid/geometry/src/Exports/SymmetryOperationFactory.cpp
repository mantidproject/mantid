#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

void export_SymmetryOperationFactory()
{
    class_<SymmetryOperationFactoryImpl,boost::noncopyable>("SymmetryOperationFactoryImpl", no_init)
            .def("exists", &SymmetryOperationFactoryImpl::isSubscribed)
            .def("createSymOp", &SymmetryOperationFactoryImpl::createSymOp)
            .def("subscribedSymbols", &SymmetryOperationFactoryImpl::subscribedSymbols)
            .def("Instance", &SymmetryOperationFactory::Instance, return_value_policy<reference_existing_object>(),
                 "Returns a reference to the SymmetryOperationFactory singleton")
            .staticmethod("Instance")
            ;
}

