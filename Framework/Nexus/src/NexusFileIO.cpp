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
#include "MantidNexus/NexusFileIO.h"

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid {
namespace NeXus {
using namespace Kernel;
using namespace API;
using namespace DataObjects;

namespace {
/// static logger
Logger g_log("NexusFileIO");
} // namespace

/// Empty default constructor
NexusFileIO::NexusFileIO()
    : fileID(), m_filehandle(), m_nexuscompression(NX_COMP_LZW),
      m_progress(nullptr), m_filename() {}

/// Constructor that supplies a progress object
NexusFileIO::NexusFileIO(Progress *prog)
    : fileID(), m_filehandle(), m_nexuscompression(NX_COMP_LZW),
      m_progress(prog), m_filename() {}

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

void NexusFileIO::openNexusWrite(const std::string &fileName,
                                 NexusFileIO::optional_size_t entryNumber) {
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
  if (Poco::File(m_filename).exists())
    mode = NXACC_RDWR;

  else {
    if (fileName.find(".xml") < fileName.size() ||
        fileName.find(".XML") < fileName.size()) {
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
    if (status == NX_ERROR) {
      g_log.error("Unable to open file " + fileName);
      throw Exception::FileError("Unable to open File:", fileName);
    }
    auto file = new ::NeXus::File(fileID, true);
    // clang-format off
    m_filehandle = boost::shared_ptr< ::NeXus::File>(file);
    // clang-format on
  }

  //
  // for existing files, search for any current mantid_workspace_<n> entries and
  // set the
  // new name to be n+1 so that we do not over-write by default. This may need
  // changing.
  //
  if (mode == NXACC_RDWR) {
    size_t count = 0;
    if (entryNumber.is_initialized()) {
      // Use the entry number provided.
      count = entryNumber.get();
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
 for other
 Nexus specs.
 @param title :: title field.
 @param wsName :: workspace name.
 */
int NexusFileIO::writeNexusProcessedHeader(const std::string &title,
                                           const std::string &wsName) const {

  std::string className = "Mantid Processed Workspace";
  std::vector<std::string> attributes, avalues;
  if (!writeNxValue<std::string>("title", title, NX_CHAR, attributes, avalues))
    return (3);

  // name for workspace if this is a multi workspace nexus file
  if (!wsName.empty()) {
    if (!writeNxValue<std::string>("workspace_name", wsName, NX_CHAR,
                                   attributes, avalues))
      return (3);
  }

  attributes.emplace_back("URL");
  avalues.push_back(
      "http://www.nexusformat.org/instruments/xml/NXprocessed.xml");
  attributes.emplace_back("Version");
  avalues.emplace_back("1.0");
  // this may not be the "correct" long term path, but it is valid at present
  if (!writeNxValue<std::string>("definition", className, NX_CHAR, attributes,
                                 avalues))
    return (3);
  avalues.clear();
  avalues.emplace_back("http://www.isis.rl.ac.uk/xml/IXmantid.xml");
  avalues.emplace_back("1.0");
  if (!writeNxValue<std::string>("definition_local", className, NX_CHAR,
                                 attributes, avalues))
    return (3);
  return (0);
}

//-----------------------------------------------------------------------------------------------
//
// write an NXdata entry with Float array values
//
void NexusFileIO::writeNxFloatArray(
    const std::string &name, const std::vector<double> &values,
    const std::vector<std::string> &attributes,
    const std::vector<std::string> &avalues) const {
  m_filehandle->writeData(name, values);
  m_filehandle->openData(name);
  for (size_t it = 0; it < attributes.size(); ++it)
    m_filehandle->putAttr(attributes[it], avalues[it]);
  m_filehandle->closeData();
}

//-----------------------------------------------------------------------------------------------
//
// write an NXdata entry with String array values
//
bool NexusFileIO::writeNxStringArray(
    const std::string &name, const std::vector<std::string> &values,
    const std::vector<std::string> &attributes,
    const std::vector<std::string> &avalues) const {
  int dimensions[2];
  size_t maxlen = 0;
  dimensions[0] = static_cast<int>(values.size());
  for (const auto &value : values)
    if (value.size() > maxlen)
      maxlen = value.size();
  dimensions[1] = static_cast<int>(maxlen);
  NXstatus status = NXmakedata(fileID, name.c_str(), NX_CHAR, 2, dimensions);
  if (status == NX_ERROR)
    return (false);
  NXopendata(fileID, name.c_str());
  for (size_t it = 0; it < attributes.size(); ++it)
    NXputattr(fileID, attributes[it].c_str(), avalues[it].c_str(),
              static_cast<int>(avalues[it].size() + 1), NX_CHAR);
  auto strs = new char[values.size() * maxlen];
  for (size_t i = 0; i < values.size(); i++) {
    strncpy(&strs[i * maxlen], values[i].c_str(), maxlen);
  }
  NXputdata(fileID, strs);
  NXclosedata(fileID);
  delete[] strs;
  return (true);
}
//
// Write an NXnote entry with data giving parameter pair values for algorithm
// history and environment
// Use NX_CHAR instead of NX_BINARY for the parameter values to make more
// simple.
//
bool NexusFileIO::writeNxNote(const std::string &noteName,
                              const std::string &author,
                              const std::string &date,
                              const std::string &description,
                              const std::string &pairValues) const {
  m_filehandle->makeGroup(noteName, "NXnote", true);

  std::vector<std::string> attributes, avalues;
  if (!date.empty()) {
    attributes.emplace_back("date");
    avalues.push_back(date);
  }
  if (!writeNxValue<std::string>("author", author, NX_CHAR, attributes,
                                 avalues))
    return (false);
  attributes.clear();
  avalues.clear();

  if (!writeNxValue<std::string>("description", description, NX_CHAR,
                                 attributes, avalues))
    return (false);
  if (!writeNxValue<std::string>("data", pairValues, NX_CHAR, attributes,
                                 avalues))
    return (false);

  m_filehandle->closeGroup();
  return (true);
}

//-------------------------------------------------------------------------------------
/** Write out a MatrixWorkspace's data as a 2D matrix.
 * Use writeNexusProcessedDataEvent if writing an EventWorkspace.
 */
int NexusFileIO::writeNexusProcessedData2D(
    const API::MatrixWorkspace_const_sptr &localworkspace,
    const bool &uniformSpectra, const std::vector<int> &spec,
    const char *group_name, bool write2Ddata) const {
  NXstatus status;

  // write data entry
  status = NXmakegroup(fileID, group_name, "NXdata");
  if (status == NX_ERROR)
    return (2);
  NXopengroup(fileID, group_name, "NXdata");
  // write workspace data
  const size_t nHist = localworkspace->getNumberHistograms();
  if (nHist < 1)
    return (2);
  const size_t nSpectBins = localworkspace->y(0).size();
  const size_t nSpect = spec.size();
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
      axis2.push_back((*sAxis)(spec[i]));
  else
    for (size_t i = 0; i < sAxis->length(); i++)
      axis2.push_back((*sAxis)(i));

  int start[2] = {0, 0};
  int asize[2] = {1, dims_array[1]};

  // -------------- Actually write the 2D data ----------------------------
  if (write2Ddata) {
    std::string name = "values";
    NXcompmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array,
                   m_nexuscompression, asize);
    NXopendata(fileID, name.c_str());
    for (size_t i = 0; i < nSpect; i++) {
      int s = spec[i];
      NXputslab(fileID, localworkspace->y(s).rawData().data(), start, asize);
      start[0]++;
    }
    if (m_progress != nullptr)
      m_progress->reportIncrement(1, "Writing data");
    int signal = 1;
    NXputattr(fileID, "signal", &signal, 1, NX_INT32);
    // More properties
    const std::string axesNames = "axis2,axis1";
    NXputattr(fileID, "axes", axesNames.c_str(),
              static_cast<int>(axesNames.size()), NX_CHAR);
    std::string yUnits = localworkspace->YUnit();
    std::string yUnitLabel = localworkspace->YUnitLabel();
    NXputattr(fileID, "units", yUnits.c_str(), static_cast<int>(yUnits.size()),
              NX_CHAR);
    NXputattr(fileID, "unit_label", yUnitLabel.c_str(),
              static_cast<int>(yUnitLabel.size()), NX_CHAR);
    NXclosedata(fileID);

    // error
    name = "errors";
    NXcompmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array,
                   m_nexuscompression, asize);
    NXopendata(fileID, name.c_str());
    start[0] = 0;
    for (size_t i = 0; i < nSpect; i++) {
      int s = spec[i];
      NXputslab(fileID, localworkspace->e(s).rawData().data(), start, asize);
      start[0]++;
    }

    if (m_progress != nullptr)
      m_progress->reportIncrement(1, "Writing data");

    // Fractional area for RebinnedOutput
    if (localworkspace->id() == "RebinnedOutput") {
      RebinnedOutput_const_sptr rebin_workspace =
          boost::dynamic_pointer_cast<const RebinnedOutput>(localworkspace);
      name = "frac_area";
      NXcompmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array,
                     m_nexuscompression, asize);
      NXopendata(fileID, name.c_str());
      start[0] = 0;
      for (size_t i = 0; i < nSpect; i++) {
        int s = spec[i];
        NXputslab(fileID, rebin_workspace->readF(s).data(), start, asize);
        start[0]++;
      }
      if (m_progress != nullptr)
        m_progress->reportIncrement(1, "Writing data");
    }

    // Potentially x error
    if (localworkspace->hasDx(0)) {
      dims_array[0] = static_cast<int>(nSpect);
      dims_array[1] = static_cast<int>(localworkspace->dx(0).size());
      std::string dxErrorName = "xerrors";
      NXcompmakedata(fileID, dxErrorName.c_str(), NX_FLOAT64, 2, dims_array,
                     m_nexuscompression, asize);
      NXopendata(fileID, dxErrorName.c_str());
      start[0] = 0;
      asize[1] = dims_array[1];
      for (size_t i = 0; i < nSpect; i++) {
        int s = spec[i];
        NXputslab(fileID, localworkspace->dx(s).rawData().data(), start, asize);
        start[0]++;
      }
    }

    NXclosedata(fileID);
  }

  // write X data, as single array or all values if "ragged"
  if (uniformSpectra) {
    dims_array[0] = static_cast<int>(localworkspace->x(0).size());
    NXmakedata(fileID, "axis1", NX_FLOAT64, 1, dims_array);
    NXopendata(fileID, "axis1");
    NXputdata(fileID, localworkspace->x(0).rawData().data());

  } else {
    dims_array[0] = static_cast<int>(nSpect);
    dims_array[1] = static_cast<int>(localworkspace->x(0).size());
    NXmakedata(fileID, "axis1", NX_FLOAT64, 2, dims_array);
    NXopendata(fileID, "axis1");
    start[0] = 0;
    asize[1] = dims_array[1];
    for (size_t i = 0; i < nSpect; i++) {
      NXputslab(fileID, localworkspace->x(i).rawData().data(), start, asize);
      start[0]++;
    }
  }

  std::string dist = (localworkspace->isDistribution()) ? "1" : "0";
  NXputattr(fileID, "distribution", dist.c_str(), 2, NX_CHAR);
  NXputattr(fileID, "units", xLabel.c_str(), static_cast<int>(xLabel.size()),
            NX_CHAR);

  auto label =
      boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(xAxis->unit());
  if (label) {
    NXputattr(fileID, "caption", label->caption().c_str(),
              static_cast<int>(label->caption().size()), NX_CHAR);
    auto unitLbl = label->label();
    NXputattr(fileID, "label", unitLbl.ascii().c_str(),
              static_cast<int>(unitLbl.ascii().size()), NX_CHAR);
  }

  NXclosedata(fileID);

  if (!sAxis->isText()) {
    // write axis2, maybe just spectra number
    dims_array[0] = static_cast<int>(axis2.size());
    NXmakedata(fileID, "axis2", NX_FLOAT64, 1, dims_array);
    NXopendata(fileID, "axis2");
    NXputdata(fileID, axis2.data());
    NXputattr(fileID, "units", sLabel.c_str(), static_cast<int>(sLabel.size()),
              NX_CHAR);

    auto label = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(
        sAxis->unit());
    if (label) {
      NXputattr(fileID, "caption", label->caption().c_str(),
                static_cast<int>(label->caption().size()), NX_CHAR);
      auto unitLbl = label->label();
      NXputattr(fileID, "label", unitLbl.ascii().c_str(),
                static_cast<int>(unitLbl.ascii().size()), NX_CHAR);
    }

    NXclosedata(fileID);
  } else {
    std::string textAxis;
    for (size_t i = 0; i < sAxis->length(); i++) {
      std::string label = sAxis->label(i);
      textAxis += label + "\n";
    }
    dims_array[0] = static_cast<int>(textAxis.size());
    NXmakedata(fileID, "axis2", NX_CHAR, 1, dims_array);
    NXopendata(fileID, "axis2");
    NXputdata(fileID, textAxis.c_str());
    NXputattr(fileID, "units", "TextAxis", 8, NX_CHAR);

    auto label = boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(
        sAxis->unit());
    if (label) {
      NXputattr(fileID, "caption", label->caption().c_str(),
                static_cast<int>(label->caption().size()), NX_CHAR);
      auto unitLbl = label->label();
      NXputattr(fileID, "label", unitLbl.ascii().c_str(),
                static_cast<int>(unitLbl.ascii().size()), NX_CHAR);
    }

    NXclosedata(fileID);
  }

  writeNexusBinMasking(localworkspace);

  status = NXclosegroup(fileID);
  return ((status == NX_ERROR) ? 3 : 0);
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
void NexusFileIO::writeTableColumn(int type, const std::string &interpret_as,
                                   const API::Column &col,
                                   const std::string &columnName) const {
  const int nRows = static_cast<int>(col.size());
  int dims_array[1] = {nRows};

  auto toNexus = new NexusT[nRows];
  for (int ii = 0; ii < nRows; ii++)
    toNexus[ii] = static_cast<NexusT>(col.cell<ColumnT>(ii));
  NXwritedata(columnName.c_str(), type, 1, dims_array, toNexus, false);
  delete[] toNexus;

  // attributes
  NXopendata(fileID, columnName.c_str());
  std::string units = "Not known";
  NXputattr(fileID, "units", units.c_str(), static_cast<int>(units.size()),
            NX_CHAR);
  NXputattr(fileID, "interpret_as", interpret_as.c_str(),
            static_cast<int>(interpret_as.size()), NX_CHAR);
  NXclosedata(fileID);
}

// Helper templated definitions
namespace {

// Get a size of a vector to be used in writeNexusVectorColumn(...)
template <typename VecType> size_t getSizeOf(const VecType &vec) {
  return vec.size();
}

// Special case of V3D
size_t getSizeOf(const Kernel::V3D &) { return 3; }
} // namespace

/**
 * Writes given vector column to the currently open Nexus file.
 * @param col     :: Column to write
 * @param columnName :: Name of NXdata to write to
 * @param nexusType  :: Nexus type to use to store data
 * @param interpret_as   :: Name of the type to use for "interpret_as" attribute
 */
template <typename VecType, typename ElemType>
void NexusFileIO::writeNexusVectorColumn(
    Column_const_sptr col, const std::string &columnName, int nexusType,
    const std::string &interpret_as) const {
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

    NXputattr(fileID, rowSizeAttrName.str().c_str(), &size, 1, NX_INT32);
  }

  std::string units = "Not known";

  // Write general attributes
  NXputattr(fileID, "units", units.c_str(), static_cast<int>(units.size()),
            NX_CHAR);
  NXputattr(fileID, "interpret_as", interpret_as.c_str(),
            static_cast<int>(interpret_as.size()), NX_CHAR);

  NXclosedata(fileID);
}
//-------------------------------------------------------------------------------------
/** Write out a table Workspace's
 */
int NexusFileIO::writeNexusTableWorkspace(
    const API::ITableWorkspace_const_sptr &itableworkspace,
    const char *group_name) const {
  NXstatus status = NX_ERROR;

  boost::shared_ptr<const TableWorkspace> tableworkspace =
      boost::dynamic_pointer_cast<const TableWorkspace>(itableworkspace);
  boost::shared_ptr<const PeaksWorkspace> peakworkspace =
      boost::dynamic_pointer_cast<const PeaksWorkspace>(itableworkspace);

  if (!tableworkspace && !peakworkspace)
    return 3;

  // write data entry
  status = NXmakegroup(fileID, group_name, "NXdata");
  if (status == NX_ERROR)
    return (2);
  NXopengroup(fileID, group_name, "NXdata");

  int nRows = static_cast<int>(itableworkspace->rowCount());

  for (size_t i = 0; i < itableworkspace->columnCount(); i++) {
    Column_const_sptr col = itableworkspace->getColumn(i);

    std::string str = "column_" + std::to_string(i + 1);

    if (col->isType<double>()) {
      writeTableColumn<double, double>(NX_FLOAT64, "", *col, str);
    } else if (col->isType<float>()) {
      writeTableColumn<float, float>(NX_FLOAT32, "", *col, str);
    } else if (col->isType<int>()) {
      writeTableColumn<int, int32_t>(NX_INT32, "", *col, str);
    } else if (col->isType<uint32_t>()) {
      writeTableColumn<uint32_t, uint32_t>(NX_UINT32, "", *col, str);
    } else if (col->isType<int64_t>()) {
      writeTableColumn<int64_t, int64_t>(NX_INT64, "", *col, str);
    } else if (col->isType<size_t>()) {
      writeTableColumn<size_t, uint64_t>(NX_UINT64, "", *col, str);
    } else if (col->isType<Boolean>()) {
      writeTableColumn<bool, bool>(NX_UINT8, "", *col, str);
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

      NXcompmakedata(fileID, str.c_str(), NX_CHAR, 2, dims_array, false, asize);
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
      NXputattr(fileID, "units", units.c_str(), static_cast<int>(units.size()),
                NX_CHAR);
      NXputattr(fileID, "interpret_as", interpret_as.c_str(),
                static_cast<int>(interpret_as.size()), NX_CHAR);

      NXclosedata(fileID);
    } else if (col->isType<std::vector<int>>()) {
      writeNexusVectorColumn<std::vector<int>, int>(col, str, NX_INT32, "");
    } else if (col->isType<std::vector<double>>()) {
      writeNexusVectorColumn<std::vector<double>, double>(col, str, NX_FLOAT64,
                                                          "");
    } else if (col->isType<Kernel::V3D>()) {
      writeNexusVectorColumn<Kernel::V3D, double>(col, str, NX_FLOAT64, "V3D");
    }

    // write out title
    NXopendata(fileID, str.c_str());
    NXputattr(fileID, "name", col->name().c_str(),
              static_cast<int>(col->name().size()), NX_CHAR);
    NXclosedata(fileID);
  }

  status = NXclosegroup(fileID);
  return ((status == NX_ERROR) ? 3 : 0);
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
int NexusFileIO::writeNexusProcessedDataEventCombined(
    const DataObjects::EventWorkspace_const_sptr &ws,
    std::vector<int64_t> &indices, double *tofs, float *weights,
    float *errorSquareds, int64_t *pulsetimes, bool compress) const {
  NXopengroup(fileID, "event_workspace", "NXdata");

  // The array of indices for each event list #
  int dims_array[1] = {static_cast<int>(indices.size())};
  if (!indices.empty()) {
    if (compress)
      NXcompmakedata(fileID, "indices", NX_INT64, 1, dims_array,
                     m_nexuscompression, dims_array);
    else
      NXmakedata(fileID, "indices", NX_INT64, 1, dims_array);
    NXopendata(fileID, "indices");
    NXputdata(fileID, indices.data());
    std::string yUnits = ws->YUnit();
    std::string yUnitLabel = ws->YUnitLabel();
    NXputattr(fileID, "units", yUnits.c_str(), static_cast<int>(yUnits.size()),
              NX_CHAR);
    NXputattr(fileID, "unit_label", yUnitLabel.c_str(),
              static_cast<int>(yUnitLabel.size()), NX_CHAR);
    NXclosedata(fileID);
  }

  // Write out each field
  dims_array[0] = static_cast<int>(
      indices.back()); // TODO big truncation error! This is the # of events
  if (tofs)
    NXwritedata("tof", NX_FLOAT64, 1, dims_array, tofs, compress);
  if (pulsetimes)
    NXwritedata("pulsetime", NX_INT64, 1, dims_array, pulsetimes, compress);
  if (weights)
    NXwritedata("weight", NX_FLOAT32, 1, dims_array, weights, compress);
  if (errorSquareds)
    NXwritedata("error_squared", NX_FLOAT32, 1, dims_array, errorSquareds,
                compress);

  // Close up the overall group
  NXstatus status = NXclosegroup(fileID);
  return ((status == NX_ERROR) ? 3 : 0);
}

//-------------------------------------------------------------------------------------
/** Write out all of the event lists in the given workspace
 * @param ws :: an EventWorkspace */
int NexusFileIO::writeNexusProcessedDataEvent(
    const DataObjects::EventWorkspace_const_sptr &ws) {
  // write data entry
  NXstatus status = NXmakegroup(fileID, "event_workspace", "NXdata");
  if (status == NX_ERROR)
    return (2);
  NXopengroup(fileID, "event_workspace", "NXdata");

  for (size_t wi = 0; wi < ws->getNumberHistograms(); wi++) {
    std::ostringstream group_name;
    group_name << "event_list_" << wi;
    this->writeEventList(ws->getSpectrum(wi), group_name.str());
  }

  // Close up the overall group
  status = NXclosegroup(fileID);
  return ((status == NX_ERROR) ? 3 : 0);
}

//-------------------------------------------------------------------------------------
/** Write out an array to the open file. */
void NexusFileIO::NXwritedata(const char *name, int datatype, int rank,
                              int *dims_array, void *data,
                              bool compress) const {
  if (compress) {
    // We'll use the same slab/buffer size as the size of the array
    NXcompmakedata(fileID, name, datatype, rank, dims_array, m_nexuscompression,
                   dims_array);
  } else {
    // Write uncompressed.
    NXmakedata(fileID, name, datatype, rank, dims_array);
  }

  NXopendata(fileID, name);
  NXputdata(fileID, data);
  NXclosedata(fileID);
}

//-------------------------------------------------------------------------------------
/** Write out the event list data, no matter what the underlying event type is
 * @param events :: vector of TofEvent or WeightedEvent, etc.
 * @param writeTOF :: if true, write the TOF values
 * @param writePulsetime :: if true, write the pulse time values
 * @param writeWeight :: if true, write the event weights
 * @param writeError :: if true, write the errors
 */
template <class T>
void NexusFileIO::writeEventListData(std::vector<T> events, bool writeTOF,
                                     bool writePulsetime, bool writeWeight,
                                     bool writeError) const {
  // Do nothing if there are no events.
  if (events.empty())
    return;

  size_t num = events.size();
  auto tofs = new double[num];
  auto weights = new double[num];
  auto errorSquareds = new double[num];
  auto pulsetimes = new int64_t[num];

  size_t i = 0;
  // Fill the C-arrays with the fields from all the events, as requested.
  for (auto it = events.cbegin(); it != events.cend(); ++it) {
    if (writeTOF)
      tofs[i] = it->tof();
    if (writePulsetime)
      pulsetimes[i] = it->pulseTime().totalNanoseconds();
    if (writeWeight)
      weights[i] = it->weight();
    if (writeError)
      errorSquareds[i] = it->errorSquared();
    ++i;
  }

  // Write out all the required arrays.
  int dims_array[1] = {static_cast<int>(num)};
  // In this mode, compressing makes things extremely slow! Not to be used for
  // managed event workspaces.
  bool compress = true; //(num > 100);
  if (writeTOF)
    NXwritedata("tof", NX_FLOAT64, 1, dims_array, tofs, compress);
  if (writePulsetime)
    NXwritedata("pulsetime", NX_INT64, 1, dims_array, pulsetimes, compress);
  if (writeWeight)
    NXwritedata("weight", NX_FLOAT32, 1, dims_array, weights, compress);
  if (writeError)
    NXwritedata("error_squared", NX_FLOAT32, 1, dims_array, errorSquareds,
                compress);

  // Free mem.
  delete[] tofs;
  delete[] weights;
  delete[] errorSquareds;
  delete[] pulsetimes;
}

//-------------------------------------------------------------------------------------
/** Write out an event list into an already-opened group
 * @param el :: reference to the EventList to write.
 * @param group_name :: group_name to create.
 * */
int NexusFileIO::writeEventList(const DataObjects::EventList &el,
                                std::string group_name) const {
  // write data entry
  NXstatus status = NXmakegroup(fileID, group_name.c_str(), "NXdata");
  if (status == NX_ERROR)
    return (2);
  NXopengroup(fileID, group_name.c_str(), "NXdata");

  // Copy the detector IDs to an array.
  const auto &dets = el.getDetectorIDs();

  // Write out the detector IDs
  if (!dets.empty()) {
    std::vector<detid_t> detectorIDs(dets.begin(), dets.end());
    int dims_array[1];
    NXwritedata("detector_IDs", NX_INT64, 1, dims_array, detectorIDs.data(),
                false);
  }

  std::string eventType("UNKNOWN");
  size_t num = el.getNumberEvents();
  switch (el.getEventType()) {
  case TOF:
    eventType = "TOF";
    writeEventListData(el.getEvents(), true, true, false, false);
    break;
  case WEIGHTED:
    eventType = "WEIGHTED";
    writeEventListData(el.getWeightedEvents(), true, true, true, true);
    break;
  case WEIGHTED_NOTIME:
    eventType = "WEIGHTED_NOTIME";
    writeEventListData(el.getWeightedEventsNoTime(), true, false, true, true);
    break;
  }

  // --- Save the type of sorting -----
  std::string sortType;
  switch (el.getSortType()) {
  case TOF_SORT:
    sortType = "TOF_SORT";
    break;
  case PULSETIME_SORT:
    sortType = "PULSETIME_SORT";
    break;
  case UNSORTED:
  default:
    sortType = "UNSORTED";
    break;
  }
  NXputattr(fileID, "sort_type", sortType.c_str(),
            static_cast<int>(sortType.size()), NX_CHAR);

  // Save an attribute with the type of each event.
  NXputattr(fileID, "event_type", eventType.c_str(),
            static_cast<int>(eventType.size()), NX_CHAR);
  // Save an attribute with the number of events
  NXputattr(fileID, "num_events", &num, 1, NX_INT64);

  // Close it up!
  status = NXclosegroup(fileID);
  return ((status == NX_ERROR) ? 3 : 0);
}

//-------------------------------------------------------------------------------------
/** Read the size of the data section in a mantid_workspace_entry and also get
 *the names of axes
 *
 */

int NexusFileIO::getWorkspaceSize(int &numberOfSpectra, int &numberOfChannels,
                                  int &numberOfXpoints, bool &uniformBounds,
                                  std::string &axesUnits,
                                  std::string &yUnits) const {
  NXstatus status;
  // open workspace group
  status = NXopengroup(fileID, "workspace", "NXdata");
  if (status == NX_ERROR)
    return (1);
  // open "values" data which is identified by attribute "signal", if it exists
  std::string entry;
  if (checkEntryAtLevelByAttribute("signal", entry))
    status = NXopendata(fileID, entry.c_str());
  else {
    status = NXclosegroup(fileID);
    return (2);
  }
  if (status == NX_ERROR) {
    status = NXclosegroup(fileID);
    return (2);
  }
  // read workspace data size
  int rank, dim[2], type;
  status = NXgetinfo(fileID, &rank, dim, &type);
  if (status == NX_ERROR)
    return (3);
  numberOfSpectra = dim[0];
  numberOfChannels = dim[1];
  // get axes attribute
  char sbuf[NX_MAXNAMELEN];
  int len = NX_MAXNAMELEN;
  type = NX_CHAR;

  char unitsAttrName[] = "units";
  if (checkAttributeName(unitsAttrName)) {
    status = NXgetattr(fileID, unitsAttrName, sbuf, &len, &type);
    if (status != NX_ERROR)
      yUnits = sbuf;
    NXclosedata(fileID);
  }
  //
  // read axis1 size
  status = NXopendata(fileID, "axis1");
  if (status == NX_ERROR)
    return (4);
  len = NX_MAXNAMELEN;
  type = NX_CHAR;
  NXgetattr(fileID, unitsAttrName, sbuf, &len, &type);
  axesUnits = std::string(sbuf, len);
  NXgetinfo(fileID, &rank, dim, &type);
  // non-uniform X has 2D axis1 data
  if (rank == 1) {
    numberOfXpoints = dim[0];
    uniformBounds = true;
  } else {
    numberOfXpoints = dim[1];
    uniformBounds = false;
  }
  NXclosedata(fileID);
  NXopendata(fileID, "axis2");
  len = NX_MAXNAMELEN;
  type = NX_CHAR;
  NXgetattr(fileID, unitsAttrName, sbuf, &len, &type);
  axesUnits += std::string(":") + std::string(sbuf, len);
  NXclosedata(fileID);
  NXclosegroup(fileID);
  return (0);
}

bool NexusFileIO::checkAttributeName(const std::string &target) const {
  // see if the given attribute name is in the current level
  // return true if it is.
  // clang-format off
  const std::vector< ::NeXus::AttrInfo> infos = m_filehandle->getAttrInfos();
  // clang-format on
  for (const auto &info : infos) {
    if (target == info.name)
      return true;
  }

  return false;
}

int NexusFileIO::getXValues(MantidVec &xValues, const int &spectra) const {
  //
  // find the X values for spectra. If uniform, the spectra number is ignored.
  //
  int rank, dim[2], type;

  // open workspace group
  NXstatus status = NXopengroup(fileID, "workspace", "NXdata");
  if (status == NX_ERROR)
    return (1);
  // read axis1 size
  status = NXopendata(fileID, "axis1");
  if (status == NX_ERROR)
    return (2);
  NXgetinfo(fileID, &rank, dim, &type);
  if (rank == 1) {
    NXgetdata(fileID, xValues.data());
  } else {
    int start[2] = {spectra, 0};
    int size[2] = {1, dim[1]};
    NXgetslab(fileID, xValues.data(), start, size);
  }
  NXclosedata(fileID);
  NXclosegroup(fileID);
  return (0);
}

int NexusFileIO::getSpectra(MantidVec &values, MantidVec &errors,
                            const int &spectra) const {
  //
  // read the values and errors for spectra
  //
  int rank, dim[2], type;

  // open workspace group
  NXstatus status = NXopengroup(fileID, "workspace", "NXdata");
  if (status == NX_ERROR)
    return (1);
  std::string entry;
  if (checkEntryAtLevelByAttribute("signal", entry))
    status = NXopendata(fileID, entry.c_str());
  else {
    status = NXclosegroup(fileID);
    return (2);
  }
  if (status == NX_ERROR) {
    NXclosegroup(fileID);
    return (2);
  }
  NXgetinfo(fileID, &rank, dim, &type);
  // get buffer and block size
  int start[2] = {spectra - 1, 0};
  int size[2] = {1, dim[1]};
  NXgetslab(fileID, values.data(), start, size);
  NXclosedata(fileID);

  // read errors
  status = NXopendata(fileID, "errors");
  if (status == NX_ERROR)
    return (2);
  NXgetinfo(fileID, &rank, dim, &type);
  // set block size;
  size[1] = dim[1];
  NXgetslab(fileID, errors.data(), start, size);
  NXclosedata(fileID);

  NXclosegroup(fileID);

  return (0);
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

bool NexusFileIO::checkEntryAtLevel(const std::string &item) const {
  // Search the currently open level for name "item"
  std::map<std::string, std::string> entries = m_filehandle->getEntries();
  for (auto &entrie : entries) {
    if (entrie.first == item)
      return true;
  }

  return (false);
}

bool NexusFileIO::checkEntryAtLevelByAttribute(const std::string &attribute,
                                               std::string &entry) const {
  // Search the currently open level for a section with "attribute" and return
  // entry name
  std::map<std::string, std::string> entries = m_filehandle->getEntries();
  for (auto &entrie : entries) {
    if (entrie.second == "SDS") {
      m_filehandle->openData(entrie.first);
      bool result = checkAttributeName(attribute);
      m_filehandle->closeData();
      if (result) {
        entry = entrie.first;
        return true;
      }
    }
  }

  return (false);
}

/**
 * Write bin masking information
 * @param ws :: The workspace
 * @return true for OK, false for error
 */
bool NexusFileIO::writeNexusBinMasking(
    API::MatrixWorkspace_const_sptr ws) const {
  std::vector<int> spectra;
  std::vector<std::size_t> bins;
  std::vector<double> weights;
  int spectra_count = 0;
  int offset = 0;
  for (std::size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    if (ws->hasMaskedBins(i)) {
      const API::MatrixWorkspace::MaskList &mList = ws->maskedBins(i);
      spectra.push_back(spectra_count);
      spectra.push_back(offset);
      for (const auto &mask : mList) {
        bins.push_back(mask.first);
        weights.push_back(mask.second);
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
  status = NXmakedata(fileID, "masked_spectra", NX_INT32, 2, dimensions);
  if (status == NX_ERROR)
    return false;
  NXopendata(fileID, "masked_spectra");
  const std::string description =
      "spectra index,offset in masked_bins and mask_weights";
  NXputattr(fileID, "description", description.c_str(),
            static_cast<int>(description.size() + 1), NX_CHAR);
  NXputdata(fileID, spectra.data());
  NXclosedata(fileID);

  // save masked bin indices
  dimensions[0] = static_cast<int>(bins.size());
  status = NXmakedata(fileID, "masked_bins", NX_UINT64, 1, dimensions);
  if (status == NX_ERROR)
    return false;
  NXopendata(fileID, "masked_bins");
  NXputdata(fileID, bins.data());
  NXclosedata(fileID);

  // save masked bin weights
  dimensions[0] = static_cast<int>(bins.size());
  status = NXmakedata(fileID, "mask_weights", NX_FLOAT64, 1, dimensions);
  if (status == NX_ERROR)
    return false;
  NXopendata(fileID, "mask_weights");
  NXputdata(fileID, weights.data());
  NXclosedata(fileID);

  return true;
}

template <> std::string NexusFileIO::logValueType<double>() const {
  return "double";
}

template <> std::string NexusFileIO::logValueType<int>() const { return "int"; }

template <> std::string NexusFileIO::logValueType<bool>() const {
  return "bool";
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
int getNexusEntryTypes(const std::string &fileName,
                       std::vector<std::string> &entryName,
                       std::vector<std::string> &definition) {
  //
  //
  NXhandle fileH;
  NXaccess mode = NXACC_READ;
  NXstatus stat = NXopen(fileName.c_str(), mode, &fileH);
  if (stat == NX_ERROR)
    return (-1);
  //
  entryName.clear();
  definition.clear();
  char *nxname, *nxclass;
  int nxdatatype;
  nxname = new char[NX_MAXNAMELEN];
  nxclass = new char[NX_MAXNAMELEN];
  int rank, dims[2], type;
  //
  // Loop through all entries looking for the definition section in each (or
  // analysis for MuonV1)
  //
  std::vector<std::string> entryList;
  while ((stat = NXgetnextentry(fileH, nxname, nxclass, &nxdatatype)) ==
         NX_OK) {
    std::string nxc(nxclass);
    if (nxc == "NXentry")
      entryList.push_back(nxname);
  }
  // for each entry found, look for "analysis" or "definition" text data fields
  // and return value plus entry name
  for (auto &entry : entryList) {
    //
    stat = NXopengroup(fileH, entry.c_str(), "NXentry");
    // loop through field names in this entry
    while ((stat = NXgetnextentry(fileH, nxname, nxclass, &nxdatatype)) ==
           NX_OK) {
      std::string nxc(nxclass), nxn(nxname);
      // if a data field
      if (nxc == "SDS")
        // if one of the two names we are looking for
        if (nxn == "definition" || nxn == "analysis") {
          NXopendata(fileH, nxname);
          stat = NXgetinfo(fileH, &rank, dims, &type);
          if (stat == NX_ERROR)
            continue;
          auto value = new char[dims[0] + 1];
          stat = NXgetdata(fileH, value);
          if (stat == NX_ERROR)
            continue;
          value[dims[0]] = '\0';
          // return e.g entryName "analysis"/definition "muonTD"
          definition.push_back(value);
          entryName.push_back(entry);
          delete[] value;
          NXclosegroup(fileH); // close data group, then entry
          stat = NXclosegroup(fileH);
          break;
        }
    }
  }
  stat = NXclose(&fileH);
  delete[] nxname;
  delete[] nxclass;
  return (static_cast<int>(entryName.size()));
}

/**
 Destructor
 */
NexusFileIO::~NexusFileIO() {
  // Close the nexus file if not already closed.
  // this->closeNexusFile();
}

} // namespace NeXus
} // namespace Mantid
