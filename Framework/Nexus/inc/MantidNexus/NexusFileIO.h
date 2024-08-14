// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/VectorColumn.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexus/DllConfig.h"
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_array.hpp>

#include <boost/optional.hpp>
#include <climits>
#include <memory>
#include <nexus/NeXusFile.hpp>

namespace Mantid {
namespace NeXus {
MANTID_NEXUS_DLL int getNexusEntryTypes(const std::string &fileName, std::vector<std::string> &entryName,
                                        std::vector<std::string> &definition);

/** @class NexusFileIO NexusFileIO.h NeXus/NexusFileIO.h

Utility method for saving NeXus format of Mantid Workspace
This class interfaces to the C Nexus API. This is written for use by
Save and Load NexusProcessed classes, though it could be extended to
other Nexus formats. It might be replaced in future by methods using
the new Nexus C++ API.
*/
class MANTID_NEXUS_DLL NexusFileIO {

public:
  // Helper typedef
  using optional_size_t = boost::optional<size_t>;

  /// Default constructor
  NexusFileIO();

  /// Contructor with Progress supplied
  NexusFileIO(API::Progress *prog);

  /// Destructor
  ~NexusFileIO();

  /// open the nexus file for writing
  void openNexusWrite(const std::string &fileName, optional_size_t entryNumber = optional_size_t(),
                      const bool append_to_file = true);
  /// write the header ifon for the Mantid workspace format
  int writeNexusProcessedHeader(const std::string &title, const std::string &wsName = "") const;
  /// close the nexus file
  void closeNexusFile();
  /// Close the group.
  void closeGroup();
  /// Write a lgos section
  int writeNexusSampleLogs(const Mantid::API::Run &runProperties) const;
  /// write the workspace data
  int writeNexusProcessedData2D(const API::MatrixWorkspace_const_sptr &localworkspace, const bool &uniformSpectra,
                                const bool &raggedSpectra, const std::vector<int> &indices, const char *group_name,
                                bool write2Ddata) const;

  /// write table workspace
  int writeNexusTableWorkspace(const API::ITableWorkspace_const_sptr &itableworkspace, const char *group_name) const;

  int writeNexusProcessedDataEvent(const DataObjects::EventWorkspace_const_sptr &ws);

  int writeNexusProcessedDataEventCombined(const DataObjects::EventWorkspace_const_sptr &ws,
                                           std::vector<int64_t> &indices, double *tofs, float *weights,
                                           float *errorSquareds, int64_t *pulsetimes, bool compress) const;

  int writeEventList(const DataObjects::EventList &el, const std::string &group_name) const;

  template <class T>
  void writeEventListData(std::vector<T> events, bool writeTOF, bool writePulsetime, bool writeWeight,
                          bool writeError) const;
  void NXwritedata(const char *name, int datatype, int rank, int *dims_array, void *data, bool compress = false) const;

  /// find size of open entry data section
  int getWorkspaceSize(int &numberOfSpectra, int &numberOfChannels, int &numberOfXpoints, bool &uniformBounds,
                       std::string &axesUnits, std::string &yUnits) const;
  /// read X values for one (or the generic if uniform) spectra
  int getXValues(MantidVec &xValues, const int &spectra) const;
  /// read values and errors for spectra
  int getSpectra(MantidVec &values, MantidVec &errors, const int &spectra) const;

  /// write bin masking information
  bool writeNexusBinMasking(const API::MatrixWorkspace_const_sptr &ws) const;

  /// Reset the pointer to the progress object.
  void resetProgress(Mantid::API::Progress *prog);

