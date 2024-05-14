// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveDiffCal.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"

#include <H5Cpp.h>
#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid::DataHandling {

using Mantid::API::FileProperty;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_const_sptr;
using Mantid::API::PropertyMode;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::GroupingWorkspace;
using Mantid::DataObjects::GroupingWorkspace_const_sptr;
using Mantid::DataObjects::GroupingWorkspace_sptr;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::MaskWorkspace_const_sptr;
using Mantid::Kernel::Direction;

using namespace H5;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveDiffCal)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SaveDiffCal::name() const { return "SaveDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveDiffCal::category() const { return "DataHandling\\Instrument;Diffraction\\DataHandling"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SaveDiffCal::summary() const { return "Saves a calibration file for powder diffraction"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveDiffCal::init() {
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("CalibrationWorkspace", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "An output workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>("GroupingWorkspace", "", Direction::Input,
                                                                         PropertyMode::Optional),
                  "Optional: A GroupingWorkspace giving the grouping info.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MaskWorkspace>>("MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: A MaskWorkspace giving which detectors are masked.");

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, ".h5"),
                  "Path to the .h5 file that will be created.");
}

std::map<std::string, std::string> SaveDiffCal::validateInputs() {
  std::map<std::string, std::string> result;

  ITableWorkspace_const_sptr calibrationWS = getProperty("CalibrationWorkspace");
  if (bool(calibrationWS)) {
    size_t numRows = calibrationWS->rowCount();
    if (numRows == 0) {
      result["CalibrationWorkspace"] = "Cannot save empty table";
    } else {
      GroupingWorkspace_const_sptr groupingWS = getProperty("GroupingWorkspace");
      if (bool(groupingWS) && numRows < groupingWS->getNumberHistograms()) {
        result["GroupingWorkspace"] = "Must have equal or less number of spectra as the table has rows";
      }
      MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace");
      if (bool(maskWS) && numRows < maskWS->getNumberHistograms()) {
        result["MaskWorkspace"] = "Must have equal or less number of spectra as the table has rows";
      }
    }
  } else {
    GroupingWorkspace_const_sptr groupingWS = getProperty("GroupingWorkspace");
    MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace");

    if ((!groupingWS) && (!maskWS)) {
      const std::string msg("Failed to supply any input workspace");
      result["CalibrationWorkspace"] = msg;
      result["GroupingWorkspace"] = msg;
      result["MaskWorkspace"] = msg;
    }
  }

  return result;
}

void SaveDiffCal::writeDoubleFieldZeros(H5::Group &group, const std::string &name) {
  std::vector<double> zeros(m_numValues, 0.);
  H5Util::writeArray1D(group, name, zeros);
}

/**
 * Create a dataset under a given group with a given name
 * Use CalibrationWorkspace to retrieve the data
 *
 * @param group :: group parent to the dataset
 * @param name :: column name of CalibrationWorkspace, and name of the dataset
 */
void SaveDiffCal::writeDoubleFieldFromTable(H5::Group &group, const std::string &name) {
  auto column = m_calibrationWS->getColumn(name);
  // Retrieve only the first m_numValues, not necessarily the whole column
  auto data = column->numeric_fill<>(m_numValues);

  // if the field is optional, check if it is all zeros
  if (name != "difc") {
    bool allZeros = std::all_of(data.cbegin(), data.cend(), [](const auto &value) { return value == 0; });
    if (allZeros)
      return; // don't write the field
  }

  H5Util::writeArray1D(group, name, data);
}

/**
 * Create a dataset under a given group with a given name
 * Use CalibrationWorkspace to retrieve the data.
 *
 * @param group :: group parent to the dataset
 * @param name :: column name of CalibrationWorkspace, and name of the dataset
 */
