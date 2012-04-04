#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/api/CloneMatrixWorkspace.h"

#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/implicit.hpp>

using namespace Mantid::API;
using Mantid::Geometry::IDetector_sptr;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::DataItem_sptr;
namespace Policies = Mantid::PythonInterface::Policies;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;

namespace
{
  /// Typedef for data access, i.e. dataX,Y,E members
  typedef Mantid::MantidVec&(MatrixWorkspace::*data_modifier)(const std::size_t);
  /// return_value_policy for read-only numpy array
  typedef return_value_policy<Policies::VectorToNumpy<Converters::WrapReadOnly> > return_readonly_numpy;
  /// return_value_policy for read-write numpy array
  typedef return_value_policy<Policies::VectorToNumpy<Converters::WrapReadWrite> > return_readwrite_numpy;

  //------------------------------- Overload macros ---------------------------
  // Overloads for binIndexOf function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_binIndexOfOverloads,
                                         MatrixWorkspace::binIndexOf, 1, 2)
}

void export_MatrixWorkspace()
{
  REGISTER_SHARED_PTR_TO_PYTHON(MatrixWorkspace);

  class_<MatrixWorkspace, boost::python::bases<ExperimentInfo,IMDWorkspace>, boost::noncopyable>("MatrixWorkspace", no_init)
    //--------------------------------------- Meta information -----------------------------------------------------------------------
    .def("blocksize", &MatrixWorkspace::blocksize, "Returns size of the Y data array")
    .def("getNumberHistograms", &MatrixWorkspace::getNumberHistograms, "Returns the number of spectra in the workspace")
    .def("binIndexOf", &MatrixWorkspace::binIndexOf, MatrixWorkspace_binIndexOfOverloads(args("xvalue", "workspace_index"),
         "Returns the index of the bin containing the given xvalue. The workspace_index is optional [default=0]"))
    .def("detectorTwoTheta", &MatrixWorkspace::detectorTwoTheta, "Returns the two theta value for a given detector")
    .def("detectorSignedTwoTheta",&MatrixWorkspace::detectorSignedTwoTheta, "Returns the signed two theta value for given detector")
    .def("getSpectrum", (ISpectrum * (MatrixWorkspace::*)(const size_t))&MatrixWorkspace::getSpectrum,
       return_internal_reference<>(), "Return the spectra at the given workspace index.")
    .def("getDetector", (IDetector_sptr (MatrixWorkspace::*) (const size_t) const)&MatrixWorkspace::getDetector,
        "Return the Detector or DetectorGroup that is linked to the given workspace index")
    .def("getRun", &MatrixWorkspace::mutableRun, return_internal_reference<>(),
             "Return the Run object for this workspace")
    .def("axes", &MatrixWorkspace::axes, "Returns the number of axes attached to the workspace")
    .def("getAxis", &MatrixWorkspace::getAxis, return_internal_reference<>())
    .def("isHistogramData", &MatrixWorkspace::isHistogramData, "Returns True if this is considered to be binned data.")
    .def("isDistribution", (const bool& (MatrixWorkspace::*)() const)&MatrixWorkspace::isDistribution,
         return_value_policy<copy_const_reference>(), "Returns the status of the distribution flag")
    .def("YUnit", &MatrixWorkspace::YUnit, "Returns the current Y unit for the data (Y axis) in the workspace")
    .def("YUnitLabel", &MatrixWorkspace::YUnitLabel, "Returns the caption for the Y axis")
    //--------------------------------------- Setters -------------------------------------------------------------------------------
    .def("setYUnitLabel", &MatrixWorkspace::setYUnitLabel, "Sets a new caption for the data (Y axis) in the workspace")
    .def("setYUnit", &MatrixWorkspace::setYUnit, "Sets a new unit for the data (Y axis) in the workspace")
    .def("setDistribution", (bool& (MatrixWorkspace::*)(const bool))&MatrixWorkspace::isDistribution,
       return_value_policy<return_by_value>(), "Set distribution flag. If True the workspace has been divided by the bin-width.")
    .def("replaceAxis", &MatrixWorkspace::replaceAxis)
    //--------------------------------------- Data access ---------------------------------------------------------------------------
    .def("readX", &MatrixWorkspace::readX, return_readonly_numpy(),
          "Creates a read-only numpy wrapper around the original X data at the given index")
    .def("readY", &MatrixWorkspace::readY, return_readonly_numpy(),
          "Creates a read-only numpy wrapper around the original Y data at the given index")
    .def("readE", &MatrixWorkspace::readE, return_readonly_numpy(),
          "Creates a read-only numpy wrapper around the original E data at the given index")
    .def("readDx", &MatrixWorkspace::readDx, return_readonly_numpy(),
         "Creates a read-only numpy wrapper around the original Dx data at the given index")
    .def("dataX", (data_modifier)&MatrixWorkspace::dataX, return_readwrite_numpy(),
         "Creates a writable numpy wrapper around the original X data at the given index")
    .def("dataY", (data_modifier)&MatrixWorkspace::dataY, return_readwrite_numpy(),
         "Creates a writable numpy wrapper around the original Y data at the given index")
    .def("dataE", (data_modifier)&MatrixWorkspace::dataE, return_readwrite_numpy(),
         "Creates a writable numpy wrapper around the original E data at the given index")
    .def("dataDx", (data_modifier)&MatrixWorkspace::dataDx, return_readwrite_numpy(),
        "Creates a writable numpy wrapper around the original Dx data at the given index")
    .def("extractX", Mantid::PythonInterface::cloneX,
         "Extracts (copies) the X data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractY", Mantid::PythonInterface::cloneY,
         "Extracts (copies) the Y data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractE", Mantid::PythonInterface::cloneE,
         "Extracts (copies) the E data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractDx", Mantid::PythonInterface::cloneDx,
         "Extracts (copies) the E data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
          "of memory free that will fit all of the data.")
    //-------------------------------------- Operators --------------------------------------------------------------------------------
    .def("equals", &Mantid::API::equals, "Performs a comparison operation on two workspaces, using the CheckWorkspacesMatch algorithm")
    ;

  REGISTER_SINGLEVALUE_HANDLER(MatrixWorkspace_sptr);
}
