// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CatalogSession.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <memory>

using Mantid::API::CatalogSession;
using namespace boost::python;

void export_CatalogSession() {
  register_ptr_to_python<std::shared_ptr<CatalogSession>>();

  class_<CatalogSession, boost::noncopyable>("CatalogSession", no_init)
      .def("getSessionId", &CatalogSession::getSessionId, args("self"), return_value_policy<copy_const_reference>(),
           "Get the session id string.");
}
