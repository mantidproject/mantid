// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CatalogManager.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>

#include <map>

using namespace Mantid::API;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(CatalogManagerImpl)

boost::python::list getActiveSessionsAsList(CatalogManagerImpl &self) {
  boost::python::list sessions;
  const auto vecSessions = self.getActiveSessions();
  for (const auto &vecSession : vecSessions) {
    sessions.append(vecSession);
  }
  return sessions;
}

void export_CatalogManager() {
  register_ptr_to_python<CatalogManagerImpl *>();

  class_<CatalogManagerImpl, boost::noncopyable>("CatalogManagerImpl", no_init)
      .def("numberActiveSessions", &CatalogManagerImpl::numberActiveSessions,
           "Number of active sessions open")
      .def("getActiveSessions", &getActiveSessionsAsList,
           "Get the active sessions")
      .def("Instance", &CatalogManager::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the CatalogManger singleton")
      .staticmethod("Instance");
}
