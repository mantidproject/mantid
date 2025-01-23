// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadDetectorInfo.h"
#include "LoadRaw/isisraw2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <Poco/Path.h>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <fstream>

namespace Mantid::DataHandling {
using namespace Kernel;
using namespace API;
using namespace Geometry;

namespace {
// Name of the offset parameter
const char *DELAY_PARAM = "DelayTime";
// Name of pressure parameter
const char *PRESSURE_PARAM = "TubePressure";
// Name of wall thickness parameter
const char *THICKNESS_PARAM = "TubeThickness";
} // namespace

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadDetectorInfo)

/// Empty default constructor
LoadDetectorInfo::LoadDetectorInfo()
    : Algorithm(), m_baseInstrument(), m_samplePos(), m_moveDets(false), m_workspace(), m_instDelta(-1.0),
      m_instPressure(-1.0), m_instThickness(-1.0) {}

void LoadDetectorInfo::init() {

  declareProperty(std::make_unique<WorkspaceProperty<>>("Workspace", "", Direction::InOut),
                  "The name of the workspace to that the detector information "
                  "will be loaded into.");

  const std::vector<std::string> exts{".dat", ".raw", ".sca", ".nxs"};
  declareProperty(std::make_unique<FileProperty>("DataFilename", "", FileProperty::Load, exts),
                  "A **raw, dat, nxs** or **sca** file that contains information about the "
                  "detectors in the "
                  "workspace. The description of **dat** and **nxs** file format is "
                  "provided below.");

  declareProperty("RelocateDets", false,
                  "If true, the detectors are moved to "
                  "the positions specified in the file "
                  "defined by the field above.",
                  Direction::Input);
}

void LoadDetectorInfo::exec() {
  cacheInputs();
  std::string filename = getPropertyValue("DataFilename");
  std::string ext = Poco::Path(filename).getExtension();
  if (boost::iequals(ext, "dat") || boost::iequals(ext, "sca")) {
    loadFromDAT(filename);
  } else if (boost::iequals(ext, "raw")) {
    loadFromRAW(filename);
  } else if (boost::iequals(ext, "nxs")) {
    loadFromIsisNXS(filename);
  } else {
    throw std::invalid_argument("Unknown file type with extension=." + ext);
  }
}

/**
 * Cache frequently accessed user input
 */
void LoadDetectorInfo::cacheInputs() {
  m_workspace = getProperty("Workspace");
  m_moveDets = getProperty("RelocateDets");

  // Cache base instrument
  m_baseInstrument = m_workspace->getInstrument()->baseInstrument();
  Geometry::IComponent_const_sptr sample = m_workspace->getInstrument()->getSample();
  if (sample)
    m_samplePos = sample->getPos();

  // cache values of instrument level parameters so we only change then if they
  // are different
  const auto &pmap = m_workspace->constInstrumentParameters();
  // delay
  auto param = pmap.get(m_baseInstrument->getComponentID(), DELAY_PARAM);
  if (param)
    m_instDelta = param->value<double>();
  // pressure
  param = pmap.get(m_baseInstrument->getComponentID(), PRESSURE_PARAM);
  if (param)
    m_instPressure = param->value<double>();
  // thickness
  param = pmap.get(m_baseInstrument->getComponentID(), THICKNESS_PARAM);
  if (param)
    m_instThickness = param->value<double>();
}

/**
 * Full format is defined in doc text
 * @param filename A full path to the input DAT file
 */
void LoadDetectorInfo::loadFromDAT(const std::string &filename) {
  std::ifstream datFile(filename.c_str());
  if (!datFile) {
    throw Exception::FileError("Unable to access dat file", filename);
  }

  std::string line;
  // skip 3 lines of header info
  for (int i = 0; i < 3; ++i)
    getline(datFile, line);

  // start loop over file
  auto &pmap = m_workspace->instrumentParameters();
  auto &wsDetInfo = m_workspace->mutableDetectorInfo();
  while (getline(datFile, line)) {
    if (line.empty() || line[0] == '#')
      continue;

    std::istringstream is(line);
    detid_t detID(0);
    int code(0);
    float delta(0.0f), l2(0.0f), theta(0.0f), phi(0.0f);
    is >> detID >> delta >> l2 >> code >> theta >> phi;
    // offset value is be subtracted so store negative
    delta *= -1.0f;

    try {
      size_t index = wsDetInfo.indexOf(detID);
      if (wsDetInfo.isMonitor(index) || code == 1)
        continue;

      // drop 10 float columns
      for (int i = 0; i < 10; ++i) {
        float droppedFloat(0.0f);
        is >> droppedFloat;
      }

      // pressure, wall thickness
      float pressure(0.0), thickness(0.0);
      is >> pressure >> thickness;

      updateParameterMap(wsDetInfo, index, pmap, l2, theta, phi, delta, pressure, thickness);
    } catch (std::out_of_range &) {
      continue;
    }
  }
}

/**
 * @param filename A full path to the input RAW file
 */
