// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Column.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace_fwd.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_array.hpp>

#include <climits>
#include <memory>
#include <optional>

namespace Mantid {
namespace NeXus {
MANTID_DATAHANDLING_DLL int getNexusEntryTypes(const std::string &fileName, std::vector<std::string> &entryName,
                                               std::vector<std::string> &definition);

/** @class NexusFileIO SaveNexusProcessedHelper.h NeXus/SaveNexusProcessedHelper.h

Utility method for saving NeXus format of Mantid Workspace
This class interfaces to the C Nexus API. This is written for use by
Save and Load NexusProcessed classes, though it could be extended to
other Nexus formats. It might be replaced in future by methods using
the new Nexus C++ API.
*/
class MANTID_DATAHANDLING_DLL NexusFileIO {

public:
  // Helper typedef
  using optional_size_t = std::optional<size_t>;

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
  /// write the workspace data
  int writeNexusProcessedData2D(const API::MatrixWorkspace_const_sptr &localworkspace, const bool &uniformSpectra,
                                const bool &raggedSpectra, const std::vector<int> &indices, const char *group_name,
                                bool write2Ddata) const;

  /// write table workspace
  int writeNexusTableWorkspace(const API::ITableWorkspace_const_sptr &itableworkspace, const char *group_name) const;

  int writeNexusProcessedDataEventCombined(const DataObjects::EventWorkspace_const_sptr &ws,
                                           std::vector<int64_t> const &indices, double const *tofs,
                                           float const *weights, float const *errorSquareds, int64_t const *pulsetimes,
                                           bool compress) const;

  void NXwritedata(const char *name, NXnumtype datatype, std::vector<int> dims_array, void const *data,
                   bool compress = false) const;

  /// write bin masking information
  bool writeNexusBinMasking(const API::MatrixWorkspace_const_sptr &ws) const;

  /// Reset the pointer to the progress object.
  void resetProgress(Mantid::API::Progress *prog);

  /// Nexus file handle
  NXhandle fileID;

private:
  /// C++ API file handle
  std::shared_ptr<::NeXus::File> m_filehandle;
  /// Nexus compression method
  ::NeXus::NXcompression m_nexuscompression;
  /// Allow an externally supplied progress object to be used
  API::Progress *m_progress;
  /// Write a simple value plus possible attributes
  bool writeNxValue(const std::string &name, const std::string &value, const std::vector<std::string> &attributes,
                    const std::vector<std::string> &avalues) const;
  /// search for exisiting MantidWorkpace_n entries in opened file
  int findMantidWSEntries() const;

  /// nexus file name
  std::string m_filename;

  /// Writes given vector column to the currently open Nexus file
  template <typename VecType, typename ElemType>
  void writeNexusVectorColumn(const API::Column_const_sptr &col, const std::string &columnName, NXnumtype nexusType,
                              const std::string &interpret_as) const;

  /// Save a numeric columns of a TableWorkspace to currently open nexus file.
  template <typename ColumnT, typename NexusT>
  void writeTableColumn(NXnumtype type, const std::string &interpret_as, const API::Column &col,
                        const std::string &columnName) const;
};

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
inline bool NexusFileIO::writeNxValue(const std::string &name, const std::string &value,
                                      const std::vector<std::string> &attributes,
                                      const std::vector<std::string> &avalues) const {
  try {
    m_filehandle->writeData(name, value);

    // open it again to add attributes
    m_filehandle->openData(name);
    for (unsigned int it = 0; it < attributes.size(); ++it) {
      m_filehandle->putAttr(attributes[it], avalues[it]);
    }
    m_filehandle->closeData();
  } catch (::NeXus::Exception &) {
    return false;
  }

  return true;
}

/// Helper typedef for a shared pointer of a NexusFileIO.
using NexusFileIO_sptr = std::shared_ptr<NexusFileIO>;

} // namespace NeXus
} // namespace Mantid
