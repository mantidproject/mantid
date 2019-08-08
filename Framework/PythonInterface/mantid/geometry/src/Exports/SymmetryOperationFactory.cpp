// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"
#include "MantidPythonInterface/core/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

namespace {
boost::python::list createSymOps(SymmetryOperationFactoryImpl &self,
                                 const std::string &identifiers) {
  std::vector<SymmetryOperation> symOps = self.createSymOps(identifiers);

  boost::python::list pythonOperations;
  for (auto &symOp : symOps) {
    pythonOperations.append(symOp);
  }

  return pythonOperations;
}
} // namespace

void export_SymmetryOperationFactory() {
  class_<SymmetryOperationFactoryImpl, boost::noncopyable>(
      "SymmetryOperationFactoryImpl", no_init)
      .def("exists", &SymmetryOperationFactoryImpl::isSubscribed,
           (arg("self"), arg("identifier")),
           "Returns true if the symmetry operation is supplied.")
      .def("createSymOp", &SymmetryOperationFactoryImpl::createSymOp,
           (arg("self"), arg("identifier")),
           "Creates the symmetry operation from the supplied x,y,z-identifier.")
      .def("createSymOps", &createSymOps, (arg("self"), arg("identifier")),
           "Creates a vector of SymmetryOperation objects from a semi-colon "
           "separated list of x,y,z-identifiers.")
      .def("subscribedSymbols",
           &SymmetryOperationFactoryImpl::subscribedSymbols, arg("self"),
           "Return all subscribed symbols.")
      .def("Instance", &SymmetryOperationFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the SymmetryOperationFactory singleton")
      .staticmethod("Instance");
}