void LoadDetectorInfo::loadFromRAW(const std::string &filename) {
  ISISRAW2 iraw;
  if (iraw.readFromFile(filename.c_str(), false) != 0) {
    throw Exception::FileError("Unable to access raw file:", filename);
  }

  const int numDets = iraw.i_det;
  const int numUserTables = iraw.i_use;
  int pressureTabNum(0), thicknessTabNum(0);
  if (numUserTables == 10) {
    pressureTabNum = 7;
    thicknessTabNum = 8;
  } else if (numUserTables == 14) {
    pressureTabNum = 11;
    thicknessTabNum = 12;
  } else {
    throw std::invalid_argument("RAW file contains unexpected number of user tables=" + std::to_string(numUserTables) +
                                ". Expected 10 or 14.");
  }

  // Is ut01 (=phi) present? Sometimes an array is present but has wrong values
  // e.g.all 1.0 or all 2.0
  bool phiPresent = (iraw.ut[0] != 1.0 && iraw.ut[0] != 2.0);

  // Start loop over detectors
  auto &pmap = m_workspace->instrumentParameters();
  auto &wsDetInfo = m_workspace->mutableDetectorInfo();
  for (int i = 0; i < numDets; ++i) {
    auto detID = static_cast<detid_t>(iraw.udet[i]);
    int code = iraw.code[i];
    try {
      size_t index = wsDetInfo.indexOf(detID);
      if (wsDetInfo.isMonitor(index) || code == 1)
        continue;

      // Positions
      float l2 = iraw.len2[i];
      float theta = iraw.tthe[i];
      float phi = (phiPresent ? iraw.ut[i] : 0.0f);

      // Parameters
      float delta = iraw.delt[i];
      // offset value is be subtracted so store negative
      delta *= -1.0f;
      // pressure, wall thickness
      float pressure = iraw.ut[i + pressureTabNum * numDets];
      float thickness = iraw.ut[i + thicknessTabNum * numDets];

      updateParameterMap(wsDetInfo, index, pmap, l2, theta, phi, delta, pressure, thickness);
    } catch (std::out_of_range &) {
      continue;
    }
  }
}

/**
 *
 * @param filename filename A full path to the input RAW file
 */
void LoadDetectorInfo::loadFromIsisNXS(const std::string &filename) {
  ::NeXus::File nxsFile(filename,
                        NXACC_READ); // will throw if file can't be opened

  // two types of file:
  //   - new type entry per detector
  //   - old libisis with single pressure, thickness entry for whole file

  // hold data read from file
  DetectorInfo detInfo;

  std::map<std::string, std::string> entries;
  nxsFile.getEntries(entries);
  if (entries.find("full_reference_detector") != entries.end()) {
    nxsFile.openGroup("full_reference_detector", "NXIXTdetector");
    readLibisisNxs(nxsFile, detInfo);
    nxsFile.closeGroup();
  } else if (entries.find("detectors.dat") != entries.end()) {
    nxsFile.openGroup("detectors.dat", "NXEntry");
    readNXSDotDat(nxsFile, detInfo);
    nxsFile.closeGroup();
  } else {
    throw std::invalid_argument("Unknown NeXus file type");
  }
  nxsFile.close();

  // Start loop over detectors
  auto &pmap = m_workspace->instrumentParameters();
  auto &wsDetInfo = m_workspace->mutableDetectorInfo();
  auto numDets = static_cast<int>(detInfo.ids.size());
  for (int i = 0; i < numDets; ++i) {
    detid_t detID = detInfo.ids[i];
    int code = detInfo.codes[i];
    try {
      size_t index = wsDetInfo.indexOf(detID);
      if (wsDetInfo.isMonitor(index) || code == 1)
        continue;

      // Positions
      double l2 = detInfo.l2[i];
      double theta = detInfo.theta[i];
      double phi = detInfo.phi[i];

      // Parameters
      double delta = detInfo.delays[i];
      // offset value is be subtracted so store negative
      delta *= -1.0;
      // pressure, wall thickness
      double pressure = detInfo.pressures[i];
      double thickness = detInfo.thicknesses[i];

      updateParameterMap(wsDetInfo, index, pmap, l2, theta, phi, delta, pressure, thickness);
    } catch (std::out_of_range &) {
      continue;
    }
  }
}

/**
 *
 * @param nxsFile A reference to the open NeXus fileIt should be opened at the
 *                "full_reference_detector" group
 * @param detInfo A reference to the struct that will hold the data from the
 *file
 */
