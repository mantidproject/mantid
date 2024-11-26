// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidGeometry/IDetector.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/WarningSuppressions.h"

#include "MantidPythonInterface/api/CloneMatrixWorkspace.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/RemoveConst.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/tuple.hpp>

#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;
using namespace Mantid::PythonInterface::Converters;
using namespace Mantid::PythonInterface::Policies;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(MatrixWorkspace)

namespace {
/// Typedef for data access, i.e. dataX,Y,E members
using data_modifier = Mantid::MantidVec &(MatrixWorkspace::*)(const std::size_t);

/// return_value_policy for read-only numpy array
using return_readonly_numpy = return_value_policy<VectorRefToNumpy<WrapReadOnly>>;
/// return_value_policy for read-write numpy array
using return_readwrite_numpy = return_value_policy<VectorRefToNumpy<WrapReadWrite>>;

//------------------------------- Overload macros ---------------------------
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// Overloads for yIndexOfX function which has 2 optional argument
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_yIndexOfXOverloads, MatrixWorkspace::yIndexOfX, 1, 3)
// Overloads for YUnitLabel which has 1 optional argument
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(MatrixWorkspace_YUnitLabelOverloads, YUnitLabel, 0, 2)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

/**
 * Set the values from an python array-style object into the given spectrum in
 * the workspace
 * @param self :: A reference to the calling object
 * @param accessor :: A member-function pointer to the data{X,Y,E} member that
 * will extract the writable values.
 * @param wsIndex :: The workspace index for the spectrum to set
 * @param values :: A numpy array. The length must match the size of the
 */
void setSpectrumFromPyObject(MatrixWorkspace &self, data_modifier accessor, const size_t wsIndex,
                             const boost::python::object &values) {
  if (NDArray::check(values)) {
    NDArrayToVector<double> converter(values);
    converter.copyTo((self.*accessor)(wsIndex));
  } else {
    PySequenceToVector<double> converter(values);
    converter.copyTo((self.*accessor)(wsIndex));
  }
}

/**
 * Set a workspace as monitor workspace for current workspace.
 *
 * @param self  :: A reference to the calling object
 * @param value :: The python pointer to the workspace to set
 */
void setMonitorWorkspace(MatrixWorkspace &self, const boost::python::object &value) {
  MatrixWorkspace_sptr monWS =
      std::dynamic_pointer_cast<MatrixWorkspace>(Mantid::PythonInterface::ExtractSharedPtr<Workspace>(value)());
  self.setMonitorWorkspace(monWS);
}
/**
 * @param self  :: A reference to the calling object
 *
 *@return weak pointer to monitor workspace used by python
 */
std::weak_ptr<Workspace> getMonitorWorkspace(MatrixWorkspace &self) {
  return std::weak_ptr<Workspace>(self.monitorWorkspace());
}
/**
 * Clear monitor workspace attached to for current workspace.
 *
 * @param self  :: A reference to the calling object
 */
void clearMonitorWorkspace(MatrixWorkspace &self) {
  MatrixWorkspace_sptr monWS;
  self.setMonitorWorkspace(monWS);
}

/**
 * @param self :: A reference to the calling object
 *
 * @return a list of associated spectrum numbers
 */
list getSpectrumNumbers(const MatrixWorkspace &self) {
  const auto &spectrumNums = self.indexInfo().spectrumNumbers();
  list spectra;

  for (const auto &index : spectrumNums) {
    spectra.append(static_cast<int32_t>(index));
  }

  return spectra;
}

/**
 * Set the X values from an python array-style object
 * @param self :: A reference to the calling object
 * @param wsIndex :: The workspace index for the spectrum to set
 * @param values :: A numpy array. The length must match the size of the
 */
void setXFromPyObject(MatrixWorkspace &self, const size_t wsIndex, const boost::python::object &values) {
  setSpectrumFromPyObject(self, &MatrixWorkspace::dataX, wsIndex, values);
}

/**
 * Set the Y values from an python array-style object
 * @param self :: A reference to the calling object
 * @param wsIndex :: The workspace index for the spectrum to set
 * @param values :: A numpy array. The length must match the size of the
 */
