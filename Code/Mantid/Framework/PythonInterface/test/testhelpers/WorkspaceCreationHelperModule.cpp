/******************************************************************************
 * Python module wrapper for the WorkspaceCreationHelper methods
 ******************************************************************************/
#include <boost/python/docstring_options.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/module.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

BOOST_PYTHON_MODULE(WorkspaceCreationHelper)
{
  using namespace boost::python;

  // Doc string options - User defined, python arguments, C++ call signatures
  docstring_options docstrings(true, true, false);

  // EventWorkspaces
  using namespace Mantid::DataObjects;
  register_ptr_to_python<boost::shared_ptr<EventWorkspace> >();
  class_<EventWorkspace, bases<Mantid::API::IEventWorkspace>,boost::noncopyable>("EventWorkspace", no_init);

  using namespace WorkspaceCreationHelper;
  def("CreateEventWorkspace", (EventWorkspace_sptr (*)())&CreateEventWorkspace);
  def("CreateEventWorkspace2", &CreateEventWorkspace2);
}