void LoadDetectorInfo::readLibisisNxs(::NeXus::File &nxsFile, DetectorInfo &detInfo) const {
  nxsFile.readData<int32_t>("det_no", detInfo.ids);
  nxsFile.readData<int32_t>("det_type", detInfo.codes);
  nxsFile.readData<double>("delay_time", detInfo.delays);
  const size_t numDets = detInfo.ids.size();

  if (m_moveDets) {
    nxsFile.readData<double>("L2", detInfo.l2);
    nxsFile.readData<double>("theta", detInfo.theta);
    nxsFile.readData<double>("phi", detInfo.phi);
  } else {
    // these will get ignored
    detInfo.l2.resize(numDets, -1.0);
    detInfo.theta.resize(numDets, -1.0);
    detInfo.phi.resize(numDets, -1.0);
  }

  // pressure & wall thickness are global here
  double pressure = -1.0;
  double thickness = -1.0;
  nxsFile.openGroup("det_he3", "NXIXTdet_he3");
  nxsFile.readData<double>("gas_pressure", pressure);
  nxsFile.readData<double>("wall_thickness", thickness);
  nxsFile.closeGroup();
  if (pressure <= 0.0) {
    g_log.warning("The data file does not contain correct He3 pressure, "
                  "default value of 10 bar is used instead");
    pressure = 10.0;
  }
  if (thickness <= 0.0) {
    g_log.warning("The data file does not contain correct detector's wall "
                  "thickness, default value of 0.8mm is used instead");
    thickness = 0.0008;
  }
  detInfo.pressures.resize(numDets, pressure);
  detInfo.thicknesses.resize(numDets, thickness);
}

/**
 *
 * @param nxsFile A reference to the open NeXus fileIt should be opened at the
 *                "detectors.dat" group
 * @param detInfo A reference to the struct that will hold the data from the
 *file
 */
void LoadDetectorInfo::readNXSDotDat(::NeXus::File &nxsFile, DetectorInfo &detInfo) const {
  std::vector<int32_t> fileIDs;
  nxsFile.readData<int32_t>("detID", fileIDs); // containts both ids & codes
  std::vector<float> fileOffsets;
  nxsFile.readData<float>("timeOffsets", fileOffsets);
  const size_t numDets = fileOffsets.size() / 2;

  std::vector<float> detCoords;
  if (m_moveDets) {
    nxsFile.readData<float>("detSphericalCoord", detCoords);
  } else {
    detCoords.resize(3 * numDets, -1.0f);
  }

  // pressure & wall thickness
  std::vector<float> pressureAndWall;
  nxsFile.readData<float>("detPressureAndWall", pressureAndWall);

  if (numDets != fileIDs.size() / 2 || numDets != detCoords.size() / 3 || numDets != pressureAndWall.size() / 2) {
    std::ostringstream os;
    os << "The sizes of NeXus data columns are inconsistent in detectors.dat.\n"
       << "detIDs=" << fileIDs.size() << ", offsets=" << fileOffsets.size()
       << ", pressure & thickness=" << pressureAndWall.size() << "\n";
    throw std::runtime_error(os.str());
  }

  detInfo.ids.resize(numDets);
  detInfo.codes.resize(numDets);
  detInfo.delays.resize(numDets);
  detInfo.l2.resize(numDets);
  detInfo.theta.resize(numDets);
  detInfo.phi.resize(numDets);
  detInfo.pressures.resize(numDets);
  detInfo.thicknesses.resize(numDets);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < static_cast<int>(numDets); i++) {
    detInfo.ids[i] = fileIDs[2 * i];
    detInfo.codes[i] = fileIDs[2 * i + 1];
    detInfo.delays[i] = fileOffsets[2 * i];

    detInfo.l2[i] = detCoords[3 * i + 0];    // L2,
    detInfo.theta[i] = detCoords[3 * i + 1]; // Theta
    detInfo.phi[i] = detCoords[3 * i + 2];   // Phi

    detInfo.pressures[i] = pressureAndWall[2 * i];       // pressure;
    detInfo.thicknesses[i] = pressureAndWall[2 * i + 1]; // wallThickness;
  }
}

/**
 *
 * @param detectorInfo A reference to the workspace's detector info
 * @param detIndex The index of the detector whose parameters should be updated
 * @param pmap A reference to the ParameterMap instance to update
 * @param l2 The new l2 value
 * @param theta The new theta value
 * @param phi The new phi value
 * @param delay The new delay time
 * @param pressure The new pressure value
 * @param thickness The new thickness value
 */
void LoadDetectorInfo::updateParameterMap(Geometry::DetectorInfo &detectorInfo, const size_t detIndex,
                                          Geometry::ParameterMap &pmap, const double l2, const double theta,
                                          const double phi, const double delay, const double pressure,
                                          const double thickness) const {

  const auto detCompID = detectorInfo.detector(detIndex).getComponentID();

  // store detector params that are different to instrument level
  if (fabs(delay - m_instDelta) > 1e-06)
    pmap.addDouble(detCompID, DELAY_PARAM, delay);
  if (fabs(pressure - m_instPressure) > 1e-06)
    pmap.addDouble(detCompID, PRESSURE_PARAM, pressure);
  if (fabs(thickness - m_instThickness) > 1e-06)
    pmap.addDouble(detCompID, THICKNESS_PARAM, thickness);

  // move
  if (m_moveDets) {
    V3D newPos;
    newPos.spherical(l2, theta, phi);
    // The sample position may not be at 0,0,0
    newPos += m_samplePos;
    detectorInfo.setPosition(detIndex, newPos);
  }
}
} // namespace Mantid::DataHandling