void setYFromPyObject(MatrixWorkspace &self, const size_t wsIndex, const boost::python::object &values) {
  setSpectrumFromPyObject(self, &MatrixWorkspace::dataY, wsIndex, values);
}

/**
 * Set the E values from an python array-style object
 * @param self :: A reference to the calling object
 * @param wsIndex :: The workspace index for the spectrum to set
 * @param values :: A numpy array. The length must match the size of the
 */
void setEFromPyObject(MatrixWorkspace &self, const size_t wsIndex, const boost::python::object &values) {
  setSpectrumFromPyObject(self, &MatrixWorkspace::dataE, wsIndex, values);
}

/**
 * Set the Dx values from an python array-style object
 * @param self :: A reference to the calling object
 * @param wsIndex :: The workspace index for the spectrum to set
 * @param values :: A numpy array. The length must match the size of the
 */
void setDxFromPyObject(MatrixWorkspace &self, const size_t wsIndex, const boost::python::object &values) {
  setSpectrumFromPyObject(self, &MatrixWorkspace::dataDx, wsIndex, values);
}

/**
 * Adds a deprecation warning to the getSampleDetails call to warn about using
 * getRun instead
 * @param self A reference to the calling object
 * @returns getRun()
 */
Mantid::API::Run &getSampleDetailsDeprecated(MatrixWorkspace &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "``getSampleDetails`` is deprecated, use ``getRun`` instead.");
  return self.mutableRun();
}

/**
 * Adds a deprecation warning to the getNumberBins call to warn about using
 * blocksize instead
 * @param self A reference to the calling object
 * @returns The blocksize()
 */
std::size_t getNumberBinsDeprecated(MatrixWorkspace &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "``getNumberBins`` is deprecated, use ``blocksize`` instead.");
  return self.blocksize();
}

/**
 * Adds a deprecation warning to the binIndexOf call to warn about using
 * yIndexOfX instead
 * @param self A reference to the calling object
 * @param xValue The x value to find the index for
 * @param index The index within the workspace to search within (default = 0)
 * @param tolerance The tolerance to accept between the passed xValue and the
 *                  stored value (default = 0.0)
 * @returns The yIndexOfX(xValue, index, tolerance)
 */
