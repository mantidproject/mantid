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

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid::NeXus {
using namespace Kernel;
using namespace API;
using namespace DataObjects;

namespace {
/// static logger
Logger g_log("NexusFileIO");
} // namespace

/// Empty default constructor
NexusFileIO::NexusFileIO() : m_filehandle(), m_nexuscompression(::NeXus::LZW), m_progress(nullptr), m_filename() {}

/// Constructor that supplies a progress object
NexusFileIO::NexusFileIO(Progress *prog)
    : m_filehandle(), m_nexuscompression(::NeXus::LZW), m_progress(prog), m_filename() {}

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
      m_nexuscompression = ::NeXus::NONE;
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

    auto file = new ::NeXus::File(fileName, mode);

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
  // NXstatus status;

  // write data entry
  try {
    m_filehandle->makeGroup(group_name, "NXdata", true);
  } catch (const ::NeXus::Exception &) {
    return 2;
  }

  // write workspace data
  const size_t nHist = localworkspace->getNumberHistograms();
  if (nHist < 1)
    return (2);
  const size_t nSpect = indices.size();
  size_t nSpectBins = localworkspace->y(0).size();
  if (raggedSpectra)
    for (size_t i = 0; i < nSpect; i++)
      nSpectBins = std::max(nSpectBins, localworkspace->y(indices[i]).size());
  ::NeXus::DimVector dims_array = {static_cast<::NeXus::dimsize_t>(nSpect),
                                   static_cast<::NeXus::dimsize_t>(nSpectBins)};

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

  ::NeXus::DimVector start = {0, 0};
  ::NeXus::DimSizeVector asize = {1, dims_array[1]};

  // -------------- Actually write the 2D data ----------------------------
  if (write2Ddata) {
    std::string name = "values";
    m_filehandle->makeCompData(name, NXnumtype::FLOAT64, dims_array, m_nexuscompression, asize, true);
    for (size_t i = 0; i < nSpect; i++) {
      m_filehandle->putSlab(localworkspace->y(indices[i]).rawData().data(), start, asize);
      start[0]++;
    }
    if (m_progress != nullptr)
      m_progress->reportIncrement(1, "Writing data");
    m_filehandle->putAttr("signal", 1);
    // More properties
    m_filehandle->putAttr("axes", "axis2,axis1");
    m_filehandle->putAttr("units", localworkspace->YUnit(), false);
    m_filehandle->putAttr("unit_label", localworkspace->YUnitLabel(), false);
    m_filehandle->closeData();

    // error
    name = "errors";
    m_filehandle->makeCompData(name, NXnumtype::FLOAT64, dims_array, m_nexuscompression, asize, true);
    start[0] = 0;
    for (size_t i = 0; i < nSpect; i++) {
      m_filehandle->putSlab(localworkspace->e(indices[i]).rawData().data(), start, asize);
      start[0]++;
    }

    if (m_progress != nullptr)
      m_progress->reportIncrement(1, "Writing data");

    // Fractional area for RebinnedOutput
    if (localworkspace->id() == "RebinnedOutput") {
      RebinnedOutput_const_sptr rebin_workspace = std::dynamic_pointer_cast<const RebinnedOutput>(localworkspace);
      name = "frac_area";
      m_filehandle->makeCompData(name, NXnumtype::FLOAT64, dims_array, m_nexuscompression, asize, true);
      start[0] = 0;
      for (size_t i = 0; i < nSpect; i++) {
        m_filehandle->putSlab(rebin_workspace->readF(indices[i]).data(), start, asize);
        start[0]++;
      }

      std::string finalized = (rebin_workspace->isFinalized()) ? "1" : "0";
      m_filehandle->putAttr("finalized", finalized);
      std::string sqrdErrs = (rebin_workspace->hasSqrdErrors()) ? "1" : "0";
      m_filehandle->putAttr("sqrd_errors", sqrdErrs);

      if (m_progress != nullptr)
        m_progress->reportIncrement(1, "Writing data");
    }

    // Potentially x error
    if (localworkspace->hasDx(0)) {
      dims_array[0] = static_cast<int>(nSpect);
      dims_array[1] = static_cast<int>(localworkspace->dx(0).size());
      std::string dxErrorName = "xerrors";
      m_filehandle->makeCompData(dxErrorName, NXnumtype::FLOAT64, dims_array, m_nexuscompression, asize, true);
      start[0] = 0;
      asize[1] = dims_array[1];
      for (size_t i = 0; i < nSpect; i++) {
        m_filehandle->putSlab(localworkspace->dx(indices[i]).rawData().data(), start, asize);
        start[0]++;
      }
    }

    m_filehandle->closeData();
  }

  // write X data, as single array or all values if "ragged"
  if (raggedSpectra) {
    size_t max_x_size{0};
    for (size_t i = 0; i < nSpect; i++)
      max_x_size = std::max(max_x_size, localworkspace->x(indices[i]).size());
    dims_array[0] = static_cast<int>(nSpect);
    dims_array[1] = static_cast<int>(max_x_size);
    m_filehandle->makeData("axis1", NXnumtype::FLOAT64, dims_array, true);
    start[0] = 0;

    // create vector of NaNs to fill invalid space at end of ragged array
    std::vector<double> nans(max_x_size, std::numeric_limits<double>::quiet_NaN());

    for (size_t i = 0; i < nSpect; i++) {
      size_t nBins = localworkspace->x(indices[i]).size();
      asize[1] = static_cast<int>(nBins);
      m_filehandle->putSlab(localworkspace->x(indices[i]).rawData().data(), start, asize);
      if (nBins < max_x_size) {
        start[1] = asize[1];
        asize[1] = static_cast<int>(max_x_size - nBins);
        m_filehandle->putSlab(nans.data(), start, asize);
        start[1] = 0;
      }
      start[0]++;
    }
  } else if (uniformSpectra) {
    m_filehandle->makeData("axis1", NXnumtype::FLOAT64,
                           ::NeXus::DimVector{static_cast<::NeXus::dimsize_t>(localworkspace->x(0).size())}, true);
    m_filehandle->putData(localworkspace->x(0).rawData().data());

  } else {
    dims_array[0] = static_cast<int>(nSpect);
    dims_array[1] = static_cast<int>(localworkspace->x(0).size());
    m_filehandle->makeData("axis1", NXnumtype::FLOAT64, dims_array, true);
    start[0] = 0;
    asize[1] = dims_array[1];
    for (size_t i = 0; i < nSpect; i++) {
      m_filehandle->putSlab(localworkspace->x(indices[i]).rawData().data(), start, asize);
      start[0]++;
    }
  }

  std::string dist = (localworkspace->isDistribution()) ? "1" : "0";
  m_filehandle->putAttr("distribution", dist);
  m_filehandle->putAttr("units", xLabel);

  auto label = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(xAxis->unit());
  if (label) {
    m_filehandle->putAttr("caption", label->caption(), false);
    auto unitLbl = label->label();
    m_filehandle->putAttr("label", unitLbl.ascii(), false);
  }

  m_filehandle->closeData();

  if (!sAxis->isText()) {
    // write axis2, maybe just spectra number
    m_filehandle->makeData("axis2", NXnumtype::FLOAT64,
                           ::NeXus::DimVector{static_cast<::NeXus::dimsize_t>(axis2.size())}, true);
    m_filehandle->putData(axis2.data());
    m_filehandle->putAttr("units", sLabel, false);

    auto unitLabel = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(sAxis->unit());
    if (unitLabel) {
      m_filehandle->putAttr("caption", unitLabel->caption(), false);
      auto unitLbl = unitLabel->label();
      m_filehandle->putAttr("label", unitLbl.ascii(), false);
    }

    m_filehandle->closeData();
  } else {
    std::string textAxis;
    for (size_t i = 0; i < sAxis->length(); i++) {
      textAxis += sAxis->label(i) + "\n";
    }
    m_filehandle->makeData("axis2", NXnumtype::CHAR,
                           ::NeXus::DimVector{static_cast<::NeXus::dimsize_t>(textAxis.size())}, true);
    m_filehandle->putData(textAxis.c_str());
    m_filehandle->putAttr("units", "TextAxis");

    auto unitLabel = std::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(sAxis->unit());
    if (unitLabel) {
      m_filehandle->putAttr("caption", unitLabel->caption(), false);
      auto unitLbl = unitLabel->label();
      m_filehandle->putAttr("label", unitLbl.ascii(), false);
    }

    m_filehandle->closeData();
  }

  writeNexusBinMasking(localworkspace);

  m_filehandle->closeGroup();
  return 0;
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
  const ::NeXus::DimVector dims_array = {nRows};

  auto toNexus = new NexusT[nRows];
  for (int ii = 0; ii < nRows; ii++)
    toNexus[ii] = static_cast<NexusT>(col.cell<ColumnT>(ii));
  writeData(columnName.c_str(), type, dims_array, toNexus, false);
  delete[] toNexus;

  // attributes
  m_filehandle->openData(columnName);
  std::string units = "Not known";
  m_filehandle->putAttr("units", units, false);
  m_filehandle->putAttr("interpret_as", interpret_as, false);
  m_filehandle->closeData();
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
  const ::NeXus::DimVector dims{static_cast<::NeXus::dimsize_t>(rowCount), static_cast<::NeXus::dimsize_t>(maxSize)};

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
  writeData(columnName.c_str(), nexusType, dims, data.get(), false);

  m_filehandle->openData(columnName);

  // Add sizes of rows as attributes. We can't use padding zeroes to determine
  // that because the
  // vector stored might end with zeroes as well.
  for (size_t i = 0; i < rowCount; ++i) {
    auto size = static_cast<int>(getSizeOf(column[i]));

    std::ostringstream rowSizeAttrName;
    rowSizeAttrName << "row_size_" << i;

    m_filehandle->putAttr(rowSizeAttrName.str(), size);
  }

  std::string units = "Not known";

  // Write general attributes
  m_filehandle->putAttr("units", units, false);
  m_filehandle->putAttr("interpret_as", interpret_as, false);

  m_filehandle->closeData();
}
//-------------------------------------------------------------------------------------
/** Write out a table Workspace's
 */
