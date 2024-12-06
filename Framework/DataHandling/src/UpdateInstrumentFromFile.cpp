// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/UpdateInstrumentFromFile.h"
#include "LoadRaw/isisraw2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadISISNexus2.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidKernel/StringTokenizer.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/scoped_ptr.hpp>
// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

#include <algorithm>
#include <fstream>

namespace Mantid::DataHandling {

DECLARE_ALGORITHM(UpdateInstrumentFromFile)

using namespace Kernel;
using namespace API;
using Geometry::IDetector_sptr;
using Geometry::Instrument_sptr;
using Kernel::V3D;

/// Empty default constructor
UpdateInstrumentFromFile::UpdateInstrumentFromFile() : m_workspace(), m_ignorePhi(false), m_ignoreMonitors(true) {}

/// Initialisation method.
void UpdateInstrumentFromFile::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace in which to store the imported instrument");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load,
                                                 std::vector<std::string>{".raw", ".nxs", ".dat", ".s*"}),
                  "The filename of the input file.\n"
                  "Currently supports RAW, ISIS NeXus, DAT & multi-column (at "
                  "least 2) ascii files");
  declareProperty("MoveMonitors", (!m_ignoreMonitors),
                  "If true the positions of any detectors marked as monitors "
                  "in the IDF will be moved also");
  declareProperty("IgnorePhi", m_ignorePhi, "If true the phi values form the file will be ignored ");
  declareProperty("AsciiHeader", "",
                  "If the file is a simple text file, then this property is used to"
                  "define the values in each column of the file. For example: "
                  "spectrum,theta,t0,-,R"
                  "Keywords=spectrum,ID,R,theta,phi. A dash means skip column. Keywords "
                  "are recognised"
                  "as identifying components to move to new positions. Any other names in "
                  "the list"
                  "are added as instrument parameters.");
  declareProperty("SkipFirstNLines", 0,
                  "If the file is ASCII, then skip this "
                  "number of lines at the start of the "
                  "file");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void UpdateInstrumentFromFile::exec() {
  // Retrieve the filename from the properties
  const std::string filename = getPropertyValue("Filename");
  m_workspace = getProperty("Workspace");

  if (!m_workspace->getInstrument()) {
    throw std::runtime_error("Input workspace has no defined instrument");
  }

  m_ignorePhi = getProperty("IgnorePhi");
  const bool moveMonitors = getProperty("MoveMonitors");
  m_ignoreMonitors = (!moveMonitors);

  // Check file type
  if (NexusDescriptor::isReadable(filename)) {
    LoadISISNexus2 isisNexus;
    LoadEventNexus eventNexus;

    // we open and close the HDF5 file.
    // there is an issue with how HDF5 files are opened (only one at a time)
    // swap the order of descriptors
    boost::scoped_ptr<Kernel::NexusHDF5Descriptor> descriptorNexusHDF5(new Kernel::NexusHDF5Descriptor(filename));

    boost::scoped_ptr<Kernel::NexusDescriptor> descriptor(new Kernel::NexusDescriptor(filename));

    if (isisNexus.confidence(*descriptor) > 0 || eventNexus.confidence(*descriptorNexusHDF5) > 0) {
      auto &nxFile = descriptor->data();
      const auto &rootEntry = descriptor->firstEntryNameType();
      nxFile.openGroup(rootEntry.first, rootEntry.second);
      updateFromNeXus(nxFile);
      return;
    }
  }

  if (FileDescriptor::isAscii(filename)) {
    // If no header specified & the extension is .dat or .sca, then assume ISIS
    // DAT file structure
    if (getPropertyValue("AsciiHeader").empty() &&
        (boost::iends_with(filename, ".dat") || boost::iends_with(filename, ".sca"))) {
      this->setPropertyValue("AsciiHeader", "ID,-,R,-,theta,phi,-,-,-,-,-,-,-,-,-,-,-,-,-");
      this->setProperty("SkipFirstNLines", 2);
    }
    updateFromAscii(filename);
    return;
  }

  LoadRawHelper isisRAW;
  auto descriptor = std::make_unique<Kernel::FileDescriptor>(filename);
  if (isisRAW.confidence(*descriptor) > 0) {
    updateFromRaw(filename);
  } else {
    throw std::invalid_argument("File \"" + filename + "\" is not a valid input file.");
  }
}

/**
 * Update the detector information from a raw file
 * @param filename :: The input filename
 */
void UpdateInstrumentFromFile::updateFromRaw(const std::string &filename) {
  ISISRAW2 iraw;
  if (iraw.readFromFile(filename.c_str(), false) != 0) {
    g_log.error("Unable to open file " + filename);
    throw Exception::FileError("Unable to open File:", filename);
  }

  const int32_t numDetector = iraw.i_det;
  std::vector<int32_t> detID(iraw.udet, iraw.udet + numDetector);
  std::vector<float> l2(iraw.len2, iraw.len2 + numDetector);
  std::vector<float> theta(iraw.tthe, iraw.tthe + numDetector);
  // Is ut01 (=phi) present? Sometimes an array is present but has wrong values
  // e.g.all 1.0 or all 2.0
  bool phiPresent = iraw.i_use > 0 && iraw.ut[0] != 1.0 && iraw.ut[0] != 2.0;
  std::vector<float> phi(0);
  if (phiPresent) {
    phi = std::vector<float>(iraw.ut, iraw.ut + numDetector);
  } else {
    phi = std::vector<float>(numDetector, 0.0);
  }
  g_log.information() << "Setting detector postions from RAW file.\n";
  setDetectorPositions(detID, l2, theta, phi);
}

