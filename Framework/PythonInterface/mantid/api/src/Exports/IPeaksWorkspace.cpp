// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidPythonInterface/core/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <boost/python/class.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/return_internal_reference.hpp>

using namespace boost::python;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using Mantid::API::Column_sptr;
using Mantid::Kernel::V3D;
using Mantid::PythonInterface::Registry::RegisterWorkspacePtrToPython;

GET_POINTER_SPECIALIZATION(IPeaksWorkspace)

namespace {

/// Create a peak via it's HKL value from a list or numpy array
IPeak *createPeakHKL(IPeaksWorkspace &self, const object &data) {
  return self.createPeakHKL(
      Mantid::PythonInterface::Converters::PyObjectToV3D(data)());
}

/// Create a peak via it's QLab value from a list or numpy array
IPeak *createPeakQLab(IPeaksWorkspace &self, const object &data) {
  return self.createPeak(
      Mantid::PythonInterface::Converters::PyObjectToV3D(data)(), boost::none);
}

/// Create a peak via it's QLab value from a list or numpy array
IPeak *createPeakQLabWithDistance(IPeaksWorkspace &self, const object &data,
                                  double detectorDistance) {
  return self.createPeak(
      Mantid::PythonInterface::Converters::PyObjectToV3D(data)(),
      detectorDistance);
}
/// Create a peak via it's QLab value from a list or numpy array
void addPeak(IPeaksWorkspace &self, const IPeak &peak) { self.addPeak(peak); }

/**
 * PeakWorkspaceTableAdaptor
 *
 * A class to map PeaksWorkspace column names to IPeak setter functions. This
 * will handle the unmarshalling of the bpl::object type before passing it to
 * the appropriate setter method.
 */
class PeakWorkspaceTableAdaptor {
public:
  /**
   * Create a PeakWorkspaceTableAdaptor
   *
   * @param peaksWorkspace reference to a peaks workspace to convert values for.
   */
  explicit PeakWorkspaceTableAdaptor(IPeaksWorkspace &peaksWorkspace)
      : m_peaksWorkspace(peaksWorkspace) {
    // Create a map of string -> setter functions
    // Each function will extract the given value from the passed python type.
    m_setterMap = {{"RunNumber", setterFunction(&IPeak::setRunNumber)},
                   {"DetID", setterFunction(&IPeak::setDetectorID)},
                   {"h", setterFunction(&IPeak::setH)},
                   {"k", setterFunction(&IPeak::setK)},
                   {"l", setterFunction(&IPeak::setL)},
                   {"Wavelength", setterFunction(&IPeak::setWavelength)},
                   {"Intens", setterFunction(&IPeak::setIntensity)},
                   {"SigInt", setterFunction(&IPeak::setSigmaIntensity)},
                   {"BinCount", setterFunction(&IPeak::setBinCount)},
                   {"PeakNumber", setterFunction(&IPeak::setPeakNumber)},
                   {"QLab", setterFunction(&IPeak::setQLabFrame)},
                   {"QSample", setterFunction(&IPeak::setQSampleFrame)}};
  }

  /**
   * Set the value of a PeaksWorkspace at the given column name and row index.
   *
   * @param columnName name of the column to set a value for.
   * @param rowIndex index of the peak to set the value for.
   * @param value value to set the property to.
   */
  void setProperty(const std::string &columnName, const int rowIndex,
                   object value) {
    auto &peak = m_peaksWorkspace.getPeak(rowIndex);
    if (m_setterMap.find(columnName) == m_setterMap.end()) {
      throw std::runtime_error(columnName +
                               " is a read only column of a peaks workspace");
    }
    m_setterMap[columnName](peak, value);
  }

private:
  // type alias for the member function to wrap
  template <typename T> using MemberFunc = void (IPeak::*)(T value);
  // special type alias for V3D functions that take an addtional parameter
  using MemberFuncV3D = void (IPeak::*)(const V3D &value,
                                        boost::optional<double>);
  // type alias for the setter function
  using SetterType = std::function<void(IPeak &peak, const object)>;

  /**
   * Wrap a setter function on a IPeak with the bpl::extract function.
   *
   * @param func A pointer to a member function to wrap.
   * @return a setter function wrapped with the bpl::extract function for the
   * setter's value type
   */
  template <typename T> SetterType setterFunction(MemberFunc<T> func) {
    return [func](IPeak &peak, const object value) {
      extract<T> extractor{value};
      if (!extractor.check()) {
        throw std::runtime_error(
            "Cannot set value. Value was not of the expected type!");
      }
      (peak.*func)(extractor());
    };
  }