void SaveDiffCal::writeIntFieldFromTable(H5::Group &group, const std::string &name) {
  auto column = m_calibrationWS->getColumn(name);
  // Retrieve only the first m_numValues, not necessarily the whole column
  auto data = column->numeric_fill<int32_t>(m_numValues);
  H5Util::writeArray1D(group, name, data);
}

/*
 * Mantid::DataObjects::WorkspaceSingleValue/SpecialWorkspace2D is a parent to both GroupingWorkspace
 * and MaskWorkspace
 */
void SaveDiffCal::writeDetIdsfromSVWS(H5::Group &group, const std::string &name,
                                      const DataObjects::SpecialWorkspace2D_const_sptr &ws) {
  if (!bool(ws))
    throw std::runtime_error("Encountered null pointer in SaveDiffCal::writeDetIdsfromSVWS which should be impossible");

  std::vector<int32_t> values;
  for (size_t i = 0; i < m_numValues; ++i) {
    const auto &detids = ws->getSpectrum(i).getDetectorIDs();
    std::transform(detids.cbegin(), detids.cend(), std::back_inserter(values),
                   [](const auto &detid) { return static_cast<int32_t>(detid); });
  }

  H5Util::writeArray1D(group, name, values);
}

/**
 * Create a dataset under a given group with a given name
 * Use GroupingWorkspace or MaskWorkspace to retrieve the data.
 *
 * Mantid::DataObjects::WorkspaceSingleValue/SpecialWorkspace2D is a parent to both GroupingWorkspace
 * and MaskWorkspace
 *
 * @param group :: group parent to the dataset
 * @param name :: column name of the workspace, and name of the dataset
 * @param ws :: pointer to GroupingWorkspace or MaskWorkspace
 */
void SaveDiffCal::writeIntFieldFromSVWS(H5::Group &group, const std::string &name,
                                        const DataObjects::SpecialWorkspace2D_const_sptr &ws) {
  const bool isMask = (name == "use");

  // output array defaults to all one (one group, use the pixel)
  std::vector<int32_t> values(m_numValues, 1);

  if (bool(ws)) {
    for (size_t i = 0; i < m_numValues; ++i) {
      auto &ids = ws->getSpectrum(i).getDetectorIDs(); // set of detector ID's
      // check if the first detector ID in the set is in the calibration table
      auto found = m_detidToIndex.find(*(ids.begin())); // (detID, row_index)
      if (found != m_detidToIndex.end()) {
        auto value = static_cast<int32_t>(ws->getValue(found->first));
        // in maskworkspace 0=use, 1=dontuse - backwards from the file
        if (isMask) {
          if (value == 0)
            value = 1; // thus "use" means a calibrated detector, good for use
          else
            value = 0;
        }
        values[found->second] = value;
      }
    }
  }

  H5Util::writeArray1D(group, name, values);
}

void SaveDiffCal::generateDetidToIndex() {
  m_detidToIndex.clear();

  auto detidCol = m_calibrationWS->getColumn("detid");
  auto detids = detidCol->numeric_fill<detid_t>();

  const size_t numDets = detids.size();
  for (size_t i = 0; i < numDets; ++i) {
    m_detidToIndex[static_cast<detid_t>(detids[i])] = i;
  }
}

void SaveDiffCal::generateDetidToIndex(const DataObjects::SpecialWorkspace2D_const_sptr &ws) {
  if (!bool(ws))
    throw std::runtime_error(
        "Encountered null pointer in SaveDiffCal::generateDetidToIndex which should be impossible");

  m_detidToIndex.clear();

  std::size_t index = 0;
  for (size_t i = 0; i < m_numValues; ++i) {
    const auto &detids = ws->getSpectrum(i).getDetectorIDs();
    for (const auto &detid : detids) {
      m_detidToIndex[static_cast<detid_t>(detid)] = index;
      index++;
    }
  }
}

