#include "MantidGeometry/Crystal/SymmetryElementFactory.h"

#include <boost/python/class.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

void export_SymmetryElementFactory()
{
    class_<SymmetryElementFactoryImpl,boost::noncopyable>("SymmetryElementFactoryImpl", no_init)
            .def("createSymElement", &SymmetryElementFactoryImpl::createSymElement)
            .def("Instance", &SymmetryElementFactory::Instance, return_value_policy<reference_existing_object>(),
                 "Returns a reference to the SymmetryElementFactory singleton")
            .staticmethod("Instance")
            ;
}
