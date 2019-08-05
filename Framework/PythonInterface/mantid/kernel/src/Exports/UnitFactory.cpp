// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::UnitFactory;
using Mantid::Kernel::UnitFactoryImpl;
namespace Policies = Mantid::PythonInterface::Policies;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(UnitFactoryImpl)

void export_UnitFactory() {
  class_<UnitFactoryImpl, boost::noncopyable>("UnitFactoryImpl", no_init)
      .def("create", &UnitFactoryImpl::create, (arg("self"), arg("className")),
           "Creates a named unit if it exists in the factory")

      .def("getKeys", &UnitFactoryImpl::getKeys, arg("self"),
           return_value_policy<Policies::VectorToNumpy>(),
           "Returns a list of units available from the factory")

      .def("Instance", &UnitFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the UnitFactory singleton")
      .staticmethod("Instance");
}
