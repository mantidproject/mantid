#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

#include "MantidPythonInterface/kernel/PropertyWithValue.h"
#include "MantidPythonInterface/kernel/SingleValueTypeHandler.h"
#include "MantidPythonInterface/api/WorkspaceToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using Mantid::Geometry::IDetector_sptr;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

namespace
{
  //------------------------------- Overload macros ---------------------------
  // Overloads for binIndexOf function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_binIndexOfOverloads,
                                         MatrixWorkspace::binIndexOf, 1, 2)
}

void export_MatrixWorkspace()
{

  register_ptr_to_python<MatrixWorkspace_sptr>();

  class_<MatrixWorkspace, boost::python::bases<ExperimentInfo,IMDWorkspace>, boost::noncopyable>("MatrixWorkspace", no_init)
    //--------------------------------------- Meta information -----------------------------------------------------------------------
    .def("blocksize", &MatrixWorkspace::blocksize, "Returns size of the Y data array")
    .def("getNumberHistograms", &MatrixWorkspace::getNumberHistograms, "Returns the number of spectra in the workspace")
    .def("binIndexOf", &MatrixWorkspace::binIndexOf, MatrixWorkspace_binIndexOfOverloads(args("xvalue", "workspace_index"),
         "Returns the index of the bin containing the given xvalue. The workspace_index is optional [default=0]"))
    .def("getSpectrum", (ISpectrum * (MatrixWorkspace::*)(const size_t))&MatrixWorkspace::getSpectrum,
       return_internal_reference<>(), "Return the spectra at the given workspace index.")
    .def("getDetector", (IDetector_sptr (MatrixWorkspace::*) (const size_t) const)&MatrixWorkspace::getDetector,
        "Return the Detector or DetectorGroup that is linked to the given workspace index")
    .def("axes", &MatrixWorkspace::axes, "Returns the number of axes attached to the workspace")
    .def("getAxis", &MatrixWorkspace::getAxis, return_internal_reference<>())
    .def("isHistogramData", &MatrixWorkspace::isHistogramData, "Returns True if this is conisdered to be binned data.")
    .def("YUnit", &MatrixWorkspace::YUnit, "Returns the current Y unit for the data (Y axis) in the workspace")
    .def("YUnitLabel", &MatrixWorkspace::YUnitLabel, "Returns the caption for the Y axis")
    //--------------------------------------- Setters -------------------------------------------------------------------------------
    .def("setYUnitLabel", &MatrixWorkspace::setYUnitLabel, "Sets a new caption for the data (Y axis) in the workspace")
    .def("setYUnit", &MatrixWorkspace::setYUnit, "Sets a new unit for the data (Y axis) in the workspace")
    .def("setDistribution", (bool& (MatrixWorkspace::*)(const bool))&MatrixWorkspace::isDistribution,
       return_value_policy<return_by_value>(), "Set distribution flag. If True the workspace has been divided by the bin-width.")
    .def("replaceAxis", &MatrixWorkspace::replaceAxis)
    //--------------------------------------- Data access ---------------------------------------------------------------------------
    .def("readX", &Mantid::PythonInterface::Numpy::readOnlyX,
          "Creates a read-only numpy wrapper around the original X data at the given index")
    .def("readY", &Mantid::PythonInterface::Numpy::readOnlyY,
          "Creates a read-only numpy wrapper around the original Y data at the given index")
    .def("readE", &Mantid::PythonInterface::Numpy::readOnlyE,
          "Creates a read-only numpy wrapper around the original E data at the given index")
    .def("readDx", &Mantid::PythonInterface::Numpy::readOnlyDx,
         "Creates a read-only numpy wrapper around the original Dx data at the given index")
    .def("dataX", &Mantid::PythonInterface::Numpy::readWriteX,
         "Creates a writable numpy wrapper around the original X data at the given index")
    .def("dataY", &Mantid::PythonInterface::Numpy::readWriteY,
         "Creates a writable numpy wrapper around the original Y data at the given index")
    .def("dataE", &Mantid::PythonInterface::Numpy::readWriteE,
         "Creates a writable numpy wrapper around the original E data at the given index")
    .def("dataDx", &Mantid::PythonInterface::Numpy::readWriteDx,
        "Creates a writable numpy wrapper around the original Dx data at the given index")
    .def("extractX", Mantid::PythonInterface::Numpy::cloneX, 
         "Extracts (copies) the X data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractY", Mantid::PythonInterface::Numpy::cloneY, 
         "Extracts (copies) the Y data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractE", Mantid::PythonInterface::Numpy::cloneE, 
         "Extracts (copies) the E data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractDx", Mantid::PythonInterface::Numpy::cloneDx,
         "Extracts (copies) the E data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
          "of memory free that will fit all of the data.")
    //-------------------------------------- Operators --------------------------------------------------------------------------------
    .def("equals", &Mantid::API::equals, "Performs a comparison operation on two workspaces, using the CheckWorkspacesMatch algorithm")
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

