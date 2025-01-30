// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// NexusFileIO
// @author Ronald Fowler
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <io.h>
// Define the MAX_NAME macro for Windows
// Maximum base file name size on modern windows systems is 260 characters
#define NAME_MAX 260
#endif /* _WIN32 */
#include "MantidAPI/NumericAxis.h"
#include "MantidDataHandling/SaveNexusProcessedHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexusCpp/NeXusException.hpp"

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid::NeXus {
using namespace Kernel;
using namespace API;
using namespace DataObjects;

namespace {
/// static logger
Logger g_log("NexusFileIO");
const std::string NULL_STR("NULL");
} // namespace

/// Empty default constructor
NexusFileIO::NexusFileIO()
    : fileID(), m_filehandle(), m_nexuscompression(NX_COMP_LZW), m_progress(nullptr), m_filename() {}

/// Constructor that supplies a progress object
NexusFileIO::NexusFileIO(Progress *prog)
    : fileID(), m_filehandle(), m_nexuscompression(NX_COMP_LZW), m_progress(prog), m_filename() {}

void NexusFileIO::resetProgress(Progress *prog) { m_progress = prog; }

//
// Write out the data in a worksvn space in Nexus "Processed" format.
// This *Proposed* standard comprises the fields:
// <NXentry name="{Name of entry}">
//   <title>
//     {Extended title for entry}
//   </title>
//   <definition
//   URL="http://www.nexusformat.org/instruments/xml/NXprocessed.xml"
//       version="1.0">
//     NXprocessed
//   </definition>
//   <NXsample name="{Name of sample}">?
//     {Any relevant sample information necessary to define the data.}
//   </NXsample>
//   <NXdata name="{Name of processed data}">
//     <values signal="1" type="NX_FLOAT[:,:]" axes="axis1:axis2">{Processed
//     values}</values>
//     <axis1 type="NX_FLOAT[:]">{Values of the first dimension's axis}</axis1>
//     <axis2 type="NX_FLOAT[:]">{Values of the second dimension's axis}</axis2>
//   </NXdata>
//   <NXprocess name="{Name of process}">?
//     {Any relevant information about the steps used to process the data.}
//   </NXprocess>
// </NXentry>

void NexusFileIO::openNexusWrite(const std::string &fileName, NexusFileIO::optional_size_t entryNumber,
                                 const bool append_to_file) {
  // open named file and entry - file may exist
  // @throw Exception::FileError if cannot open Nexus file for writing
  //
  NXaccess mode(NXACC_CREATE5);
  std::string mantidEntryName;
  m_filename = fileName;
  //
  // If file to write exists, then open as is else see if the extension is xml,
  // if so open as xml
  // format otherwise as compressed hdf5
  //
  if ((Poco::File(m_filename).exists() && append_to_file) || m_filehandle)
    mode = NXACC_RDWR;

  else {
    if (fileName.find(".xml") < fileName.size() || fileName.find(".XML") < fileName.size()) {
      mode = NXACC_CREATEXML;
      m_nexuscompression = NX_COMP_NONE;
    }
    mantidEntryName = "mantid_workspace_1";
  }

  /*Only create the file handle if needed.*/
  if (!m_filehandle) {
    // The nexus or HDF5 libraries crash when the filename is greater than 255
    // on OSX and Ubuntu.
    Poco::Path path(fileName);
    std::string baseName = path.getBaseName();
    if (baseName.size() > NAME_MAX) {
      std::string message = "Filename is too long. Unable to open file: ";
      throw Exception::FileError(message, fileName);
    }

    // open the file and copy the handle into the NeXus::File object
    NXstatus status = NXopen(fileName.c_str(), mode, &fileID);
    if (status == NXstatus::NX_ERROR) {
      g_log.error("Unable to open file " + fileName);
      throw Exception::FileError("Unable to open File:", fileName);
    }

    auto file = new ::NeXus::File(fileID, true);

    m_filehandle = std::shared_ptr<::NeXus::File>(file);
  }

  //
  // for existing files, search for any current mantid_workspace_<n> entries and
  // set the
  // new name to be n+1 so that we do not over-write by default. This may need
  // changing.
  //
  if (mode == NXACC_RDWR) {
    size_t count = 0;
    if (entryNumber.has_value()) {
      // Use the entry number provided.
      count = entryNumber.value();
    } else {
      // Have to figure it our ourselves. Requires opening the exisitng file to
      // get the information via a search.
      count = findMantidWSEntries();
    }

    std::stringstream suffix;
    suffix << (count + 1);
    mantidEntryName = "mantid_workspace_" + suffix.str();
  }
  //
  // make and open the new mantid_workspace_<n> group
  // file remains open until explicit close
  //
  const std::string className = "NXentry";

  m_filehandle->makeGroup(mantidEntryName, className);
  m_filehandle->openGroup(mantidEntryName, className);
}

void NexusFileIO::closeGroup() { m_filehandle->closeGroup(); }

//-----------------------------------------------------------------------------------------------
void NexusFileIO::closeNexusFile() {
  if (m_filehandle) {
    m_filehandle.reset();
  }
}

//-----------------------------------------------------------------------------------------------
/**  Write Nexus mantid workspace header fields for the
 NXentry/IXmantid/NXprocessed field.
 The URLs are not correct as they do not exist presently, but follow the format
 for other Nexus specs.
 @param title :: title field.
 @param wsName :: workspace name.
 @return zero on success, a non-zero value on failure.
 */
int NexusFileIO::writeNexusProcessedHeader(const std::string &title, const std::string &wsName) const {

  const std::string className = "Mantid Processed Workspace";
  std::vector<std::string> attributes, avalues;
  attributes.reserve(2);
  avalues.reserve(2);
  if (!writeNxValue("title", title, attributes, avalues))
    return (3);

  // name for workspace if this is a multi workspace nexus file
  if (!wsName.empty()) {
    if (!writeNxValue("workspace_name", wsName, attributes, avalues))
      return (3);
  }

  attributes.emplace_back("URL");
  avalues.emplace_back("http://www.nexusformat.org/instruments/xml/NXprocessed.xml");
  attributes.emplace_back("Version");
  avalues.emplace_back("1.0");
  // this may not be the "correct" long term path, but it is valid at present
  if (!writeNxValue("definition", className, attributes, avalues))
    return (3);
  avalues.clear();
  avalues.emplace_back("http://www.isis.rl.ac.uk/xml/IXmantid.xml");
  avalues.emplace_back("1.0");
  if (!writeNxValue("definition_local", className, attributes, avalues))
    return (3);
  return (0);
}

//-------------------------------------------------------------------------------------
/** Write out a MatrixWorkspace's data as a 2D matrix.
 * Use writeNexusProcessedDataEvent if writing an EventWorkspace.
 */
int NexusFileIO::writeNexusProcessedData2D(const API::MatrixWorkspace_const_sptr &localworkspace,
                                           const bool &uniformSpectra, const bool &raggedSpectra,
                                           const std::vector<int> &indices, const char *group_name,
                                           bool write2Ddata) const {
  NXstatus status;

  // write data entry
  status = NXmakegroup(fileID, group_name, "NXdata");
  if (status == NXstatus::NX_ERROR)
    return (2);
  NXopengroup(fileID, group_name, "NXdata");
  // write workspace data
  const size_t nHist = localworkspace->getNumberHistograms();
  if (nHist < 1)
    return (2);
  const size_t nSpect = indices.size();
  size_t nSpectBins = localworkspace->y(0).size();
  if (raggedSpectra)
    for (size_t i = 0; i < nSpect; i++)
      nSpectBins = std::max(nSpectBins, localworkspace->y(indices[i]).size());
  int dims_array[2] = {static_cast<int>(nSpect), static_cast<int>(nSpectBins)};

  // Set the axis labels and values
  Mantid::API::Axis *xAxis = localworkspace->getAxis(0);
  Mantid::API::Axis *sAxis = localworkspace->getAxis(1);
  std::string xLabel, sLabel;
  if (xAxis->isSpectra())
    xLabel = "spectraNumber";
  else {
    if (xAxis->unit())
      xLabel = xAxis->unit()->unitID();
    else
      xLabel = "unknown";
  }
  if (sAxis->isSpectra())
    sLabel = "spectraNumber";
  else {
    if (sAxis->unit())
      sLabel = sAxis->unit()->unitID();
    else
      sLabel = "unknown";
  }
  // Get the values on the vertical axis
  std::vector<double> axis2;
  if (nSpect < nHist)
    for (size_t i = 0; i < nSpect; i++)
      axis2.emplace_back((*sAxis)(indices[i]));
  else
    for (size_t i = 0; i < sAxis->length(); i++)
      axis2.emplace_back((*sAxis)(i));

  int start[2] = {0, 0};
  int asize[2] = {1, dims_array[1]};

  // -------------- Actually write the 2D data ----------------------------
  if (write2Ddata) {
    std::string name = "values";
    NXcompmakedata(fileID, name.c_str(), NXnumtype::FLOAT64, 2, dims_array, m_nexuscompression, asize);
    NXopendata(fileID, name.c_str());
    for (size_t i = 0; i < nSpect; i++) {
      NXputslab(fileID, localworkspace->y(indices[i]).rawData().data(), start, asize);
      start[0]++;
    }
    if (m_progress != nullptr)
      m_progress->reportIncrement(1, "Writing data");
    int signal = 1;
    NXputattr(fileID, "signal", &signal, 1, NXnumtype::INT32);
    // More properties
    const std::string axesNames = "axis2,axis1";
    NXputattr(fileID, "axes", axesNames.c_str(), static_cast<int>(axesNames.size()), NXnumtype::CHAR);
    std::string yUnits = localworkspace->YUnit();
    std::string yUnitLabel = localworkspace->YUnitLabel();
    NXputattr(fileID, "units", yUnits.c_str(), static_cast<int>(yUnits.size()), NXnumtype::CHAR);
    NXputattr(fileID, "unit_label", yUnitLabel.c_str(), static_cast<int>(yUnitLabel.size()), NXnumtype::CHAR);
    NXclosedata(fileID);

    // error
    name = "errors";
    NXcompmakedata(fileID, name.c_str(), NXnumtype::FLOAT64, 2, dims_array, m_nexuscompression, asize);
    NXopendata(fileID, name.c_str());
    start[0] = 0;
    for (size_t i = 0; i < nSpect; i++) {
      NXputslab(fileID, localworkspace->e(indices[i]).rawData().data(), start, asize);
      start[0]++;
    }

    if (m_progress != nullptr)
      m_progress->reportIncrement(1, "Writing data");

    // Fractional area for RebinnedOutput
    if (localworkspace->id() == "RebinnedOutput") {
      RebinnedOutput_const_sptr rebin_workspace = std::dynamic_pointer_cast<const RebinnedOutput>(localworkspace);
      name = "frac_area";
      NXcompmakedata(fileID, name.c_str(), NXnumtype::FLOAT64, 2, dims_array, m_nexuscompression, asize);
      NXopendata(fileID, name.c_str());
      start[0] = 0;
      for (size_t i = 0; i < nSpect; i++) {
        NXputslab(fileID, rebin_workspace->readF(indices[i]).data(), start, asize);
        start[0]++;
      }

      std::string finalized = (rebin_workspace->isFinalized()) ? "1" : "0";
      NXputattr(fileID, "finalized", finalized.c_str(), 2, NXnumtype::CHAR);
      std::string sqrdErrs = (rebin_workspace->hasSqrdErrors()) ? "1" : "0";
      NXputattr(fileID, "sqrd_errors", sqrdErrs.c_str(), 2, NXnumtype::CHAR);

      if (m_progress != nullptr)
        m_progress->reportIncrement(1, "Writing data");
    }

    // Potentially x error
    if (localworkspace->hasDx(0)) {
      dims_array[0] = static_cast<int>(nSpect);
      dims_array[1] = static_cast<int>(localworkspace->dx(0).size());
      std::string dxErrorName = "xerrors";
      NXcompmakedata(fileID, dxErrorName.c_str(), NXnumtype::FLOAT64, 2, dims_array, m_nexuscompression, asize);
      NXopendata(fileID, dxErrorName.c_str());
      start[0] = 0;
      asize[1] = dims_array[1];
      for (size_t i = 0; i < nSpect; i++) {

        NXputslab(fileID, localworkspace->dx(indices[i]).rawData().data(), start, asize);
        start[0]++;
      }
    }

    NXclosedata(fileID);
  }

  // write X data, as single array or all values if "ragged"
  if (raggedSpectra) {
    size_t max_x_size{0};
    for (size_t i = 0; i < nSpect; i++)
      max_x_size = std::max(max_x_size, localworkspace->x(indices[i]).size());
    dims_array[0] = static_cast<int>(nSpect);
    dims_array[1] = static_cast<int>(max_x_size);
    NXmakedata(fileID, "axis1", NXnumtype::FLOAT64, 2, dims_array);
    NXopendata(fileID, "axis1");
    start[0] = 0;

    // create vector of NaNs to fill invalid space at end of ragged array
    std::vector<double> nans(max_x_size, std::numeric_limits<double>::quiet_NaN());

    for (size_t i = 0; i < nSpect; i++) {
      size_t nBins = localworkspace->x(indices[i]).size();
      asize[1] = static_cast<int>(nBins);
      NXputslab(fileID, localworkspace->x(indices[i]).rawData().data(), start, asize);
      if (nBins < max_x_size) {
        start[1] = asize[1];
        asize[1] = static_cast<int>(max_x_size - nBins);
        NXputslab(fileID, nans.data(), start, asize);
        start[1] = 0;
      }
      start[0]++;
    }
  } else if (uniformSpectra) {
    dims_array[0] = static_cast<int>(localworkspace->x(0).size());
    NXmakedata(fileID, "axis1", NXnumtype::FLOAT64, 1, dims_array);
    NXopendata(fileID, "axis1");
    NXputdata(fileID, localworkspace->x(0).rawData().data());

  } else {
    dims_array[0] = static_cast<int>(nSpect);
    dims_array[1] = static_cast<int>(localworkspace->x(0).size());
    NXmakedata(fileID, "axis1", NXnumtype::FLOAT64, 2, dims_array);
    NXopendata(fileID, "axis1");
    start[0] = 0;
    asize[1] = dims_array[1];
    for (size_t i = 0; i < nSpect; i++) {
      NXputslab(fileID, localworkspace->x(indices[i]).rawData().data(), start, asize);
      start[0]++;
    }
  }

  std::string dist = (localworkspace->isDistribution()) ? "1" : "0";
  NXputattr(fileID, "distribution", dist.c_str(), 2, NXnumtype::CHAR);
  NXputattr(fileID, "units", xLabel.c_str(), static_cast<int>(xLabel.size()), NXnumtype::CHAR);

  auto label = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(xAxis->unit());
  if (label) {
    NXputattr(fileID, "caption", label->caption().c_str(), static_cast<int>(label->caption().size()), NXnumtype::CHAR);
    auto unitLbl = label->label();
    NXputattr(fileID, "label", unitLbl.ascii().c_str(), static_cast<int>(unitLbl.ascii().size()), NXnumtype::CHAR);
  }

  NXclosedata(fileID);

  if (!sAxis->isText()) {
    // write axis2, maybe just spectra number
    dims_array[0] = static_cast<int>(axis2.size());
    NXmakedata(fileID, "axis2", NXnumtype::FLOAT64, 1, dims_array);
    NXopendata(fileID, "axis2");
    NXputdata(fileID, axis2.data());
    NXputattr(fileID, "units", sLabel.c_str(), static_cast<int>(sLabel.size()), NXnumtype::CHAR);

    auto unitLabel = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(sAxis->unit());
    if (unitLabel) {
      NXputattr(fileID, "caption", unitLabel->caption().c_str(), static_cast<int>(unitLabel->caption().size()),
                NXnumtype::CHAR);
      auto unitLbl = unitLabel->label();
      NXputattr(fileID, "label", unitLbl.ascii().c_str(), static_cast<int>(unitLbl.ascii().size()), NXnumtype::CHAR);
    }

    NXclosedata(fileID);
  } else {
    std::string textAxis;
    for (size_t i = 0; i < sAxis->length(); i++) {
      textAxis += sAxis->label(i) + "\n";
    }
    dims_array[0] = static_cast<int>(textAxis.size());
    NXmakedata(fileID, "axis2", NXnumtype::CHAR, 1, dims_array);
    NXopendata(fileID, "axis2");
    NXputdata(fileID, textAxis.c_str());
    NXputattr(fileID, "units", "TextAxis", 8, NXnumtype::CHAR);

    auto unitLabel = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(sAxis->unit());
    if (unitLabel) {
      NXputattr(fileID, "caption", unitLabel->caption().c_str(), static_cast<int>(unitLabel->caption().size()),
                NXnumtype::CHAR);
      auto unitLbl = unitLabel->label();
      NXputattr(fileID, "label", unitLbl.ascii().c_str(), static_cast<int>(unitLbl.ascii().size()), NXnumtype::CHAR);
    }

    NXclosedata(fileID);
  }

  writeNexusBinMasking(localworkspace);

  status = NXclosegroup(fileID);
  return ((status == NXstatus::NX_ERROR) ? 3 : 0);
}

//-------------------------------------------------------------------------------------
/**
 * Save a numeric columns of a TableWorkspace to currently open nexus file.
 * @param type :: Nexus code for the element data type.
 * @param interpret_as :: Value of the interpret_as attribute.
 * @param col :: Reference to the column being svaed.
 * @param columnName :: Name of the nexus data set in which the column values
 * are saved.
 */
template <typename ColumnT, typename NexusT>
void NexusFileIO::writeTableColumn(NXnumtype type, const std::string &interpret_as, const API::Column &col,
                                   const std::string &columnName) const {
  const auto nRows = static_cast<int>(col.size());
  int dims_array[1] = {nRows};

  auto toNexus = new NexusT[nRows];
  for (int ii = 0; ii < nRows; ii++)
    toNexus[ii] = static_cast<NexusT>(col.cell<ColumnT>(ii));
  NXwritedata(columnName.c_str(), type, 1, dims_array, toNexus, false);
  delete[] toNexus;

  // attributes
  NXopendata(fileID, columnName.c_str());
  std::string units = "Not known";
  NXputattr(fileID, "units", units.c_str(), static_cast<int>(units.size()), NXnumtype::CHAR);
  NXputattr(fileID, "interpret_as", interpret_as.c_str(), static_cast<int>(interpret_as.size()), NXnumtype::CHAR);
  NXclosedata(fileID);
}

// Helper templated definitions
namespace {

// Get a size of a vector to be used in writeNexusVectorColumn(...)
template <typename VecType> size_t getSizeOf(const VecType &vec) { return vec.size(); }

// Special case of V3D
size_t getSizeOf(const Kernel::V3D & /*unused*/) { return 3; }
} // namespace

/**
 * Writes given vector column to the currently open Nexus file.
 * @param col     :: Column to write
 * @param columnName :: Name of NXdata to write to
 * @param nexusType  :: Nexus type to use to store data
 * @param interpret_as   :: Name of the type to use for "interpret_as" attribute
 */
template <typename VecType, typename ElemType>
void NexusFileIO::writeNexusVectorColumn(const Column_const_sptr &col, const std::string &columnName,
                                         NXnumtype nexusType, const std::string &interpret_as) const {
  ConstColumnVector<VecType> column(col);
  size_t rowCount = column.size();

  // Search for the longest array amongst the cells
  size_t maxSize(0);
  for (size_t i = 0; i < rowCount; ++i) {
    size_t size = getSizeOf(column[i]);

    if (size > maxSize)
      maxSize = size;
  }

  // Set-up dimensions
  int dims[2]{static_cast<int>(rowCount), static_cast<int>(maxSize)};

  // Create data array
  boost::scoped_array<ElemType> data(new ElemType[rowCount * maxSize]);

  for (size_t i = 0; i < rowCount; ++i) {
    // copy data in a cell to a vector with the same element type
    std::vector<ElemType> values = column[i];

    // So that all the arrays are of the size maxSize
    values.resize(maxSize);

    // Copy values to the data array
    for (size_t j = 0; j < maxSize; ++j)
      data[i * maxSize + j] = values[j];
  }

  // Write data
  NXwritedata(columnName.c_str(), nexusType, 2, dims, data.get(), false);

  NXopendata(fileID, columnName.c_str());

  // Add sizes of rows as attributes. We can't use padding zeroes to determine
  // that because the
  // vector stored might end with zeroes as well.
  for (size_t i = 0; i < rowCount; ++i) {
    auto size = static_cast<int>(getSizeOf(column[i]));

    std::ostringstream rowSizeAttrName;
    rowSizeAttrName << "row_size_" << i;

    NXputattr(fileID, rowSizeAttrName.str().c_str(), &size, 1, NXnumtype::INT32);
  }

  std::string units = "Not known";

  // Write general attributes
  NXputattr(fileID, "units", units.c_str(), static_cast<int>(units.size()), NXnumtype::CHAR);
  NXputattr(fileID, "interpret_as", interpret_as.c_str(), static_cast<int>(interpret_as.size()), NXnumtype::CHAR);

  NXclosedata(fileID);
}
//-------------------------------------------------------------------------------------
/** Write out a table Workspace's
 */
int NexusFileIO::writeNexusTableWorkspace(const API::ITableWorkspace_const_sptr &itableworkspace,
                                          const char *group_name) const {
  NXstatus status = NXstatus::NX_ERROR;

  std::shared_ptr<const TableWorkspace> tableworkspace =
      std::dynamic_pointer_cast<const TableWorkspace>(itableworkspace);
  std::shared_ptr<const PeaksWorkspace> peakworkspace =
      std::dynamic_pointer_cast<const PeaksWorkspace>(itableworkspace);

  if (!tableworkspace && !peakworkspace)
    return 3;

  // write data entry
  status = NXmakegroup(fileID, group_name, "NXdata");
  if (status == NXstatus::NX_ERROR)
    return (2);
  NXopengroup(fileID, group_name, "NXdata");

  auto nRows = static_cast<int>(itableworkspace->rowCount());

  for (size_t i = 0; i < itableworkspace->columnCount(); i++) {
    Column_const_sptr col = itableworkspace->getColumn(i);

    std::string str = "column_" + std::to_string(i + 1);

    if (col->isType<double>()) {
      writeTableColumn<double, double>(NXnumtype::FLOAT64, "", *col, str);
    } else if (col->isType<float>()) {
      writeTableColumn<float, float>(NXnumtype::FLOAT32, "", *col, str);
    } else if (col->isType<int>()) {
      writeTableColumn<int, int32_t>(NXnumtype::INT32, "", *col, str);
    } else if (col->isType<uint32_t>()) {
      writeTableColumn<uint32_t, uint32_t>(NXnumtype::UINT32, "", *col, str);
    } else if (col->isType<int64_t>()) {
      writeTableColumn<int64_t, int64_t>(NXnumtype::INT64, "", *col, str);
    } else if (col->isType<size_t>()) {
      writeTableColumn<size_t, uint64_t>(NXnumtype::UINT64, "", *col, str);
    } else if (col->isType<Boolean>()) {
      writeTableColumn<bool, bool>(NXnumtype::UINT8, "", *col, str);
    } else if (col->isType<std::string>()) {
      // determine max string size
      size_t maxStr = 0;
      for (int ii = 0; ii < nRows; ii++) {
        if (col->cell<std::string>(ii).size() > maxStr)
          maxStr = col->cell<std::string>(ii).size();
      }
      // If the column is empty fill the data with spaces.
      // Strings containing spaces only will be read back in as empty strings.
      if (maxStr == 0) {
        maxStr = 1;
      }
      int dims_array[2] = {nRows, static_cast<int>(maxStr)};
      int asize[2] = {1, dims_array[1]};

      NXcompmakedata(fileID, str.c_str(), NXnumtype::CHAR, 2, dims_array, false, asize);
      NXopendata(fileID, str.c_str());
      auto toNexus = new char[maxStr * nRows];
      for (int ii = 0; ii < nRows; ii++) {
        std::string rowStr = col->cell<std::string>(ii);
        for (size_t ic = 0; ic < rowStr.size(); ic++)
          toNexus[ii * maxStr + ic] = rowStr[ic];
        for (size_t ic = rowStr.size(); ic < static_cast<size_t>(maxStr); ic++)
          toNexus[ii * maxStr + ic] = ' ';
      }

      NXputdata(fileID, toNexus);
      delete[] toNexus;

      // attributes
      std::string units = "N/A";
      std::string interpret_as = "A string";
      NXputattr(fileID, "units", units.c_str(), static_cast<int>(units.size()), NXnumtype::CHAR);
      NXputattr(fileID, "interpret_as", interpret_as.c_str(), static_cast<int>(interpret_as.size()), NXnumtype::CHAR);

      NXclosedata(fileID);
    } else if (col->isType<std::vector<int>>()) {
      writeNexusVectorColumn<std::vector<int>, int>(col, str, NXnumtype::INT32, "");
    } else if (col->isType<std::vector<double>>()) {
      writeNexusVectorColumn<std::vector<double>, double>(col, str, NXnumtype::FLOAT64, "");
    } else if (col->isType<Kernel::V3D>()) {
      writeNexusVectorColumn<Kernel::V3D, double>(col, str, NXnumtype::FLOAT64, "V3D");
    }

    // write out title
    NXopendata(fileID, str.c_str());
    NXputattr(fileID, "name", col->name().c_str(), static_cast<int>(col->name().size()), NXnumtype::CHAR);
    NXclosedata(fileID);
  }

  status = NXclosegroup(fileID);
  return ((status == NXstatus::NX_ERROR) ? 3 : 0);
}

//-------------------------------------------------------------------------------------
/** Write out a combined chunk of event data
 *
 * @param ws :: an EventWorkspace
 * @param indices :: array of event list indexes
 * @param tofs :: array of TOFs
 * @param weights :: array of event weights
 * @param errorSquareds :: array of event squared errors
 * @param pulsetimes :: array of pulsetimes
 * @param compress :: if true, compress the entry
 */
int NexusFileIO::writeNexusProcessedDataEventCombined(const DataObjects::EventWorkspace_const_sptr &ws,
                                                      std::vector<int64_t> const &indices, double const *tofs,
                                                      float const *weights, float const *errorSquareds,
                                                      int64_t const *pulsetimes, bool compress) const {
  NXopengroup(fileID, "event_workspace", "NXdata");

  // The array of indices for each event list #
  int dims_array[1] = {static_cast<int>(indices.size())};
  if (!indices.empty()) {
    if (compress)
      NXcompmakedata(fileID, "indices", NXnumtype::INT64, 1, dims_array, m_nexuscompression, dims_array);
    else
      NXmakedata(fileID, "indices", NXnumtype::INT64, 1, dims_array);
    NXopendata(fileID, "indices");
    NXputdata(fileID, indices.data());
    std::string yUnits = ws->YUnit();
    std::string yUnitLabel = ws->YUnitLabel();
    NXputattr(fileID, "units", yUnits.c_str(), static_cast<int>(yUnits.size()), NXnumtype::CHAR);
    NXputattr(fileID, "unit_label", yUnitLabel.c_str(), static_cast<int>(yUnitLabel.size()), NXnumtype::CHAR);
    NXclosedata(fileID);
  }

  // Write out each field
  dims_array[0] = static_cast<int>(indices.back()); // TODO big truncation error! This is the # of events
  if (tofs)
    NXwritedata("tof", NXnumtype::FLOAT64, 1, dims_array, tofs, compress);
  if (pulsetimes)
    NXwritedata("pulsetime", NXnumtype::INT64, 1, dims_array, pulsetimes, compress);
  if (weights)
    NXwritedata("weight", NXnumtype::FLOAT32, 1, dims_array, weights, compress);
  if (errorSquareds)
    NXwritedata("error_squared", NXnumtype::FLOAT32, 1, dims_array, errorSquareds, compress);

  // Close up the overall group
  NXstatus status = NXclosegroup(fileID);
  return ((status == NXstatus::NX_ERROR) ? 3 : 0);
}

//-------------------------------------------------------------------------------------
/** Write out an array to the open file. */
void NexusFileIO::NXwritedata(const char *name, NXnumtype datatype, int rank, int *dims_array, void const *data,
                              bool compress) const {
  if (compress) {
    // We'll use the same slab/buffer size as the size of the array
    NXcompmakedata(fileID, name, datatype, rank, dims_array, m_nexuscompression, dims_array);
  } else {
    // Write uncompressed.
    NXmakedata(fileID, name, datatype, rank, dims_array);
  }

  NXopendata(fileID, name);
  NXputdata(fileID, data);
  NXclosedata(fileID);
}

int NexusFileIO::findMantidWSEntries() const {
  // search exiting file for entries of form mantid_workspace_<n> and return
  // count
  int count = 0;
  std::map<std::string, std::string> entries = m_filehandle->getEntries();
  for (auto &entrie : entries) {
    if (entrie.second == "NXentry") {
      if (entrie.first.find("mantid_workspace_") == 0)
        count++;
    }
  }

  return count;
}

/**
 * Write bin masking information
 * @param ws :: The workspace
 * @return true for OK, false for error
 */
bool NexusFileIO::writeNexusBinMasking(const API::MatrixWorkspace_const_sptr &ws) const {
  std::vector<int> spectra;
  std::vector<std::size_t> bins;
  std::vector<double> weights;
  int spectra_count = 0;
  int offset = 0;
  for (std::size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    if (ws->hasMaskedBins(i)) {
      const API::MatrixWorkspace::MaskList &mList = ws->maskedBins(i);
      spectra.emplace_back(spectra_count);
      spectra.emplace_back(offset);
      for (const auto &mask : mList) {
        bins.emplace_back(mask.first);
        weights.emplace_back(mask.second);
      }
      ++spectra_count;
      offset += static_cast<int>(mList.size());
    }
  }

  if (spectra_count == 0)
    return false;

  NXstatus status;

  // save spectra offsets as a 2d array of ints
  int dimensions[2]{spectra_count, 2};
  status = NXmakedata(fileID, "masked_spectra", NXnumtype::INT32, 2, dimensions);
  if (status == NXstatus::NX_ERROR)
    return false;
  NXopendata(fileID, "masked_spectra");
  const std::string description = "spectra index,offset in masked_bins and mask_weights";
  NXputattr(fileID, "description", description.c_str(), static_cast<int>(description.size() + 1), NXnumtype::CHAR);
  NXputdata(fileID, spectra.data());
  NXclosedata(fileID);

  // save masked bin indices
  dimensions[0] = static_cast<int>(bins.size());
  status = NXmakedata(fileID, "masked_bins", NXnumtype::UINT64, 1, dimensions);
  if (status == NXstatus::NX_ERROR)
    return false;
  NXopendata(fileID, "masked_bins");
  NXputdata(fileID, bins.data());
  NXclosedata(fileID);

  // save masked bin weights
  dimensions[0] = static_cast<int>(bins.size());
  status = NXmakedata(fileID, "mask_weights", NXnumtype::FLOAT64, 1, dimensions);
  if (status == NXstatus::NX_ERROR)
    return false;
  NXopendata(fileID, "mask_weights");
  NXputdata(fileID, weights.data());
  NXclosedata(fileID);

  return true;
}

/** Get all the Nexus entry types for a file
 *
 * Try to open named Nexus file and return all entries plus the definition found
 *for each.
 * If definition not found, try and return "analysis" field (Muon V1 files)
 * Closes file on exit.
 *
 * @param fileName :: file to open
 * @param entryName :: vector that gets filled with strings with entry names
 * @param definition :: vector that gets filled with the "definition" or
 *"analysis" string.
 * @return count of entries if OK, -1 failed to open file.
 */
int getNexusEntryTypes(const std::string &fileName, std::vector<std::string> &entryName,
                       std::vector<std::string> &definition) {
  std::unique_ptr<::NeXus::File> fileH;

  try {
    fileH = std::make_unique<::NeXus::File>(fileName);
  } catch (::NeXus::Exception &) {
    return -1;
  }
  entryName.clear();
  definition.clear();

  //
  // Loop through all entries looking for the definition section in each (or
  // analysis for MuonV1)
  //
  std::vector<std::string> entryList;

  std::pair<std::string, std::string> entry;
  while (true) {
    entry = fileH->getNextEntry();
    if (entry.first == NULL_STR && entry.second == NULL_STR)
      break;

    if (entry.second == "NXentry")
      entryList.emplace_back(entry.first);
  }

  // for each entry found, look for "analysis" or "definition" text data fields
  // and return value plus entry name

  for (auto &item : entryList) {
    fileH->openGroup(item, "NXentry");
    // loop through field names in this entry
    while (true) {
      entry = fileH->getNextEntry();
      if (entry.first == NULL_STR && entry.second == NULL_STR)
        break;
      // if a data field
      if (entry.second == "SDS") {
        // if one of the two names we are looking for
        if (entry.first == "definition" || entry.first == "analysis") {
          std::string value;
          fileH->readData(entry.first, value);
          definition.emplace_back(value);
          entryName.emplace_back(item);
        }
      }
    }
    fileH->closeGroup();
  }

  return (static_cast<int>(entryName.size()));
}

/**
 Destructor
 */
NexusFileIO::~NexusFileIO() {
  // Close the nexus file if not already closed.
  // this->closeNexusFile();
}

} // namespace Mantid::NeXus