  /// Nexus file handle
  NXhandle fileID;

private:
  /// C++ API file handle
  // clang-format off
  std::shared_ptr< ::NeXus::File> m_filehandle;
  // clang-format on
  /// Nexus compression method
  int m_nexuscompression;
  /// Allow an externally supplied progress object to be used
  API::Progress *m_progress;
  /// Write a simple value plus possible attributes
  template <class TYPE>
  bool writeNxValue(const std::string &name, const TYPE &value, const int nxType,
                    const std::vector<std::string> &attributes, const std::vector<std::string> &avalues) const;
  /// Returns true if the given property is a time series property
  bool isTimeSeries(Kernel::Property *prop) const;
  /// Write a time series log entry
  bool writeTimeSeriesLog(Kernel::Property *prop) const;
  /// Write a single value log entry
  bool writeSingleValueLog(Kernel::Property *prop) const;
  /// Writes a numeric log to the Nexus file
  template <class T> void writeNumericTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const;
  /// Write a single valued NXLog entry to the Nexus file
  template <class TYPE>
  bool writeSingleValueNXLog(const std::string &name, const TYPE &value, const int nxType,
                             const std::vector<std::string> &attributes, const std::vector<std::string> &avalues) const;
  /// write an NXnote with standard fields (but NX_CHAR rather than NX_BINARY
  /// data)
  bool writeNxNote(const std::string &noteName, const std::string &author, const std::string &date,
                   const std::string &description, const std::string &pairValues) const;
  /// write a float array along with any defined attributes
  void writeNxFloatArray(const std::string &name, const std::vector<double> &values,
                         const std::vector<std::string> &attributes, const std::vector<std::string> &avalues) const;
  /// write a char array along with any defined attributes
  bool writeNxStringArray(const std::string &name, const std::vector<std::string> &values,
                          const std::vector<std::string> &attributes, const std::vector<std::string> &avalues) const;
  /// Write NXlog data for given string TimeSeriesProperty
  void writeNumericTimeLog_String(const Kernel::TimeSeriesProperty<std::string> *s_timeSeries) const;
  /// check if the gievn item exists in the current level
  bool checkEntryAtLevel(const std::string &item) const;
  /// check if given attribute name is in currently opened entry
  bool checkAttributeName(const std::string &target) const;
  /// Look for entry with given attribute (eg "signal")
  bool checkEntryAtLevelByAttribute(const std::string &attribute, std::string &entry) const;
  /// search for exisiting MantidWorkpace_n entries in opened file
  int findMantidWSEntries() const;
  /// convert posix time to time_t
  std::time_t to_time_t(const boost::posix_time::ptime &t) ///< convert posix time to time_t
  {
    /**
    Take the input Posix time, subtract the unix epoch, and return the seconds
    as a std::time_t value.
    @param t :: time of interest as ptime
    @return :: time_t value of t
    */
    if (t == boost::posix_time::neg_infin)
      return 0;
    else if (t == boost::posix_time::pos_infin)
      return LONG_MAX;
    boost::posix_time::ptime start(boost::gregorian::date(1970, 1, 1));
    return (t - start).total_seconds();
  }

  /// nexus file name
  std::string m_filename;

  /** Writes a numeric log to the Nexus file
   *  @tparam T A numeric type (double, int, bool)
   *  @param timeSeries :: A pointer to the log property
   */
  template <class T> void writeNexusTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const;

  /// Return the log value type as string
  template <class T> std::string logValueType() const { return "unknown"; }

  /// Writes given vector column to the currently open Nexus file
  template <typename VecType, typename ElemType>
  void writeNexusVectorColumn(const API::Column_const_sptr &col, const std::string &columnName, int nexusType,
                              const std::string &interpret_as) const;