/**
 * Update the detector information from a NeXus file
 * @param nxFile :: Handle to a NeXus file where the root group has been opened
 */
void UpdateInstrumentFromFile::updateFromNeXus(::NeXus::File &nxFile) {
  try {
    nxFile.openGroup("isis_vms_compat", "IXvms");
  } catch (::NeXus::Exception &) {
    throw std::runtime_error("Unknown NeXus flavour. Cannot update instrument "
                             "positions using this type of file");
  }
  // Det ID
  std::vector<int32_t> detID;
  nxFile.openData("UDET");
  nxFile.getData(detID);
  nxFile.closeData();
  // Position information
  std::vector<float> l2, theta, phi;
  nxFile.openData("LEN2");
  nxFile.getData(l2);
  nxFile.closeData();
  nxFile.openData("TTHE");
  nxFile.getData(theta);
  nxFile.closeData();
  nxFile.openData("UT01");
  nxFile.getData(phi);
  nxFile.closeData();

  g_log.information() << "Setting detector postions from NeXus file.\n";
  setDetectorPositions(detID, l2, theta, phi);
}

/**
 * Updates from a more generic ascii file
 * @param filename :: The input filename
 */
void UpdateInstrumentFromFile::updateFromAscii(const std::string &filename) {
  AsciiFileHeader header;
  const bool isSpectrum = parseAsciiHeader(header);

  // Throws for multiple detectors
  const spec2index_map specToIndex(m_workspace->getSpectrumToWorkspaceIndexMap());

  std::ifstream datfile(filename.c_str(), std::ios_base::in);
  const int skipNLines = getProperty("SkipFirstNLines");
  std::string line;
  int lineCount(0);
  while (lineCount < skipNLines) {
    std::getline(datfile, line);
    ++lineCount;
  }

  Geometry::ParameterMap &pmap = m_workspace->instrumentParameters();
  auto &detectorInfo = m_workspace->mutableDetectorInfo();
  const auto &spectrumInfo = m_workspace->spectrumInfo();

  std::vector<double> colValues(header.colCount - 1, 0.0);
  while (std::getline(datfile, line)) {
    boost::trim(line);
    std::istringstream is(line);
    // Column 0 should be ID/spectrum number
    int32_t detOrSpec(-1000);
    is >> detOrSpec;
    // If first thing read is not a number then skip the line
    if (is.fail()) {
      g_log.debug() << "Skipping \"" << line << "\". Cannot interpret as list of numbers.\n";
      continue;
    }

    bool skip{false};
    auto index = static_cast<size_t>(-1);
    const Geometry::IDetector *det{nullptr};
    if (isSpectrum) {
      auto it = specToIndex.find(detOrSpec);
      if (it != specToIndex.end()) {
        index = it->second;
        if (spectrumInfo.hasDetectors(index)) {
          det = &spectrumInfo.detector(index);
        } else {
          skip = true;
        }
      } else {
        g_log.debug() << "Skipping \"" << line << "\". Spectrum is not in workspace.\n";
        continue;
      }
    } else {
      try {
        index = detectorInfo.indexOf(detOrSpec);
        det = &detectorInfo.detector(index);
      } catch (std::out_of_range &) {
        skip = true;
      }
    }
    if (skip || index == static_cast<size_t>(-1)) {
      g_log.debug() << "Skipping \"" << line << "\". Spectrum in workspace but cannot find associated detector.\n";
      continue;
    }

    std::vector<size_t> indices;
    if (isSpectrum) {
      if (auto group = dynamic_cast<const Geometry::DetectorGroup *>(det)) {
        const auto detIDs = group->getDetectorIDs();
        std::transform(detIDs.cbegin(), detIDs.cend(), std::back_inserter(indices),
                       [detectorInfo](const auto detID) { return detectorInfo.indexOf(detID); });
      } else {
        indices.emplace_back(detectorInfo.indexOf(det->getID()));
      }
    } else {
      indices.emplace_back(index);
    }

    // Special cases for detector r,t,p. Everything else is
    // attached as an detector parameter
    double R(0.0), theta(0.0), phi(0.0);
    for (size_t i = 1; i < header.colCount; ++i) {
      double value(0.0);
      is >> value;
      if (i < header.colCount - 1 && is.eof()) {
        // If stringstream is at EOF & we are not at the last column then
        // there aren't enought columns in the file
        throw std::runtime_error("UpdateInstrumentFromFile::updateFromAscii - "
                                 "File contains fewer than expected number of "
                                 "columns, check AsciiHeader property.");
      }

      if (i == header.rColIdx)
        R = value;
      else if (i == header.thetaColIdx)
        theta = value;
      else if (i == header.phiColIdx)
        phi = value;
      else if (header.detParCols.count(i) == 1) {
        for (const auto detIndex : indices) {
          auto id = detectorInfo.detector(detIndex).getComponentID();
          pmap.addDouble(id, header.colToName[i], value);
        }
      }
    }
    // Check stream state. stringstream::EOF should have been reached, if not
    // then there is still more to
    // read and the file has more columns than the header indicated
    if (!is.eof()) {
      throw std::runtime_error("UpdateInstrumentFromFile::updateFromAscii - "
                               "File contains more than expected number of "
                               "columns, check AsciiHeader property.");
    }

    // If not supplied use current values
    double r, t, p;
    if (isSpectrum)
      spectrumInfo.position(index).getSpherical(r, t, p);
    else
      detectorInfo.position(index).getSpherical(r, t, p);
    if (header.rColIdx == 0)
      R = r;
    if (header.thetaColIdx == 0)
      theta = t;
    if (header.phiColIdx == 0 || m_ignorePhi)
      phi = p;

    for (const auto detIndex : indices)
      setDetectorPosition(detectorInfo, detIndex, static_cast<float>(R), static_cast<float>(theta),
                          static_cast<float>(phi));
  }
}

