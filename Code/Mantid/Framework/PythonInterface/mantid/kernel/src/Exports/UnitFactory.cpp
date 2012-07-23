#include "MantidKernel/UnitFactory.h"

#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::UnitFactory;
using Mantid::Kernel::UnitFactoryImpl;
namespace Policies = Mantid::PythonInterface::Policies;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;

void export_UnitFactory()
{
  class_<UnitFactoryImpl, boost::noncopyable>("UnitFactoryImpl", no_init)
    .def("getKeys", &UnitFactoryImpl::getKeys, return_value_policy<Policies::VectorToNumpy>(),
         "Returns a list of units available from the factory")

    .def("Instance", &UnitFactory::Instance, return_value_policy<reference_existing_object>(),
         "Returns a reference to the UnitFactory singleton")
    .staticmethod("Instance")
    ;
}