std::size_t binIndexOfDeprecated(MatrixWorkspace &self, const double xValue, const std::size_t index = 0) {
  PyErr_Warn(PyExc_DeprecationWarning, "``binIndexOf`` is deprecated, use ``yIndexOfX`` instead.");
  return self.yIndexOfX(xValue, index);
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// Overloads for binIndexOfDeprecated function which has 1 optional argument
BOOST_PYTHON_FUNCTION_OVERLOADS(binIndexOfDeprecatedOverloads, binIndexOfDeprecated, 2, 3)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

/**
 * This is an anonymous wrapper around the homonym method of MatrixWorkspace.
 * This takes int as argument since python does not speak unsigned.
 * @param self
 * @param i : workspace index [int]
 * @return bin indices of masked bins at the workspace index given
 */
std::vector<size_t> maskedBinsIndices(MatrixWorkspace &self, const int i) { return self.maskedBinsIndices(i); }

/**
 * Raw Pointer wrapper of replaceAxis to allow it to work with python
 * @param self
 * @param axisIndex :: The index of the axis to replace
 * @param newAxis :: A pointer to the new axis. The class will take ownership of
 * its clone.
 */
void pythonReplaceAxis(MatrixWorkspace &self, const std::size_t &axisIndex, Axis *newAxis) {
  self.replaceAxis(axisIndex, std::unique_ptr<Axis>(newAxis->clone(&self)));
}

/**
 * Wrapper around MatrixWorkspace::getSignalAtCoord API to allow us to pass in a
 * numpy array of coordinates and get a numpy array of signals out.
 * @param self :: A Matrix Workspace
 * @param npCoords :: An NDArray of coordinates with shape (n, 2)
 * @param normalization :: MDNormalization object specifying normalization type
 */
object getSignalAtCoord(MatrixWorkspace &self, const NDArray &npCoords,
                        const Mantid::API::MDNormalization &normalization) {
  if (npCoords.get_shape()[1] != 2) {
    throw std::invalid_argument("MatrixWorkspace::getSignalAtCoord - Input "
                                "array must have shape (n, 2)");
  }
  // Create our output array
  Py_intptr_t length = len(npCoords);
  auto *signalValues = new Mantid::signal_t[length];

  // Convert coords to a vector
  std::vector<Mantid::coord_t> coords = NDArrayToVector<Mantid::coord_t>(npCoords)();

  // Fill output array
  for (int i = 0; i < length; ++i) {
    std::array<Mantid::coord_t, 2> coord = {{coords[2 * i], coords[2 * i + 1]}};
    signalValues[i] = self.getSignalAtCoord(coord.data(), normalization);
  }
  PyObject *npSignalArray =
      Impl::wrapWithNDArray(signalValues, 1, &length, NumpyWrapMode::ReadOnly, OwnershipMode::Python);
  return object(handle<>(npSignalArray));
}

boost::python::tuple findY(MatrixWorkspace &self, double value, tuple start) {
  int64_t first = extract<int64_t>(start[0]);
  int64_t second = extract<int64_t>(start[1]);
  auto idx = self.findY(value, std::make_pair(first, second));
  return make_tuple(idx.first, idx.second);
}

/**
 * Gets the bin edges from one matrix workspace and applies them to another
 * workspace.
 * @param self :: The MatrixWorkspace whose bin edges are being set.
 * @param ws :: The MatrixWorkspace from which the bin edges are retrieved.
 * @param getIndex :: The index from which the bin edges are retrieved.
 * @param setIndex :: The index at which the bin edges are being set.
 */
void applyBinEdgesFromAnotherWorkspace(MatrixWorkspace &self, const MatrixWorkspace &ws, const size_t getIndex,
                                       const size_t setIndex) {
  self.setBinEdges(setIndex, ws.binEdges(getIndex));
}

/**
 * Gets the points from one matrix workspace and applies them to another
 * workspace.
 * @param self :: The MatrixWorkspace whose points are being set.
 * @param ws :: The MatrixWorkspace from which the points are retrieved.
 * @param getIndex :: The index from which the points are retrieved.
 * @param setIndex :: The index at which the points are being set.
 */
void applyPointsFromAnotherWorkspace(MatrixWorkspace &self, const MatrixWorkspace &ws, const size_t getIndex,
                                     const size_t setIndex) {
  self.setPoints(setIndex, ws.points(getIndex));
}

std::vector<size_t> getIndicesFromDetectorIDs(MatrixWorkspace &self, const boost::python::list &detIDs) {
  return self.getIndicesFromDetectorIDs(Converters::PySequenceToVector<int>(detIDs)());
}

} // namespace

