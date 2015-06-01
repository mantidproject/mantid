#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

#include "MantidPythonInterface/api/CloneMatrixWorkspace.h"
#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"

#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/numeric.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace boost::python;

namespace Converters = Mantid::PythonInterface::Converters;
namespace Policies = Mantid::PythonInterface::Policies;
namespace Registry = Mantid::PythonInterface::Registry;

namespace
{
  /// Typedef for data access, i.e. dataX,Y,E members
  typedef Mantid::MantidVec&(MatrixWorkspace::*data_modifier)(const std::size_t);

  /// return_value_policy for read-only numpy array
  typedef return_value_policy<Policies::VectorRefToNumpy<Converters::WrapReadOnly> > return_readonly_numpy;
  /// return_value_policy for read-write numpy array
  typedef return_value_policy<Policies::VectorRefToNumpy<Converters::WrapReadWrite> > return_readwrite_numpy;

  //------------------------------- Overload macros ---------------------------
  // Overloads for binIndexOf function which has 1 optional argument
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_binIndexOfOverloads,
                                         MatrixWorkspace::binIndexOf, 1, 2)

  /**
   * Set the values from an python array-style object into the given spectrum in the workspace
   * @param self :: A reference to the calling object
   * @param accessor :: A member-function pointer to the data{X,Y,E} member that will extract the writable values.
   * @param wsIndex :: The workspace index for the spectrum to set
   * @param values :: A numpy array. The length must match the size of the
   */
  void setSpectrumFromPyObject(MatrixWorkspace & self, data_modifier accessor,
                               const size_t wsIndex, numeric::array values)
  {
    boost::python::tuple shape(values.attr("shape"));
    if( boost::python::len(shape) != 1 )
    {
      throw std::invalid_argument("Invalid shape for setting 1D spectrum array, array is "
          + boost::lexical_cast<std::string>(boost::python::len(shape)) + "D");
    }
    const size_t pyArrayLength = boost::python::extract<size_t>(shape[0]);
    Mantid::MantidVec & wsArrayRef = (self.*accessor)(wsIndex);
    const size_t wsArrayLength = wsArrayRef.size();

    if(pyArrayLength != wsArrayLength)
    {
      throw std::invalid_argument("Length mismatch between workspace array & python array. ws="
            + boost::lexical_cast<std::string>(wsArrayLength) + ", python=" + boost::lexical_cast<std::string>(pyArrayLength));
    }
    for(size_t i = 0; i < wsArrayLength; ++i)
    {
      wsArrayRef[i] = extract<double>(values[i]);
    }
  }


  /**
   * Set the X values from an python array-style object
   * @param self :: A reference to the calling object
   * @param wsIndex :: The workspace index for the spectrum to set
   * @param values :: A numpy array. The length must match the size of the
   */
  void setXFromPyObject(MatrixWorkspace & self, const size_t wsIndex, numeric::array values)
  {
    setSpectrumFromPyObject(self, &MatrixWorkspace::dataX, wsIndex, values);
  }

  /**
   * Set the Y values from an python array-style object
   * @param self :: A reference to the calling object
   * @param wsIndex :: The workspace index for the spectrum to set
   * @param values :: A numpy array. The length must match the size of the
   */
  void setYFromPyObject(MatrixWorkspace & self, const size_t wsIndex, numeric::array values)
  {
    setSpectrumFromPyObject(self, &MatrixWorkspace::dataY, wsIndex, values);
  }

  /**
   * Set the E values from an python array-style object
   * @param self :: A reference to the calling object
   * @param wsIndex :: The workspace index for the spectrum to set
   * @param values :: A numpy array. The length must match the size of the
   */
  void setEFromPyObject(MatrixWorkspace & self, const size_t wsIndex, numeric::array values)
  {
    setSpectrumFromPyObject(self, &MatrixWorkspace::dataE, wsIndex, values);
  }

  /**
   * Adds a deprecation warning to the getNumberBins call to warn about using blocksize instead
   * @param self A reference to the calling object
   * @returns The blocksize()
   */
  size_t getNumberBinsDeprecated(MatrixWorkspace & self)
  {
    PyErr_Warn(PyExc_DeprecationWarning, "'getNumberBins' is deprecated, use 'blocksize' instead.");
    return self.blocksize();
  }

  /**
   * Adds a deprecation warning to the getSampleDetails call to warn about using getRun instead
   * @param self A reference to the calling object
   * @returns getRun()
   */
  Mantid::API::Run & getSampleDetailsDeprecated(MatrixWorkspace & self)
  {
    PyErr_Warn(PyExc_DeprecationWarning, "'getSampleDetails' is deprecated, use 'getRun' instead.");
    return self.mutableRun();
  }

}

