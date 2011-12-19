#include "MantidAPI/ITableWorkspace.h"
#include "MantidPythonInterface/kernel/SingleValueTypeHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::Workspace;
using namespace boost::python;

void export_ITableWorkspace()
{
  register_ptr_to_python<ITableWorkspace_sptr>();

  class_<ITableWorkspace,bases<Workspace>, boost::noncopyable>("ITableWorkspace", no_init)
    ;

  DECLARE_SINGLEVALUETYPEHANDLER(ITableWorkspace, Mantid::Kernel::DataItem_sptr);
}

