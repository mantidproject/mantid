// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadDspacemap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDspacemap)

LoadDspacemap::LoadDspacemap() { this->deprecatedDate("2024-9-17"); }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadDspacemap::init() {
  // 3 properties for getting the right instrument
  LoadCalFile::getInstrument3WaysInit(this);

  const std::vector<std::string> exts{".dat", ".bin"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The DspacemapFile containing the d-space mapping.");

  std::vector<std::string> propOptions{"POWGEN", "VULCAN-ASCII", "VULCAN-Binary"};
  declareProperty("FileType", "POWGEN", std::make_shared<StringListValidator>(propOptions),
                  "The type of file being read.");

  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output OffsetsWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadDspacemap::exec() {
  // Get the instrument
  Instrument_const_sptr inst = LoadCalFile::getInstrument3Ways(this);

  // Read in the calibration data
  const std::string DFileName = getProperty("Filename");

  // Create the blank output
  OffsetsWorkspace_sptr offsetsWS(new OffsetsWorkspace(inst));
  setProperty("OutputWorkspace", offsetsWS);

  std::string type = this->getPropertyValue("FileType");
  if (type == "POWGEN") {
    // generate map of the tof->d conversion factors
    CalculateOffsetsFromDSpacemapFile(DFileName, offsetsWS);
  } else {
    // Map of udet:funny vulcan correction factor.
    std::map<detid_t, double> vulcan;
    if (type == "VULCAN-ASCII") {
      readVulcanAsciiFile(DFileName, vulcan);
    } else if (type == "VULCAN-Binary") {
      readVulcanBinaryFile(DFileName, vulcan);
    } else
      throw std::invalid_argument("Unexpected FileType property value received.");

    // Now that we have loaded the vulcan file (either type), convert it out.
    this->CalculateOffsetsFromVulcanFactors(vulcan, offsetsWS);
  }
}

//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 *
 * @param DFileName :: name of dspacemap file
 * @param offsetsWS :: OffsetsWorkspace to be filled.
 */
void LoadDspacemap::CalculateOffsetsFromDSpacemapFile(const std::string &DFileName,
                                                      const Mantid::DataObjects::OffsetsWorkspace_sptr &offsetsWS) {
  // Get a pointer to the instrument contained in the workspace

  const auto &detectorInfo = offsetsWS->detectorInfo();
  const double l1 = detectorInfo.l1();
  // Read in the POWGEN-style Dspace mapping file
  const char *filename = DFileName.c_str();
  std::ifstream fin(filename, std::ios_base::in | std::ios_base::binary);

  std::vector<double> dspace;
  double read;
  while (!fin.eof()) {
    fin.read(reinterpret_cast<char *>(&read), sizeof read);
    // Factor of 10 between ISAW and Mantid
    read *= 10.;
    dspace.emplace_back(read);
  }

  const auto &detectorIds = detectorInfo.detectorIDs();

  for (size_t detectorIndex = 0; detectorIndex < detectorInfo.size(); ++detectorIndex) {
    const auto detectorId = detectorIds[detectorIndex];

    // Compute the factor
    double offset = 0.0;
    if (!detectorInfo.isMonitor(detectorIndex)) {
      double factor = Geometry::Conversion::tofToDSpacingFactor(l1, detectorInfo.l2(detectorIndex),
                                                                detectorInfo.twoTheta(detectorIndex), offset);
      offset = dspace[detectorId] / factor - 1.0;
    }
    // Save in the map
    try {
      offsetsWS->setValue(detectorId, offset);
    } catch (std::invalid_argument &) {
    }
  }
}

const double CONSTANT = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

//-----------------------------------------------------------------------
/**
 * Make a map of the conversion factors between tof and D-spacing
 * for all pixel IDs in a workspace.
 * map vulcan should contain the module/module and stack/stack offset
 *
 * @param vulcan :: map between detector ID and vulcan correction factor.
 * @param offsetsWS :: OffsetsWorkspace to be filled.
 */
void LoadDspacemap::CalculateOffsetsFromVulcanFactors(std::map<detid_t, double> &vulcan,
                                                      const Mantid::DataObjects::OffsetsWorkspace_sptr &offsetsWS) {
  // Get a pointer to the instrument contained in the workspace
  // At this point, instrument VULCAN has been created?
  Instrument_const_sptr instrument = offsetsWS->getInstrument();

  g_log.notice() << "Name of instrument = " << instrument->getName() << '\n';
  g_log.notice() << "Input map (dict):  size = " << vulcan.size() << '\n';

  // To get all the detector ID's
  detid2det_map allDetectors;
  instrument->getDetectors(allDetectors);

  detid2det_map::const_iterator it;
  int numfinds = 0;
  g_log.notice() << "Input number of detectors = " << allDetectors.size() << '\n';

  // Get detector information
  double l1, beamline_norm;
  Kernel::V3D beamline, samplePos;
  instrument->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

  /*** A survey of parent detector
  std::map<detid_t, bool> parents;
  for (it = allDetectors.begin(); it != allDetectors.end(); it++){
    int32_t detid = it->first;

    // def std::shared_ptr<const Mantid::Geometry::IDetector>
  IDetector_const_sptr;

    std::string parentname =
  it->second->getParent()->getComponentID()->getName();
    g_log.notice() << "Name = " << parentname << '\n';
    // parents.insert(parentid, true);
  }
  ***/

  /*** Here some special configuration for VULCAN is hard-coded here!
   *   Including (1) Super-Parent Information
   ***/
  Kernel::V3D referencePos;
  detid_t anydetinrefmodule = 21 * 1250 + 5;

  auto det_iter = allDetectors.find(anydetinrefmodule);

  if (det_iter == allDetectors.end()) {
    throw std::invalid_argument("Any Detector ID is Instrument's detector");
  }
  referencePos = det_iter->second->getParent()->getPos();
  double refl2 = referencePos.norm();
  double halfcosTwoThetaRef = referencePos.scalar_prod(beamline) / (refl2 * beamline_norm);
  double sinThetaRef = sqrt(0.5 - halfcosTwoThetaRef);
  double difcRef = sinThetaRef * (l1 + refl2) / CONSTANT;

  // Loop over all detectors in instrument to find the offset
  for (it = allDetectors.begin(); it != allDetectors.end(); ++it) {
    int detectorID = it->first;
    Geometry::IDetector_const_sptr det = it->second;
    double offset = 0.0;

    // Find the vulcan factor;
    double vulcan_factor = 0.0;
    std::map<detid_t, double>::const_iterator vulcan_iter = vulcan.find(detectorID);
    if (vulcan_iter != vulcan.end()) {
      vulcan_factor = vulcan_iter->second;
      numfinds++;
    }

    // g_log.notice() << "Selected Detector with ID = " << detectorID << "  ID2
    // = " << id2 << '\n'; proved to be same

    double intermoduleoffset = 0;
    double interstackoffset = 0;

    detid_t intermoduleid = detid_t(detectorID / 1250) * 1250 + 1250 - 2;
    vulcan_iter = vulcan.find(intermoduleid);
    if (vulcan_iter == vulcan.end()) {
      g_log.error() << "Cannot find inter-module offset ID = " << intermoduleid << '\n';
    } else {
      intermoduleoffset = vulcan_iter->second;
    }

    detid_t interstackid = detid_t(detectorID / 1250) * 1250 + 1250 - 1;
    vulcan_iter = vulcan.find(interstackid);
    if (vulcan_iter == vulcan.end()) {
      g_log.error() << "Cannot find inter-module offset ID = " << intermoduleid << '\n';
    } else {
      interstackoffset = vulcan_iter->second;
    }

    /***  This is the previous way to correct upon DIFC[module center pixel]
    // The actual factor is 10^(-value_in_the_file)
    vulcan_factor = pow(10.0,-vulcan_factor);
    // At this point, tof_corrected = vulcan_factor * tof_input
    // So this is the offset
    offset = vulcan_factor - 1.0;
    ***/

    /*** New approach to correct based on DIFC of each pixel
     *   Equation:  offset = DIFC^(pixel)/DIFC^(parent)*(1+vulcan_offset)-1
     *   offset should be close to 0
     ***/
    // 1. calculate DIFC
    Kernel::V3D detPos;
    detPos = det->getPos();

    // Now detPos will be set with respect to samplePos
    detPos -= samplePos;
    double l2 = detPos.norm();
    double halfcosTwoTheta = detPos.scalar_prod(beamline) / (l2 * beamline_norm);
    double sinTheta = sqrt(0.5 - halfcosTwoTheta);
    double difc_pixel = sinTheta * (l1 + l2) / CONSTANT;

    // Kernel::V3D parentPos = det->getParent()->getPos();
    // parentPos -= samplePos;
    // double l2parent = parentPos.norm();
    // double halfcosTwoThetaParent = parentPos.scalar_prod(beamline)/(l2 *
    // beamline_norm);
    // double sinThetaParent = sqrt(0.5 - halfcosTwoThetaParent);
    // double difc_parent = sinThetaParent*(l1+l2parent)/CONSTANT;

    /*** Offset Replicate Previous Result
    offset = difc_pixel/difc_parent*(pow(10.0, -vulcan_factor))-1.0;
    ***/

    offset = difc_pixel / difcRef * (pow(10.0, -(vulcan_factor + intermoduleoffset + interstackoffset))) - 1.0;

    // Save in the map
    try {
      offsetsWS->setValue(detectorID, offset);

      if (intermoduleid != 27498 && intermoduleid != 28748 && intermoduleid != 29998 && intermoduleid != 33748 &&
          intermoduleid != 34998 && intermoduleid != 36248) {
        g_log.error() << "Detector ID = " << detectorID << "  Inter-Module ID = " << intermoduleid << '\n';
        throw std::invalid_argument("Indexing error!");
      }

    } catch (std::invalid_argument &) {
      g_log.notice() << "Misses Detector ID = " << detectorID << '\n';
    }
  } // for

  g_log.notice() << "Number of matched detectors =" << numfinds << '\n';
}

//-----------------------------------------------------------------------
/** Reads an ASCII VULCAN filename where:
 *
 * 1st column : pixel ID
 * 2nd column : float "correction",  where corrected_TOF = initial_TOF /
 *10^correction
 *
 * @param fileName :: vulcan file name
 * @param[out] vulcan :: a map of pixel ID : correction factor in the file (2nd
 *column)
 */
void LoadDspacemap::readVulcanAsciiFile(const std::string &fileName, std::map<detid_t, double> &vulcan) {
  std::ifstream grFile(fileName.c_str());
  if (!grFile) {
    g_log.error() << "Unable to open vulcan file " << fileName << '\n';
    return;
  }
  vulcan.clear();
  std::string str;
  int numentries = 0;
  while (getline(grFile, str)) {
    if (str.empty() || str[0] == '#')
      continue;
    std::istringstream istr(str);
    int32_t udet;
    double correction;
    istr >> udet >> correction;
    vulcan.emplace(udet, correction);
    numentries++;
  }

  g_log.notice() << "Read Vulcan ASCII File:  # Entry = " << numentries << '\n';
}

/** Structure of the vulcan binary file */
struct VulcanCorrectionFactor {
  /// ID for pixel
  double pixelID;
  /// Correction factor for pixel
  double factor;
};

//-----------------------------------------------------------------------
/** Reads a Binary VULCAN filename where:
 *
 * 1st 8 bytes : pixel ID
 * 2nd 8 bytes : double "correction",  where corrected_TOF = initial_TOF /
 *10^correction
 *
 * @param fileName :: vulcan file name
 * @param[out] vulcan :: a map of pixel ID : correction factor in the file (2nd
 *column)
 */
void LoadDspacemap::readVulcanBinaryFile(const std::string &fileName, std::map<detid_t, double> &vulcan) {
  BinaryFile<VulcanCorrectionFactor> file(fileName);
  std::vector<VulcanCorrectionFactor> results = file.loadAll();
  for (const auto &result : results) {
    vulcan[static_cast<detid_t>(result.pixelID)] = result.factor;
  }
}

} // namespace Mantid::DataHandling