  /// Save a numeric columns of a TableWorkspace to currently open nexus file.
  template <typename ColumnT, typename NexusT>
  void writeTableColumn(int type, const std::string &interpret_as, const API::Column &col,
                        const std::string &columnName) const;
};

/**
 * Write a single valued entry to the Nexus file
 * @param name :: The name of the entry
 * @param value :: The value of the entry
 * @param nxType :: The nxType of the entry
 * @param attributes :: A list of attributes 1:1 mapped to their values in the
 * <code>avalues</code> argument
 * @param avalues :: A list of attribute values in the same order as the
 * <code>attributes</code> argument
 * @returns A boolean indicating success or failure
 */
template <class TYPE>
bool NexusFileIO::writeNxValue(const std::string &name, const TYPE &value, const int nxType,
                               const std::vector<std::string> &attributes,
                               const std::vector<std::string> &avalues) const {
  int dimensions[1] = {1};
  if (NXmakedata(fileID, name.c_str(), nxType, 1, dimensions) == NX_ERROR)
    return false;
  if (NXopendata(fileID, name.c_str()) == NX_ERROR)
    return false;
  for (unsigned int it = 0; it < attributes.size(); ++it) {
    NXputattr(fileID, attributes[it].c_str(), (void *)avalues[it].c_str(), static_cast<int>(avalues[it].size() + 1),
              NX_CHAR);
  }
  NXputdata(fileID, (void *)&value);
  NXclosedata(fileID);
  return true;
}

/**
 * Write a single valued entry to the Nexus file (specialization for a string)
 * @param name :: The name of the entry
 * @param value :: The value of the entry
 * @param nxType :: The nxType of the entry
 * @param attributes :: A list of attributes 1:1 mapped to their values in the
 * <code>avalues</code> argument
 * @param avalues :: A list of attribute values in the same order as the
 * <code>attributes</code> argument
 * @returns A boolean indicating success or failure
 */
template <>
inline bool NexusFileIO::writeNxValue(const std::string &name, const std::string &value, const int nxType,
                                      const std::vector<std::string> &attributes,
                                      const std::vector<std::string> &avalues) const {
  (void)nxType;
  int dimensions[1] = {0};
  std::string nxstr = value;
  if (nxstr.empty())
    nxstr += " ";
  dimensions[0] = static_cast<int>(nxstr.size() + 1);
  if (NXmakedata(fileID, name.c_str(), nxType, 1, dimensions) == NX_ERROR)
    return false;
  if (NXopendata(fileID, name.c_str()) == NX_ERROR)
    return false;
  for (unsigned int it = 0; it < attributes.size(); ++it) {
    NXputattr(fileID, attributes[it].c_str(), avalues[it].c_str(), static_cast<int>(avalues[it].size() + 1), NX_CHAR);
  }
  NXputdata(fileID, reinterpret_cast<void *>(const_cast<char *>(nxstr.c_str())));
  NXclosedata(fileID);
  return true;
}

/**
 * Write a single valued NXLog entry to the Nexus file
 * @param name :: The name of the entry
 * @param value :: The value of the entry
 * @param nxType :: The nxType of the entry
 * @param attributes :: A list of attributes 1:1 mapped to their values in the
 * <code>avalues</code> argument
 * @param avalues :: A list of attribute values in the same order as the
 * <code>attributes</code> argument
 * @returns A boolean indicating success or failure
 */
template <class TYPE>
bool NexusFileIO::writeSingleValueNXLog(const std::string &name, const TYPE &value, const int nxType,
                                        const std::vector<std::string> &attributes,
                                        const std::vector<std::string> &avalues) const {
  if (NXmakegroup(fileID, name.c_str(), "NXlog") == NX_ERROR)
    return false;
  NXopengroup(fileID, name.c_str(), "NXlog");
  int dimensions[1] = {1};
  if (NXmakedata(fileID, "value", nxType, 1, dimensions) == NX_ERROR)
    return false;
  if (NXopendata(fileID, "value") == NX_ERROR)
    return false;
  for (unsigned int it = 0; it < attributes.size(); ++it) {
    NXputattr(fileID, attributes[it].c_str(), (void *)avalues[it].c_str(), static_cast<int>(avalues[it].size() + 1),
              NX_CHAR);
  }
  NXputdata(fileID, (void *)&value);
  NXclosedata(fileID);
  NXclosegroup(fileID);
  return true;
}

/**
 * Write a single valued NXLog entry to the Nexus file (specialization for a
 * string)
 * @param name :: The name of the entry
 * @param value :: The value of the entry
 * @param nxType :: The nxType of the entry
 * @param attributes :: A list of attributes 1:1 mapped to their values in the
 * <code>avalues</code> argument
 * @param avalues :: A list of attribute values in the same order as the
 * <code>attributes</code> argument
 * @returns A boolean indicating success or failure
 */
template <>
inline bool NexusFileIO::writeSingleValueNXLog(const std::string &name, const std::string &value, const int nxType,
                                               const std::vector<std::string> &attributes,
                                               const std::vector<std::string> &avalues) const {
  (void)nxType;
  if (NXmakegroup(fileID, name.c_str(), "NXlog") == NX_ERROR)
    return false;
  NXopengroup(fileID, name.c_str(), "NXlog");
  int dimensions[1] = {0};
  std::string nxstr = value;
  if (nxstr.empty())
    nxstr += " ";
  dimensions[0] = static_cast<int>(nxstr.size() + 1); // Allow for null-terminator
  if (NXmakedata(fileID, "value", NX_CHAR, 1, dimensions) == NX_ERROR)
    return false;
  if (NXopendata(fileID, "value") == NX_ERROR)
    return false;
  for (unsigned int it = 0; it < attributes.size(); ++it) {
    NXputattr(fileID, attributes[it].c_str(), reinterpret_cast<void *>(const_cast<char *>(avalues[it].c_str())),
              static_cast<int>(avalues[it].size() + 1), NX_CHAR);
  }
  NXputdata(fileID, reinterpret_cast<void *>(const_cast<char *>(nxstr.c_str())));
  NXclosedata(fileID);
  NXclosegroup(fileID);
  return true;
}

/** Writes a numeric log to the Nexus file
 *  @tparam T A numeric type (double, int, bool)
 *  @param timeSeries :: A pointer to the log property
 */
template <class T> void NexusFileIO::writeNumericTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const {
  // write NXlog section for double values
  NXstatus status;
  // get a name for the log, possibly removing the path component
  std::string logName = timeSeries->name();
  size_t ipos = logName.find_last_of("/\\");
  if (ipos != std::string::npos)
    logName = logName.substr(ipos + 1);
  // extract values from timeseries
  std::map<Types::Core::DateAndTime, T> dV = timeSeries->valueAsMap();
  std::vector<double> values;
  std::vector<double> times;
  Types::Core::DateAndTime t0;
  bool first = true;
  for (typename std::map<Types::Core::DateAndTime, T>::const_iterator dv = dV.begin(); dv != dV.end(); ++dv) {
    T val = dv->second;
    Types::Core::DateAndTime time = dv->first;
    values.emplace_back(val);
    if (first) {
      t0 = time; // start time of log
      first = false;
    }
    times.emplace_back(Types::Core::DateAndTime::secondsFromDuration(time - t0));
  }
  // create log
  status = NXmakegroup(fileID, logName.c_str(), "NXlog");
  if (status == NX_ERROR)
    return;

  NXopengroup(fileID, logName.c_str(), "NXlog");
  // write log data
  std::vector<std::string> attributes, avalues;
  attributes.emplace_back("type");
  avalues.emplace_back(logValueType<T>());
  writeNxFloatArray("value", values, attributes, avalues);
  attributes.clear();
  avalues.clear();
  // get ISO time, and save it as an attribute
  attributes.emplace_back("start");
  avalues.emplace_back(t0.toISO8601String());

  writeNxFloatArray("time", times, attributes, avalues);
  NXclosegroup(fileID);
}

/// Helper typedef for a shared pointer of a NexusFileIO.
using NexusFileIO_sptr = std::shared_ptr<NexusFileIO>;

} // namespace NeXus
} // namespace Mantid