int NexusFileIO::writeNexusTableWorkspace(const API::ITableWorkspace_const_sptr &itableworkspace,
                                          const char *group_name) const {
  std::shared_ptr<const TableWorkspace> tableworkspace =
      std::dynamic_pointer_cast<const TableWorkspace>(itableworkspace);
  std::shared_ptr<const PeaksWorkspace> peakworkspace =
      std::dynamic_pointer_cast<const PeaksWorkspace>(itableworkspace);

  if (!tableworkspace && !peakworkspace)
    return 3;

  // write data entry
  try {
    m_filehandle->makeGroup(group_name, "NXdata", true);
  } catch (::NeXus::Exception &) {
    return 2;
  }

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
      const ::NeXus::DimVector dims_array = {nRows, static_cast<::NeXus::dimsize_t>(maxStr)};
      const ::NeXus::DimSizeVector asize = {1, dims_array[1]};

      m_filehandle->makeCompData(str, NXnumtype::CHAR, dims_array, ::NeXus::LZW, asize);

      m_filehandle->openData(str);
      auto toNexus = new char[maxStr * nRows];
      for (int ii = 0; ii < nRows; ii++) {
        std::string rowStr = col->cell<std::string>(ii);
        for (size_t ic = 0; ic < rowStr.size(); ic++)
          toNexus[ii * maxStr + ic] = rowStr[ic];
        for (size_t ic = rowStr.size(); ic < static_cast<size_t>(maxStr); ic++)
          toNexus[ii * maxStr + ic] = ' ';
      }

      m_filehandle->putData(toNexus);
      delete[] toNexus;

      // attributes
      std::string units = "N/A";
      std::string interpret_as = "A string";
      m_filehandle->putAttr("units", units);
      m_filehandle->putAttr("interpret_as", interpret_as);

      m_filehandle->closeData();
    } else if (col->isType<std::vector<int>>()) {
      writeNexusVectorColumn<std::vector<int>, int>(col, str, NXnumtype::INT32, "");
    } else if (col->isType<std::vector<double>>()) {
      writeNexusVectorColumn<std::vector<double>, double>(col, str, NXnumtype::FLOAT64, "");
    } else if (col->isType<Kernel::V3D>()) {
      writeNexusVectorColumn<Kernel::V3D, double>(col, str, NXnumtype::FLOAT64, "V3D");
    }

    // write out title
    m_filehandle->openData(str);
    m_filehandle->putAttr("name", col->name());
    m_filehandle->closeData();
  }

  try {
    m_filehandle->closeGroup();
  } catch (::NeXus::Exception &) {
    return 3;
  }
  return 0;
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
  m_filehandle->openGroup("event_workspace", "NXdata");

  // The array of indices for each event list #
  ::NeXus::DimVector dims_array = {static_cast<::NeXus::dimsize_t>(indices.size())};
  if (!indices.empty()) {
    if (compress)
      m_filehandle->makeCompData("indices", NXnumtype::INT64, dims_array, m_nexuscompression, dims_array);
    else
      m_filehandle->makeData("indices", NXnumtype::INT64, dims_array);
    m_filehandle->openData("indices");
    m_filehandle->putData(indices.data());
    m_filehandle->putAttr("units", ws->YUnit(), false);
    m_filehandle->putAttr("unit_label", ws->YUnitLabel(), false);
    m_filehandle->closeData();
  }

  // Write out each field
  dims_array[0] = static_cast<int>(indices.back()); // TODO big truncation error! This is the # of events
  if (tofs)
    writeData("tof", NXnumtype::FLOAT64, dims_array, tofs, compress);
  if (pulsetimes)
    writeData("pulsetime", NXnumtype::INT64, dims_array, pulsetimes, compress);
  if (weights)
    writeData("weight", NXnumtype::FLOAT32, dims_array, weights, compress);
  if (errorSquareds)
    writeData("error_squared", NXnumtype::FLOAT32, dims_array, errorSquareds, compress);

  // Close up the overall group
  m_filehandle->closeGroup();
  return 0;
}

