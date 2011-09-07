#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using Mantid::Kernel::PropertyWithValue;
using namespace boost::python;

void export_MatrixWorkspace()
{
  // Leave this here for now but move it if it needs expanding to add methods
  class_<IMDWorkspace, boost::python::bases<Workspace>, boost::noncopyable>("IMDWorkspace", no_init)
    ;

  register_ptr_to_python<MatrixWorkspace_sptr>();
  class_<MatrixWorkspace, boost::python::bases<ExperimentInfo,IMDWorkspace>, boost::noncopyable>("MatrixWorkspace", no_init)
    ;

}

void export_MatrixWorkspaceProperty()
{
  EXPORT_PROP_W_VALUE(MatrixWorkspace_sptr);
  register_ptr_to_python<WorkspaceProperty<MatrixWorkspace>*>();
  class_<WorkspaceProperty<MatrixWorkspace>, bases<PropertyWithValue<MatrixWorkspace_sptr>,IWorkspaceProperty>,
         boost::noncopyable>("WorkspaceProperty_MatrixWorkspace", no_init)
      ;
}