bool SaveDiffCal::tableHasColumn(const std::string &ColumnName) const {
  if (m_calibrationWS) {
    const std::vector<std::string> names = m_calibrationWS->getColumnNames();
    return std::any_of(names.cbegin(), names.cend(), [&ColumnName](const auto &name) { return name == ColumnName; });
  } else {
    return false;
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveDiffCal::exec() {
  m_calibrationWS = getProperty("CalibrationWorkspace");
  GroupingWorkspace_sptr groupingWS = getProperty("GroupingWorkspace");
  MaskWorkspace_const_sptr maskWS = getProperty("MaskWorkspace");

  // Get a starting number of values to work with that will be refined below
  // THE ORDER OF THE IF/ELSE TREE MATTERS
  m_numValues = std::numeric_limits<std::size_t>::max();
  if (m_calibrationWS)
    m_numValues = std::min(m_numValues, m_calibrationWS->rowCount());
  if (groupingWS)
    m_numValues = std::min(m_numValues, groupingWS->getNumberHistograms());
  if (maskWS)
    m_numValues = std::min(m_numValues, maskWS->getNumberHistograms());

  // Initialize the mapping of detid to row number to make getting information
  // from the table faster. ORDER MATTERS
  if (m_calibrationWS) {
    this->generateDetidToIndex();
  } else if (groupingWS) {
    this->generateDetidToIndex(groupingWS);
  } else if (maskWS) {
    this->generateDetidToIndex(maskWS);
  }

  if (groupingWS && groupingWS->isDetectorIDMappingEmpty())
    groupingWS->buildDetectorIDMapping();

  // delete the file if it already exists
  std::string filename = getProperty("Filename");
  if (Poco::File(filename).exists()) {
    Poco::File(filename).remove();
  }

  H5File file(filename, H5F_ACC_EXCL);

  auto calibrationGroup = H5Util::createGroupNXS(file, "calibration", "NXentry");

  // write the d-spacing to TOF conversion parameters for the selected pixels
  // as datasets under the NXentry group
  if (m_calibrationWS) {
    this->writeDoubleFieldFromTable(calibrationGroup, "difc");
    this->writeDoubleFieldFromTable(calibrationGroup, "difa");
    this->writeDoubleFieldFromTable(calibrationGroup, "tzero");
  } else {
    writeDoubleFieldZeros(calibrationGroup, "difc");
    // LoadDiffCal will set difa and tzero to zero if they are missing
  }

  // add the detid from which ever of these exists
  if (m_calibrationWS) {
    this->writeIntFieldFromTable(calibrationGroup, "detid");
  } else if (groupingWS) {
    this->writeDetIdsfromSVWS(calibrationGroup, "detid", groupingWS);
  } else if (maskWS) {
    this->writeDetIdsfromSVWS(calibrationGroup, "detid", maskWS);
  }

  // the dasid is a legacy column that is not used by mantid but should be written if it exists
  if (this->tableHasColumn("dasid")) // optional field
    this->writeIntFieldFromTable(calibrationGroup, "dasid");
  else
    g_log.information("Not writing out values for \"dasid\"");

  this->writeIntFieldFromSVWS(calibrationGroup, "group", groupingWS);
  this->writeIntFieldFromSVWS(calibrationGroup, "use", maskWS);

  // check if the input calibration table has an "offset" field
  if (this->tableHasColumn("offset")) // optional field
    this->writeDoubleFieldFromTable(calibrationGroup, "offset");
  else
    g_log.information("Not writing out values for \"offset\"");

  // get the instrument information only if a GroupingWorkspace or
  // MaskWorkspace is supplied by the user
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
  H5Util::writeStrAttribute(instrumentGroup, "NX_class", "NXinstrument");
  if (!instrumentName.empty()) {
    H5Util::write(instrumentGroup, "name", instrumentName);
  }
  if (!instrumentSource.empty()) {
    H5Util::write(instrumentGroup, "instrument_source", instrumentSource);
  }

  file.close();
}

} // namespace Mantid::DataHandling