/**
 * Parse the header and fill the headerInfo struct and returns a boolean
 * indicating if the table is spectrum or detector ID based
 * @param headerInfo :: [Out] Fills the given struct with details about the
 * header
 * @returns True if the header is spectrum based, false otherwise
 */
bool UpdateInstrumentFromFile::parseAsciiHeader(UpdateInstrumentFromFile::AsciiFileHeader &headerInfo) {
  const std::string header = getProperty("AsciiHeader");
  if (header.empty()) {
    throw std::invalid_argument("Ascii file provided but the AsciiHeader "
                                "property is empty, cannot interpret columns");
  }

  Mantid::Kernel::StringTokenizer splitter(header, ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  headerInfo.colCount = splitter.count();
  auto it = splitter.begin(); // First column must be spectrum number or detector ID
  const std::string &col0 = *it;
  bool isSpectrum(false);
  if (boost::iequals("spectrum", col0))
    isSpectrum = true;
  if (!isSpectrum && !boost::iequals("id", col0)) {
    throw std::invalid_argument("Invalid AsciiHeader, first column name must "
                                "be either 'spectrum' or 'id'");
  }

  ++it;
  size_t counter(1);
  for (; it != splitter.end(); ++it) {
    const std::string &colName = *it;
    if (boost::iequals("R", colName))
      headerInfo.rColIdx = counter;
    else if (boost::iequals("theta", colName))
      headerInfo.thetaColIdx = counter;
    else if (boost::iequals("phi", colName))
      headerInfo.phiColIdx = counter;
    else if (boost::iequals("-", colName)) // Skip dashed
    {
      ++counter;
      continue;
    } else {
      headerInfo.detParCols.insert(counter);
      headerInfo.colToName.emplace(counter, colName);
    }
    ++counter;
  }

  return isSpectrum;
}

/**
 * Set the detector positions given the r,theta and phi.
 * @param detID :: A vector of detector IDs
 * @param l2 :: A vector of l2 distances
 * @param theta :: A vector of theta distances
 * @param phi :: A vector of phi values
 */
void UpdateInstrumentFromFile::setDetectorPositions(const std::vector<int32_t> &detID, const std::vector<float> &l2,
                                                    const std::vector<float> &theta, const std::vector<float> &phi) {
  const auto numDetector = static_cast<int>(detID.size());
  g_log.information() << "Setting new positions for " << numDetector << " detectors\n";

  auto &detectorInfo = m_workspace->mutableDetectorInfo();
  for (int i = 0; i < numDetector; ++i) {
    try {
      auto index = detectorInfo.indexOf(detID[i]);
      double p{phi[i]};
      if (m_ignorePhi) {
        double r, t;
        detectorInfo.position(index).getSpherical(r, t, p);
      }
      setDetectorPosition(detectorInfo, index, l2[i], theta[i], static_cast<float>(p));
    } catch (std::out_of_range &) {
      // Invalid detID[i]
      continue;
    }
    progress(static_cast<double>(i) / numDetector, "Updating Detector Positions from File");
  }
}

/**
 * Set the new detector position given the r,theta and phi.
 * @param detectorInfo :: Reference to the DetectorInfo
 * @param index :: Index into detectorInfo
 * @param l2 :: A single l2
 * @param theta :: A single theta
 * @param phi :: A single phi
 */
void UpdateInstrumentFromFile::setDetectorPosition(Geometry::DetectorInfo &detectorInfo, const size_t index,
                                                   const float l2, const float theta, const float phi) {
  if (m_ignoreMonitors && detectorInfo.isMonitor(index))
    return;

  Kernel::V3D pos;
  pos.spherical(l2, theta, phi);
  detectorInfo.setPosition(index, pos);
}

} // namespace Mantid::DataHandling