//-------------------------------------------------------------------------------------
/** Write out an array to the open file. */
template <typename NumT>
void NexusFileIO::writeData(const char *name, NXnumtype datatype, ::NeXus::DimVector dims_array, NumT const *data,
                            bool compress) const {
  if (compress) {
    // We'll use the same slab/buffer size as the size of the array
    m_filehandle->makeCompData(name, datatype, dims_array, m_nexuscompression, dims_array);
  } else {
    // Write uncompressed.
    m_filehandle->makeData(name, datatype, dims_array);
  }

  m_filehandle->openData(name);
  m_filehandle->putData(data);
  m_filehandle->closeData();
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
  std::vector<int32_t> spectra;
  std::vector<int64_t> bins;
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

  // save spectra offsets as a 2d array of ints
  std::vector<int64_t> dimensions = {spectra_count, 2};

  m_filehandle->makeData("masked_spectra", NXnumtype::INT32, dimensions, true);
  m_filehandle->putAttr("description", "spectra index,offset in masked_bins and mask_weights");
  m_filehandle->putData(spectra.data());
  m_filehandle->closeData();

  // save masked bin indices
  dimensions[0] = static_cast<int>(bins.size());
  m_filehandle->makeData("masked_bins", NXnumtype::UINT64, dimensions, true);
  m_filehandle->putData(bins.data());
  m_filehandle->closeData();

  // save masked bin weights
  dimensions[0] = static_cast<int>(bins.size());
  m_filehandle->makeData("mask_weights", NXnumtype::FLOAT64, dimensions, true);
  m_filehandle->putData(weights.data());
  m_filehandle->closeData();

  return true;
}

/**
 Destructor
 */
NexusFileIO::~NexusFileIO() {
  // Close the nexus file if not already closed.
  // this->closeNexusFile();
}

} // namespace Mantid::NeXus
