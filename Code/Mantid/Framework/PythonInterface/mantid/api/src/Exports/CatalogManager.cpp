#include "MantidAPI/CatalogManager.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using namespace boost::python;

boost::python::object getActiveSessionsAsList(CatalogManagerImpl& self)
{
  boost::python::list sessions;
  const auto vecSessions = self.getActiveSessions();
  for(auto itr = vecSessions.begin(); itr != vecSessions.end(); ++itr)
  {
    sessions.append(*itr);
  }
  return sessions;
}

void export_CatalogManager()
{
  register_ptr_to_python<CatalogManagerImpl*>();

  class_<CatalogManagerImpl,boost::noncopyable>("CatalogManagerImpl", no_init)
    .def("numberActiveSessions", &CatalogManagerImpl::numberActiveSessions, "Number of active sessions open")
    .def("getActiveSessions", &getActiveSessionsAsList, "Get the active sessions")
    .def("Instance", &CatalogManager::Instance, return_value_policy<reference_existing_object>(),
          "Returns a reference to the CatalogManger singleton")
    .staticmethod("Instance");

}
