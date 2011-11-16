#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"
#include "MantidPythonInterface/kernel/SingleValueTypeHandler.h"
#include "MantidPythonInterface/api/WorkspaceToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

void export_MatrixWorkspace()
{
  // Leave this here for now but move it if it needs expanding to add methods
  class_<IMDWorkspace, boost::python::bases<Workspace>, boost::noncopyable>("IMDWorkspace", no_init)
    ;

  register_ptr_to_python<MatrixWorkspace_sptr>();
  class_<MatrixWorkspace, boost::python::bases<ExperimentInfo,IMDWorkspace>, boost::noncopyable>("MatrixWorkspace", no_init)
    //--------------------------------------- Meta information -----------------------------------------------------------------------
    .def("blocksize", &MatrixWorkspace::blocksize, "Returns size of the Y data array")
    .def("get_number_histograms", &MatrixWorkspace::getNumberHistograms, "Returns the number of spectra in the workspace")
    //--------------------------------------- Array access ---------------------------------------------------------------------------
    .def("read_x", &Mantid::PythonInterface::Numpy::wrapX,
          "Creates a read-only numpy wrapper around the original X data at the given index")
    .def("read_y", &Mantid::PythonInterface::Numpy::wrapY,
          "Creates a read-only numpy wrapper around the original Y data at the given index")
    .def("read_e", &Mantid::PythonInterface::Numpy::wrapE,
          "Creates a read-only numpy wrapper around the original E data at the given index")
    .def("extract_x", Mantid::PythonInterface::Numpy::cloneX, "Extracts (copies) the X data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block of memory free that will fit all of the data.")
    .def("extract_y", Mantid::PythonInterface::Numpy::cloneY, "Extracts (copies) the Y data from the workspace into a 2D numpy array. "
          "Note: This can fail for large workspaces as numpy will require a block of memory free that will fit all of the data.")
    .def("extract_e", Mantid::PythonInterface::Numpy::cloneE, "Extracts (copies) the E data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block of memory free that will fit all of the data.")
  ;

  DECLARE_SINGLEVALUETYPEHANDLER(MatrixWorkspace, DataItem_sptr);
}

void export_MatrixWorkspaceProperty()
{
  EXPORT_PROP_W_VALUE(MatrixWorkspace_sptr, MatrixWorkspace);
  register_ptr_to_python<WorkspaceProperty<MatrixWorkspace>*>();
  class_<WorkspaceProperty<MatrixWorkspace>, bases<PropertyWithValue<MatrixWorkspace_sptr>,IWorkspaceProperty>,
         boost::noncopyable>("WorkspaceProperty_MatrixWorkspace", no_init)
      ;
}

