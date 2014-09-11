#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

namespace {
    using namespace Mantid::PythonInterface;

    SymmetryOperation_sptr createSymOpPython(SymmetryOperationFactoryImpl & self, const std::string &identifier)
    {
        SymmetryOperation_const_sptr constSymOp = self.createSymOp(identifier);

        return boost::const_pointer_cast<SymmetryOperation>(constSymOp);
    }
}

void export_SymmetryOperationFactory()
{

    class_<SymmetryOperationFactoryImpl,boost::noncopyable>("SymmetryOperationFactoryImpl", no_init)
            .def("exists", &SymmetryOperationFactoryImpl::exists)
            .def("createSymOp", &createSymOpPython)
            .def("getKeys", &SymmetryOperationFactoryImpl::getKeys, "Returns all registered symmetry operations")
            .def("Instance", &SymmetryOperationFactory::Instance, return_value_policy<reference_existing_object>(),
                 "Returns a reference to the SymmetryOperationFactory singleton")
            .staticmethod("Instance")
            ;
}

