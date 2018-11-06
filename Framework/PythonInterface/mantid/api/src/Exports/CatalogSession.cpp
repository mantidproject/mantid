// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CatalogSession.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/shared_ptr.hpp>

using Mantid::API::CatalogSession;
using namespace boost::python;

void export_CatalogSession() {
  register_ptr_to_python<boost::shared_ptr<CatalogSession>>();

  class_<CatalogSession, boost::noncopyable>("CatalogSession", no_init)
      .def("getSessionId", &CatalogSession::getSessionId, args("self"),
           "Get the session id string.");
}
