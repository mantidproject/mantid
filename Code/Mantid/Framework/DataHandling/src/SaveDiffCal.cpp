#include "MantidDataHandling/SaveDiffCal.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"

#include <H5Cpp.h>
#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

using Mantid::API::FileProperty;
using Mantid::API::PropertyMode;
using Mantid::API::WorkspaceProperty;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_const_sptr;
using Mantid::DataObjects::GroupingWorkspace;
using Mantid::DataObjects::GroupingWorkspace_const_sptr;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::MaskWorkspace_const_sptr;
using Mantid::Kernel::Direction;

using namespace H5;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveDiffCal)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveDiffCal::SaveDiffCal() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveDiffCal::~SaveDiffCal() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SaveDiffCal::name() const { return "SaveDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveDiffCal::category() const {
  return "DataHandling;Diffraction";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SaveDiffCal::summary() const {
  return "Saves a calibration file for powder diffraction";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveDiffCal::init() {
  declareProperty(new WorkspaceProperty<ITableWorkspace>("CalibrationWorkspace",
                                                         "", Direction::Input),
                  "An output workspace.");

  declareProperty(
      new WorkspaceProperty<GroupingWorkspace>(
          "GroupingWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: An GroupingWorkspace workspace giving the grouping info.");

  declareProperty(
      new WorkspaceProperty<MaskWorkspace>(
          "MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: An Workspace workspace giving which detectors are masked.");

  declareProperty(new FileProperty("Filename", "", FileProperty::Save, ".h5"),
                  "Path to the .h5 file that will be created.");
}

std::map<std::string, std::string> SaveDiffCal::validateInputs() {
  std::map<std::string, std::string> result;

  ITableWorkspace_const_sptr calibrationWS =
      getProperty("CalibrationWorkspace");
  if (!bool(calibrationWS)) {
    result["CalibrationWorkspace"] = "Cannot save empty table";
  } else {
    size_t numRows = calibrationWS->rowCount();
    if (numRows == 0) {
      result["CalibrationWorkspace"] = "Cannot save empty table";
    } else {
      GroupingWorkspace_const_sptr groupingWS =
          getProperty("GroupingWorkspace");
      if (bool(groupingWS) && numRows < groupingWS->getNumberHistograms()) {
        result["GroupingWorkspace"] =
            "Must have same number of spectra as the table has rows";
      }
      MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace");
      if (bool(maskWS) && numRows < maskWS->getNumberHistograms()) {
        result["MaskWorkspace"] =
            "Must have same number of spectra as the table has rows";
      }
    }
  }

  return result;
}

namespace { // anonymous

DataSpace getDataSpace(const size_t length) {
  hsize_t dims[] = {length};
  return DataSpace(1, dims);
}

template <typename NumT> DataSpace getDataSpace(const std::vector<NumT> &data) {
  return getDataSpace(data.size());
}

/**
 * Sets up the chunking and compression rate.
 * @param length
 * @return The configured property list
 */
DSetCreatPropList getPropList(const std::size_t length) {
  DSetCreatPropList propList;
  hsize_t chunk_dims[1] = {length};
  propList.setChunk(1, chunk_dims);
  propList.setDeflate(6);
  return propList;
}

void writeStrAttribute(H5::Group &location, const std::string &name,
                       const std::string &value) {
  StrType attrType(0, H5T_VARIABLE);
  DataSpace attrSpace(H5S_SCALAR);
  auto groupAttr = location.createAttribute(name, attrType, attrSpace);
  groupAttr.write(attrType, value);
}

void writeArray(H5::Group &group, const std::string &name,
                const std::string &value) {
  StrType dataType(0, value.length() + 1);
  DataSpace dataSpace = getDataSpace(1);
  H5::DataSet data = group.createDataSet(name, dataType, dataSpace);
  data.write(value, dataType);
}

void writeArray(H5::Group &group, const std::string &name,
                const std::vector<double> &values) {
  DataType dataType(PredType::NATIVE_DOUBLE);
  DataSpace dataSpace = getDataSpace(values);

  DSetCreatPropList propList = getPropList(values.size());

  auto data = group.createDataSet(name, dataType, dataSpace, propList);
  data.write(&(values[0]), dataType);
}

void writeArray(H5::Group &group, const std::string &name,
                const std::vector<int32_t> &values) {
  DataType dataType(PredType::NATIVE_INT32);
  DataSpace dataSpace = getDataSpace(values);

  DSetCreatPropList propList = getPropList(values.size());

  auto data = group.createDataSet(name, dataType, dataSpace, propList);
  data.write(&(values[0]), dataType);
}

} // anonymous

void SaveDiffCal::writeDoubleFieldFromTable(H5::Group &group,
                                            const std::string &name) {
  auto column = m_calibrationWS->getColumn(name);
  std::vector<double> data;
  column->numeric_fill(data);
  data.erase(data.begin() + m_numValues, data.end());
  writeArray(group, name, std::vector<double>(data));
}

void SaveDiffCal::writeIntFieldFromTable(H5::Group &group,
                                         const std::string &name) {
  auto column = m_calibrationWS->getColumn(name);
  std::vector<int32_t> data;
  column->numeric_fill(data);
  data.erase(data.begin() + m_numValues, data.end());
  writeArray(group, name, std::vector<int32_t>(data));
}

// TODO should flip for mask
void SaveDiffCal::writeIntFieldFromSVWS(
    H5::Group &group, const std::string &name,
    DataObjects::SpecialWorkspace2D_const_sptr ws) {
  auto detidCol = m_calibrationWS->getColumn("detid");
  std::vector<detid_t> detids;
  detidCol->numeric_fill(detids);
  bool isMask = bool(boost::dynamic_pointer_cast<const MaskWorkspace>(ws));

  // output array defaults to all one (one group, use the pixel)
  const int32_t DEFAULT_VALUE = (isMask ? 0 : 1);
  std::vector<int32_t> values(m_numValues, DEFAULT_VALUE);

  int32_t value;
  for (size_t i = 0; i < m_numValues; ++i) {
    auto spectrum = ws->getSpectrum(i);
    auto ids = spectrum->getDetectorIDs();
    auto found = m_detidToIndex.find(*(ids.begin()));
    if (found != m_detidToIndex.end()) {
      value = static_cast<int32_t>(ws->getValue(found->first));
      // in maskworkspace 0=use, 1=dontuse
      if (isMask) {
        if (value == 0)
          value = 1;
        else
          value = 0;
      }
      values[found->second] = value;
    }
  }

  writeArray(group, name, values);
}

void SaveDiffCal::generateDetidToIndex() {
  m_detidToIndex.clear();

  auto detidCol = m_calibrationWS->getColumn("detid");
  std::vector<detid_t> detids;
  detidCol->numeric_fill(detids);

  const size_t numDets = detids.size();
  for (size_t i = 0; i < numDets; ++i) {
    m_detidToIndex[static_cast<detid_t>(detids[i])] = i;
  }
}

bool SaveDiffCal::tableHasColumn(const std::string name) const {
  const std::vector<std::string> names = m_calibrationWS->getColumnNames();
  for (auto it = names.begin(); it != names.end(); ++it) {
    if (name == (*it))
      return true;
  }

  return false;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveDiffCal::exec() {
  m_calibrationWS = getProperty("CalibrationWorkspace");
  this->generateDetidToIndex();
  GroupingWorkspace_const_sptr groupingWS = getProperty("GroupingWorkspace");
  MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace");
  std::string filename = getProperty("Filename");

  m_numValues = m_calibrationWS->rowCount();
  if (bool(groupingWS) && groupingWS->getNumberHistograms() < m_numValues) {
    m_numValues = groupingWS->getNumberHistograms();
  }
  if (bool(maskWS) && maskWS->getNumberHistograms() < m_numValues) {
    m_numValues = maskWS->getNumberHistograms();
  }

  // delete the file if it already exists
  if (Poco::File(filename).exists()) {
    Poco::File(filename).remove();
  }

  H5File file(filename, H5F_ACC_EXCL);

  auto calibrationGroup = file.createGroup("calibration");
  writeStrAttribute(calibrationGroup, "NX_class", "NXentry");

  this->writeDoubleFieldFromTable(calibrationGroup, "difc");
  this->writeDoubleFieldFromTable(calibrationGroup, "difa");
  this->writeDoubleFieldFromTable(calibrationGroup, "tzero");

  this->writeIntFieldFromTable(calibrationGroup, "detid");
  if (this->tableHasColumn("dasid")) // optional field
    this->writeIntFieldFromTable(calibrationGroup, "dasid");
  else
    g_log.information("Not writing out values for \"dasid\"");

  this->writeIntFieldFromSVWS(calibrationGroup, "group", groupingWS);
  this->writeIntFieldFromSVWS(calibrationGroup, "use", maskWS);

  if (this->tableHasColumn("offset")) // optional field
    this->writeDoubleFieldFromTable(calibrationGroup, "offset");
  else
    g_log.information("Not writing out values for \"offset\"");

  // get the instrument information
  std::string instrumentName;
  std::string instrumentSource;
  if (bool(groupingWS)) {
    instrumentName = groupingWS->getInstrument()->getName();
    instrumentSource = groupingWS->getInstrument()->getFilename();
  }
  if (bool(maskWS)) {
    if (instrumentName.empty()) {
      instrumentName = maskWS->getInstrument()->getName();
    }
    if (instrumentSource.empty()) {
      instrumentSource = maskWS->getInstrument()->getFilename();
    }
  }
  if (!instrumentSource.empty()) {
    instrumentSource = Poco::Path(instrumentSource).getFileName();
  }

  // add the instrument information
  auto instrumentGroup = calibrationGroup.createGroup("instrument");
  writeStrAttribute(instrumentGroup, "NX_class", "NXinstrument");
  if (!instrumentName.empty()) {
    writeArray(instrumentGroup, "name", instrumentName);
  }
  if (!instrumentSource.empty()) {
    writeArray(instrumentGroup, "instrument_source", instrumentSource);
  }

  file.close();
}

} // namespace DataHandling
} // namespace Mantid
