#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

namespace {
    boost::python::list createSymOps(SymmetryOperationFactoryImpl &self, const std::string &identifiers) {
        std::vector<SymmetryOperation> symOps = self.createSymOps(identifiers);

        boost::python::list pythonOperations;
        for(auto it = symOps.begin(); it != symOps.end(); ++it) {
            pythonOperations.append(*it);
        }

        return pythonOperations;
    }

}

void export_SymmetryOperationFactory()
{
    class_<SymmetryOperationFactoryImpl,boost::noncopyable>("SymmetryOperationFactoryImpl", no_init)
            .def("exists", &SymmetryOperationFactoryImpl::isSubscribed, "Returns true if the symmetry operation is supplied.")
            .def("createSymOp", &SymmetryOperationFactoryImpl::createSymOp, "Creates the symmetry operation from the supplied x,y,z-identifier.")
            .def("createSymOps", &createSymOps, "Creates a vector of SymmetryOperation objects from a semi-colon separated list of x,y,z-identifiers.")
            .def("subscribedSymbols", &SymmetryOperationFactoryImpl::subscribedSymbols, "Return all subscribed symbols.")
            .def("Instance", &SymmetryOperationFactory::Instance, return_value_policy<reference_existing_object>(),
                 "Returns a reference to the SymmetryOperationFactory singleton")
            .staticmethod("Instance")
            ;
}

