// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidAPI/ScriptRepository.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>

using Mantid::API::ScriptRepositoryFactory;
using Mantid::API::ScriptRepositoryFactoryImpl;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ScriptRepositoryFactoryImpl)

namespace {
///@cond

//------------------------------------------------------------------------------------------------------

///@endcond
} // namespace

void export_ScriptRepositoryFactory() {
  class_<ScriptRepositoryFactoryImpl, boost::noncopyable>(
      "ScriptRepositoryFactory", no_init)
      .def("create", &ScriptRepositoryFactoryImpl::create,
           (arg("self"), arg("class_name")),
           "Return a pointer to the ScriptRepository object")
      .def("Instance", &ScriptRepositoryFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the ScriptRepositoryFactory singleton")
      .staticmethod("Instance");
}