  /**
   * Wrap a setter function on a IPeak with the bpl::extract function.
   *
   * This is a specilization of the templated function to handle the
   * 2 parameter signature of V3D setter functions.
   *
   * @param func A pointer to a member function to wrap.
   * @return a setter function wrapped with the bpl::extract function for the
   * setter's value type
   */
  SetterType setterFunction(MemberFuncV3D func) {
    return [func](IPeak &peak, const object value) {
      extract<const V3D &> extractor{value};
      if (!extractor.check()) {
        throw std::runtime_error(
            "Cannot set value. Value was not of the expected type!");
      }
      (peak.*func)(extractor(), boost::none);
    };
  }

  // The PeaksWorkspace we need to map value to.
  IPeaksWorkspace &m_peaksWorkspace;
  // Map of string value to setter functions.
  std::unordered_map<std::string, SetterType> m_setterMap;
};

/**
 * Get the row index and column name from python types.
 *
 * @param self A reference to the PeaksWorkspace python object that we were
 * called on
 * @param col_or_row A python object containing either a row index or a column
 * name
 * @param row_or_col An integer giving the row if value is a string or the
 * column if value is an index
 */
std::pair<int, std::string> getRowAndColumnName(IPeaksWorkspace &self,
                                                const object &col_or_row,
                                                const int row_or_col) {
  extract<std::string> columnNameExtractor{col_or_row};
  std::string columnName;
  int rowIndex;

  if (columnNameExtractor.check()) {
    columnName = columnNameExtractor();
    rowIndex = row_or_col;
  } else {
    rowIndex = extract<int>(col_or_row)();
    const auto colIndex = row_or_col;
    const auto columnNames = self.getColumnNames();
    columnName = columnNames.at(colIndex);
  }

  return std::make_pair(rowIndex, columnName);
}

/**
 * Sets the value of the given cell
 * @param self A reference to the PeaksWorkspace python object that we were
 * called on
 * @param value A python object containing either a row index or a column name
 * @param row_or_col An integer giving the row if value is a string or the
 * column if value is an index
 */
void setCell(IPeaksWorkspace &self, const object &col_or_row,
             const int row_or_col, const object &value) {
  std::string columnName;
  int rowIndex;
  std::tie(rowIndex, columnName) =
      getRowAndColumnName(self, col_or_row, row_or_col);

  PeakWorkspaceTableAdaptor tableMap{self};
  tableMap.setProperty(columnName, rowIndex, value);
}
} // namespace

void export_IPeaksWorkspace() {
  // IPeaksWorkspace class
  class_<IPeaksWorkspace, bases<ITableWorkspace, ExperimentInfo>,
         boost::noncopyable>("IPeaksWorkspace", no_init)
      .def("getNumberPeaks", &IPeaksWorkspace::getNumberPeaks, arg("self"),
           "Returns the number of peaks within the workspace")
      .def("addPeak", addPeak, (arg("self"), arg("peak")),
           "Add a peak to the workspace")
      .def("removePeak", &IPeaksWorkspace::removePeak,
           (arg("self"), arg("peak_num")), "Remove a peak from the workspace")
      .def("getPeak", &IPeaksWorkspace::getPeakPtr,
           (arg("self"), arg("peak_num")), return_internal_reference<>(),
           "Returns a peak at the given index")
      .def("createPeak", createPeakQLab, (arg("self"), arg("data")),
           return_value_policy<manage_new_object>(),
           "Create a Peak and return it from its coordinates in the QLab frame")
      .def("createPeak", createPeakQLabWithDistance,
           (arg("self"), arg("data"), arg("detector_distance")),
           return_value_policy<manage_new_object>(),
           "Create a Peak and return it from its coordinates in the QLab "
           "frame, detector-sample distance explicitly provided")
      .def("createPeakHKL", createPeakHKL, (arg("self"), arg("data")),
           return_value_policy<manage_new_object>(),
           "Create a Peak and return it from its coordinates in the HKL frame")
      .def("hasIntegratedPeaks", &IPeaksWorkspace::hasIntegratedPeaks,
           arg("self"), "Determine if the peaks have been integrated")
      .def("getRun", &IPeaksWorkspace::mutableRun, arg("self"),
           return_internal_reference<>(),
           "Return the Run object for this workspace")
      .def("peakInfoNumber", &IPeaksWorkspace::peakInfoNumber,
           (arg("self"), arg("qlab_frame"), arg("lab_coordinate")),
           "Peak info number at Q vector for this workspace")
      .def("setCell", &setCell,
           (arg("self"), arg("row_or_column"), arg("column_or_row"),
            arg("value")),
           "Sets the value of a given cell. If the row_or_column argument is a "
           "number then it is interpreted as a row otherwise it "
           "is interpreted as a column name.");

  //-------------------------------------------------------------------------------------------------

  RegisterWorkspacePtrToPython<IPeaksWorkspace>();
}