/** Python exports of the Mantid::API::MatrixWorkspace class. */
void export_MatrixWorkspace() {
  /// Typedef to remove const qualifier on input detector shared_ptr. See
  /// Policies/RemoveConst.h for more details

  class_<MatrixWorkspace, boost::python::bases<ExperimentInfo, IMDWorkspace>, boost::noncopyable>("MatrixWorkspace",
                                                                                                  no_init)
      //--------------------------------------- Meta information
      //-----------------------------------------------------------------------
      .def("isRaggedWorkspace", &MatrixWorkspace::isRaggedWorkspace, arg("self"),
           "Returns true if the workspace is ragged (has differently sized "
           "spectra).")
      .def("blocksize", &MatrixWorkspace::blocksize, arg("self"), "Returns size of the Y data array")
      .def("getNumberBins", &MatrixWorkspace::getNumberBins, (arg("self"), arg("index")),
           "Returns the number of bins for a given histogram index.")
      .def("getNumberBins", &getNumberBinsDeprecated, arg("self"),
           "Returns size of the Y data array (deprecated, use "
           ":class:`~mantid.api.MatrixWorkspace.blocksize` "
           "instead)")
      .def("getMaxNumberBins", &MatrixWorkspace::getMaxNumberBins, arg("self"),
           "Returns the maximum number of bins in a workspace (works on ragged "
           "data).")
      .def("getNumberHistograms", &MatrixWorkspace::getNumberHistograms, arg("self"),
           "Returns the number of spectra in the workspace")
      .def("getPlotType", &MatrixWorkspace::getPlotType, arg("self"), "Returns the plot type of the workspace")
      .def("getSpectrumNumbers", &getSpectrumNumbers, arg("self"),
           "Returns a list of all spectrum numbers in the workspace")
      .def("yIndexOfX", &MatrixWorkspace::yIndexOfX,
           MatrixWorkspace_yIndexOfXOverloads((arg("self"), arg("xvalue"), arg("workspaceIndex"), arg("tolerance")),
                                              "Returns the y index which corresponds to the X Value provided. "
                                              "The workspace_index [default=0] and tolerance [default=0.0] is "
                                              "optional."))
      .def("detectorTwoTheta", &MatrixWorkspace::detectorTwoTheta, (arg("self"), arg("det")),
           "Returns the two theta value for a given detector")
      .def("detectorSignedTwoTheta", &MatrixWorkspace::detectorSignedTwoTheta, (arg("self"), arg("det")),
           "Returns the signed two theta value for given detector")
      .def("getSpectrum", (ISpectrum & (MatrixWorkspace::*)(const size_t)) & MatrixWorkspace::getSpectrum,
           (arg("self"), arg("workspaceIndex")), return_internal_reference<>(),
           "Return the spectra at the given workspace index.")
      .def("getIndexFromSpectrumNumber", &MatrixWorkspace::getIndexFromSpectrumNumber, (arg("self"), arg("spec_no")),
           "Returns workspace index correspondent to the given spectrum "
           "number. Throws if no such spectrum is present in the workspace")
      .def("getIndicesFromDetectorIDs", &getIndicesFromDetectorIDs, (arg("self"), arg("detID_list")),
           "Returns a list of workspace indices from the corrresponding "
           "detector IDs.")
      .def("getDetector", &MatrixWorkspace::getDetector, return_value_policy<RemoveConstSharedPtr>(),
           (arg("self"), arg("workspaceIndex")),
           "Return the :class:`~mantid.geometry.Detector` or "
           ":class:`~mantid.geometry.DetectorGroup` that is linked to "
           "the given workspace index")
      .def("getRun", &MatrixWorkspace::mutableRun, arg("self"), return_internal_reference<>(),
           "Return the :class:`~mantid.api.Run` object for this workspace")
      .def("axes", &MatrixWorkspace::axes, arg("self"), "Returns the number of axes attached to the workspace")
      .def("getAxis", &MatrixWorkspace::getAxis, (arg("self"), arg("axis_index")), return_internal_reference<>(),
           "Get a pointer to a workspace axis")
      .def("isHistogramData", &MatrixWorkspace::isHistogramData, arg("self"),
           "Returns ``True`` if this is considered to be binned data.")
      .def("isDistribution", (bool(MatrixWorkspace::*)() const) & MatrixWorkspace::isDistribution, arg("self"),
           "Returns the status of the distribution flag")
      .def("YUnit", &MatrixWorkspace::YUnit, arg("self"),
           "Returns the current Y unit for the data (Y axis) in the workspace")
      .def("YUnitLabel", &MatrixWorkspace::YUnitLabel,
           MatrixWorkspace_YUnitLabelOverloads((arg("self"), arg("useLatex"), arg("plotAsDistribution")),
                                               "Returns the caption for the Y axis"))
      .def("hasAnyMaskedBins", &MatrixWorkspace::hasAnyMaskedBins, (arg("self")),
           "Returns true if any of the bins in this workspace are masked.")
      .def("hasMaskedBins", &MatrixWorkspace::hasMaskedBins, (arg("self"), arg("workspaceIndex")),
           "Returns true if this spectrum contains any masked bins")
      .def("maskedBinsIndices", &maskedBinsIndices, (arg("self"), arg("workspaceIndex")),
           "Returns all the masked bins' indices at the workspace index. "
           ":class:`~mantid.api.MatrixWorkspace.hasMaskedBins` MUST be called "
           "first to check if any bins are "
           "masked, otherwise an exception will be thrown")
      .def("findY", &findY, (arg("self"), arg("value"), arg("start") = make_tuple(0, 0)),
           "Find first index in Y equal to value. Start may be specified to "
           "begin at a specifc index. Returns tuple with the "
           "histogram and bin indices.")
      .def("getSampleDetails", &getSampleDetailsDeprecated, arg("self"), return_internal_reference<>(),
           "Return the Run object for this workspace (deprecated, use "
           ":class:`~mantid.api.MatrixWorkspace.getRun` "
           "instead)")
      .def("binIndexOf", &binIndexOfDeprecated,
           binIndexOfDeprecatedOverloads((arg("self"), arg("xvalue"), arg("workspaceIndex")),
                                         "Returns the index of the bin containing the given xvalue "
                                         "(deprecated, use "
                                         ":class:`~mantid.api.MatrixWorkspace.yIndexOfX` instead)"))

      //--------------------------------------- Setters
      //------------------------------------
      .def("setPlotType", &MatrixWorkspace::setPlotType, (arg("self"), arg("newType")),
           "Sets a new plot type for the workspace")
      .def("setYUnitLabel", &MatrixWorkspace::setYUnitLabel, (arg("self"), arg("newLabel")),
           "Sets a new caption for the data (Y axis) in the workspace")
      .def("setYUnit", &MatrixWorkspace::setYUnit, (arg("self"), arg("newUnit")),
           "Sets a new unit for the data (Y axis) in the workspace")
      .def("setDistribution", &MatrixWorkspace::setDistribution, (arg("self"), arg("newVal")),
           "Set distribution flag. If True the workspace has been divided by "
           "the bin-width.")
      .def("replaceAxis", &pythonReplaceAxis, (arg("self"), arg("axisIndex"), arg("newAxis")),
           "Replaces one of the workspace's axes with the new one provided. "
           "The axis is cloned.")
      .def("applyBinEdgesFromAnotherWorkspace", &applyBinEdgesFromAnotherWorkspace,
           (arg("self"), arg("ws"), arg("getIndex"), arg("setIndex")),
           "Sets the bin edges at setIndex to be the bin edges of ws at "
           "getIndex.")
      .def("applyPointsFromAnotherWorkspace", &applyPointsFromAnotherWorkspace,
           (arg("self"), arg("ws"), arg("getIndex"), arg("setIndex")),
           "Sets the points at setIndex to be the points of ws at getIndex.")

      //--------------------------------------- Read spectrum data
      //-------------------------
      .def("readX", &MatrixWorkspace::readX, (arg("self"), arg("workspaceIndex")), return_readonly_numpy(),
           "Creates a read-only numpy wrapper "
           "around the original X data at the "
           "given index")
      .def("readY", &MatrixWorkspace::readY, return_readonly_numpy(), args("self", "workspaceIndex"),
           "Creates a read-only numpy wrapper "
           "around the original Y data at the "
           "given index")
      .def("readE", &MatrixWorkspace::readE, return_readonly_numpy(), args("self", "workspaceIndex"),
           "Creates a read-only numpy wrapper "
           "around the original E data at the "
           "given index")
      .def("readDx", &MatrixWorkspace::readDx, return_readonly_numpy(), args("self", "workspaceIndex"),
           "Creates a read-only numpy wrapper "
           "around the original Dx data at the "
           "given index")
      .def("hasDx", &MatrixWorkspace::hasDx, args("self", "workspaceIndex"),
           "Returns True if the spectrum uses the DX (X Error) array, else "
           "False.")
      //--------------------------------------- Write spectrum data
      //------------------------
      .def("dataX", (data_modifier)&MatrixWorkspace::dataX, return_readwrite_numpy(), args("self", "workspaceIndex"),
           "Creates a writable numpy wrapper around the original X data at the "
           "given index")
      .def("dataY", (data_modifier)&MatrixWorkspace::dataY, return_readwrite_numpy(), args("self", "workspaceIndex"),
           "Creates a writable numpy wrapper around the original Y data at the "
           "given index")
      .def("dataE", (data_modifier)&MatrixWorkspace::dataE, return_readwrite_numpy(), args("self", "workspaceIndex"),
           "Creates a writable numpy wrapper around the original E data at the "
           "given index")
      .def("dataDx", (data_modifier)&MatrixWorkspace::dataDx, return_readwrite_numpy(), args("self", "workspaceIndex"),
           "Creates a writable numpy wrapper around the original Dx data at "
           "the given index")
      .def("setX", &setXFromPyObject, args("self", "workspaceIndex", "x"),
           "Set X values from a python list or numpy array. It performs a "
           "simple copy into the array.")
      .def("setY", &setYFromPyObject, args("self", "workspaceIndex", "y"),
           "Set Y values from a python list or numpy array. It performs a "
           "simple copy into the array.")
      .def("setE", &setEFromPyObject, args("self", "workspaceIndex", "e"),
           "Set E values from a python list or numpy array. It performs a "
           "simple copy into the array.")
      .def("setDx", &setDxFromPyObject, args("self", "workspaceIndex", "dX"),
           "Set Dx values from a python list or numpy array. It performs a "
           "simple copy into the array.")

      // --------------------------------------- Extract data
      // ------------------------------
      .def("extractX", Mantid::PythonInterface::cloneX, args("self"),
           "Extracts (copies) the X data from the workspace into a 2D numpy "
           "array. "
           "Note: This can fail for large workspaces as numpy will require a "
           "block "
           "of memory free that will fit all of the data.")
      .def("extractY", Mantid::PythonInterface::cloneY, args("self"),
           "Extracts (copies) the Y data from the workspace into a 2D numpy "
           "array. "
           "Note: This can fail for large workspaces as numpy will require a "
           "block "
           "of memory free that will fit all of the data.")
      .def("extractE", Mantid::PythonInterface::cloneE, args("self"),
           "Extracts (copies) the E data from the workspace into a 2D numpy "
           "array. "
           "Note: This can fail for large workspaces as numpy will require a "
           "block "
           "of memory free that will fit all of the data.")
      .def("extractDx", Mantid::PythonInterface::cloneDx, args("self"),
           "Extracts (copies) the E data from the workspace into a 2D numpy "
           "array. "
           "Note: This can fail for large workspaces as numpy will require a "
           "block "
           "of memory free that will fit all of the data.")
      .def("getSignalAtCoord", &getSignalAtCoord, args("self", "coords", "normalization"),
           "Return signal for array of coordinates")
      //-------------------------------------- Operators
      //-----------------------------------
      .def("equals", &Mantid::API::equals, args("self", "other", "tolerance"),
           "Performs a comparison operation on two workspaces, using the "
           "CompareWorkspaces algorithm")
      //---------   monitor workspace --------------------------------------
      .def("getMonitorWorkspace", &getMonitorWorkspace, args("self"),
           "Return internal monitor workspace bound to current workspace.")
      .def("setMonitorWorkspace", &setMonitorWorkspace, args("self", "MonitorWS"),
           "Set specified workspace as monitor workspace for"
           "current workspace. "
           "Note: The workspace does not have to contain monitors though "
           "some subsequent algorithms may expect it to be "
           "monitor workspace later.")
      .def("clearMonitorWorkspace", &clearMonitorWorkspace, args("self"),
           "Forget about monitor workspace, attached to the current workspace")
      .def("isCommonBins", &MatrixWorkspace::isCommonBins, "Returns true if the workspace has common X bins.")
      .def("isCommonLogBins", &MatrixWorkspace::isCommonLogBins,
           "Returns true if the workspace has common X bins with log spacing.");

  RegisterWorkspacePtrToPython<MatrixWorkspace>();
}