/** Python exports of the Mantid::API::MatrixWorkspace class. */
void export_MatrixWorkspace()
{
  /// Typedef to remove const qualifier on input detector shared_ptr. See Policies/RemoveConst.h for more details
  typedef double (MatrixWorkspace::*getDetectorSignature)(Mantid::Geometry::IDetector_sptr det) const;

  class_<MatrixWorkspace, boost::python::bases<ExperimentInfo,IMDWorkspace>, boost::noncopyable>("MatrixWorkspace", no_init)
    //--------------------------------------- Meta information -----------------------------------------------------------------------
    .def("blocksize", &MatrixWorkspace::blocksize, args("self"), "Returns size of the Y data array")
    .def("getNumberHistograms", &MatrixWorkspace::getNumberHistograms, args("self"), 
         "Returns the number of spectra in the workspace")
    .def("binIndexOf", &MatrixWorkspace::binIndexOf,
          MatrixWorkspace_binIndexOfOverloads((arg("self"), arg("xvalue"), arg("workspaceIndex")),
         "Returns the index of the bin containing the given xvalue. The workspace_index is optional [default=0]"))
    .def("detectorTwoTheta", (getDetectorSignature)&MatrixWorkspace::detectorTwoTheta,
         args("self", "det"),
         "Returns the two theta value for a given detector")
    .def("detectorSignedTwoTheta",(getDetectorSignature)&MatrixWorkspace::detectorSignedTwoTheta,
         args("self", "det"),
         "Returns the signed two theta value for given detector")
    .def("getSpectrum", (ISpectrum * (MatrixWorkspace::*)(const size_t))&MatrixWorkspace::getSpectrum,
         return_internal_reference<>(), args("self", "workspaceIndex"), 
         "Return the spectra at the given workspace index.")
    .def("getIndexFromSpectrumNumber",&MatrixWorkspace::getIndexFromSpectrumNumber,args("self"),
          "Returns workspace index correspondent to the given spectrum number. Throws if no such spectrum is present in the workspace")
    .def("getDetector", &MatrixWorkspace::getDetector, return_value_policy<Policies::RemoveConstSharedPtr>(),
        args("self", "workspaceIndex"), 
         "Return the Detector or DetectorGroup that is linked to the given workspace index")
    .def("getRun", &MatrixWorkspace::mutableRun, return_internal_reference<>(),
             args("self"), "Return the Run object for this workspace")
    .def("axes", &MatrixWorkspace::axes, args("self"), "Returns the number of axes attached to the workspace")
    .def("getAxis", &MatrixWorkspace::getAxis, return_internal_reference<>(), args("self", "axis_index"))
    .def("isHistogramData", &MatrixWorkspace::isHistogramData, args("self"), 
         "Returns True if this is considered to be binned data.")
    .def("isDistribution", (const bool& (MatrixWorkspace::*)() const)&MatrixWorkspace::isDistribution,
         return_value_policy<copy_const_reference>(), args("self"), "Returns the status of the distribution flag")
    .def("YUnit", &MatrixWorkspace::YUnit, args("self"), 
         "Returns the current Y unit for the data (Y axis) in the workspace")
    .def("YUnitLabel", &MatrixWorkspace::YUnitLabel, args("self"), "Returns the caption for the Y axis")

    // Deprecated
    .def("getNumberBins", &getNumberBinsDeprecated, args("self"), 
         "Returns size of the Y data array (deprecated, use blocksize instead)")
    .def("getSampleDetails", &getSampleDetailsDeprecated, return_internal_reference<>(), args("self"), 
         "Return the Run object for this workspace (deprecated, use getRun instead)")

    //--------------------------------------- Setters ------------------------------------
    .def("setYUnitLabel", &MatrixWorkspace::setYUnitLabel, args("self", "newLabel"), 
         "Sets a new caption for the data (Y axis) in the workspace")
    .def("setYUnit", &MatrixWorkspace::setYUnit, args("self", "newUnit"), 
         "Sets a new unit for the data (Y axis) in the workspace")
    .def("setDistribution", (bool& (MatrixWorkspace::*)(const bool))&MatrixWorkspace::isDistribution,
         return_value_policy<return_by_value>(), args("self", "newVal"), 
         "Set distribution flag. If True the workspace has been divided by the bin-width.")
    .def("replaceAxis", &MatrixWorkspace::replaceAxis, args("self", "axisIndex", "newAxis"))

    //--------------------------------------- Read spectrum data -------------------------
    .def("readX", &MatrixWorkspace::readX, return_readonly_numpy(),
          args("self", "workspaceIndex"), 
         "Creates a read-only numpy wrapper around the original X data at the given index")
    .def("readY", &MatrixWorkspace::readY, return_readonly_numpy(),
         args("self", "workspaceIndex"),  
         "Creates a read-only numpy wrapper around the original Y data at the given index")
    .def("readE", &MatrixWorkspace::readE, return_readonly_numpy(),
          args("self", "workspaceIndex"), 
         "Creates a read-only numpy wrapper around the original E data at the given index")
    .def("readDx", &MatrixWorkspace::readDx, return_readonly_numpy(),
         args("self", "workspaceIndex"), 
         "Creates a read-only numpy wrapper around the original Dx data at the given index")

    //--------------------------------------- Write spectrum data ------------------------
    .def("dataX", (data_modifier)&MatrixWorkspace::dataX, return_readwrite_numpy(),
         args("self", "workspaceIndex"), 
         "Creates a writable numpy wrapper around the original X data at the given index")
    .def("dataY", (data_modifier)&MatrixWorkspace::dataY, return_readwrite_numpy(),
         args("self", "workspaceIndex"), 
         "Creates a writable numpy wrapper around the original Y data at the given index")
    .def("dataE", (data_modifier)&MatrixWorkspace::dataE, return_readwrite_numpy(),
         args("self", "workspaceIndex"), 
         "Creates a writable numpy wrapper around the original E data at the given index")
    .def("dataDx", (data_modifier)&MatrixWorkspace::dataDx, return_readwrite_numpy(),
        args("self", "workspaceIndex"), 
         "Creates a writable numpy wrapper around the original Dx data at the given index")
    .def("setX", &setXFromPyObject, args("self", "workspaceIndex", "x"), 
         "Set X values from a python list or numpy array. It performs a simple copy into the array.")
    .def("setY", &setYFromPyObject, args("self", "workspaceIndex", "y"), 
         "Set Y values from a python list or numpy array. It performs a simple copy into the array.")
    .def("setE", &setEFromPyObject, args("self", "workspaceIndex", "e"), 
         "Set E values from a python list or numpy array. It performs a simple copy into the array.")

    // --------------------------------------- Extract data ------------------------------
    .def("extractX", Mantid::PythonInterface::cloneX, args("self"),
         "Extracts (copies) the X data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractY", Mantid::PythonInterface::cloneY, args("self"),
         "Extracts (copies) the Y data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractE", Mantid::PythonInterface::cloneE, args("self"),
         "Extracts (copies) the E data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
         "of memory free that will fit all of the data.")
    .def("extractDx", Mantid::PythonInterface::cloneDx, args("self"),
         "Extracts (copies) the E data from the workspace into a 2D numpy array. "
         "Note: This can fail for large workspaces as numpy will require a block "
          "of memory free that will fit all of the data.")
    //-------------------------------------- Operators -----------------------------------
    .def("equals", &Mantid::API::equals, args("self", "other", "tolerance"), 
         "Performs a comparison operation on two workspaces, using the "
         "CheckWorkspacesMatch algorithm")
    ;
  
  //-------------------------------------------------------------------------------------------------

  static const int NUM_IDS = 7;
  static const char * WORKSPACE_IDS[NUM_IDS] = {\
      "GroupingWorkspace", "MaskWorkspace", "OffsetsWorkspace",
      "RebinnedOutput", "SpecialWorkspace2D", "Workspace2D", "WorkspaceSingleValue"
  };

  Registry::DataItemInterface<MatrixWorkspace> entry;
  for(int i = 0; i < NUM_IDS; ++i)
  {
    entry.castFromID(WORKSPACE_IDS[i]);
  }
  ;
}
