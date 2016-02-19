#include "MantidDataHandling/LoadSwans.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadSwans)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadSwans::LoadSwans() { m_ws = EventWorkspace_sptr(new EventWorkspace()); }

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadSwans::~LoadSwans() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadSwans::name() const { return "LoadSwans"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadSwans::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadSwans::category() const { return "DataHandling"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadSwans::summary() const { return "Loads SNS SWANS Data"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadSwans::init() {
  declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, {".dat", ".txt"}),
      "The name of the text file to read, including its full or "
      "relative path. The file extension must be .txt or .dat.");

  declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadSwans::exec() {

  // Load this here to get the necessary Parameters from the XML file
  loadInstrument();
  m_detector_size = setDetectorSize();
  std::map<uint32_t, std::vector<uint32_t>> pos_tof_map = LoadSwans::loadData();
  loadDataIntoTheWorkspace(pos_tof_map);
  loadInstrument();
  setTimeAxis();
  placeDetectorInSpace();
  setProperty("OutputWorkspace", m_ws);
}

/**
 * Run the Child Algorithm LoadInstrument.
 * It sets the workspace with the necessary information
 */
void LoadSwans::loadInstrument() {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_ws);
    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(true));
    loadInst->execute();
  } catch (...) {
    g_log.information("Cannot load the instrument definition.");
  }
}

/**
 * Place the detector in space according to the distance and angle
 * Needs in the IDF Parameters file the entries:
 * detector-name, detector-sample-distance, detector-rotation-angle
 */
void LoadSwans::placeDetectorInSpace() {

  std::string componentName =
      m_ws->getInstrument()->getStringParameter("detector-name")[0];
  const double distance = static_cast<double>(
      m_ws->getInstrument()->getNumberParameter("detector-sample-distance")[0]);
  const double angle = static_cast<double>(
      m_ws->getInstrument()->getNumberParameter("detector-rotation-angle")[0]);

  g_log.information() << "Moving detector " << componentName << " " << distance
                      << " meters and " << angle << " degrees." << std::endl;

  LoadHelper helper;

  const double deg2rad = M_PI / 180.0;
  V3D pos = helper.getComponentPosition(m_ws, componentName);
  double angle_rad = angle * deg2rad;
  V3D newpos(distance * sin(angle_rad), pos.Y(), distance * cos(angle_rad));
  helper.moveComponent(m_ws, componentName, newpos);

  // Apply a local rotation to stay perpendicular to the beam
  const V3D axis(0.0, 1.0, 0.0);
  Quat rotation(angle, axis);
  helper.rotateComponent(m_ws, componentName, rotation);
}

/**
 * Load the data into a map. The map is indexed by pixel id (0 to 128*128-1 =
 * m_detector_size)
 * The map values are the events TOF
 * @returns the map of events indexed by pixel index
 */
std::map<uint32_t, std::vector<uint32_t>> LoadSwans::loadData() {

  std::string filename = getPropertyValue("Filename");
  std::ifstream input(filename, std::ifstream::binary | std::ios::ate);
  input.seekg(0);

  m_ws->initialize(m_detector_size, 1, 1);

  std::map<uint32_t, std::vector<uint32_t>> pos_tof_map;

  while (input.is_open()) {
    if (input.eof())
      break;
    uint32_t tof = 0;
    input.read((char *)&tof, sizeof(tof));
    tof -= static_cast<uint32_t>(1e9);
    tof = static_cast<uint32_t>(tof * 0.1);
    uint32_t pos = 0;

    input.read((char *)&pos, sizeof(pos));
    if (pos < 400000) {
      g_log.warning() << "Detector index invalid: " << pos << std::endl;
      continue;
    }
    pos -= 400000;
    pos_tof_map[pos].push_back(tof);
  }

  return pos_tof_map;
}

/*
 * Puts all events from the map into the WS
 *
 */
void LoadSwans::loadDataIntoTheWorkspace(
    const std::map<uint32_t, std::vector<uint32_t>> &pos_tof_map) {
  for (auto it = pos_tof_map.begin(); it != pos_tof_map.end(); ++it) {

    EventList &el = m_ws->getEventList(it->first);
    el.setSpectrumNo(it->first);
    el.setDetectorID(it->first);
    for (auto itv = it->second.begin(); itv != it->second.end(); ++itv) {
      el.addEventQuickly(TofEvent(*itv));
    }
  }
}

/**
 * Get shortest and longest tof from the Parameters file and sets the time axis
 * Properties must be present in the Parameters file: shortest-tof, longest-tof
 */
void LoadSwans::setTimeAxis() {
  unsigned int shortest_tof = static_cast<unsigned int>(
      m_ws->getInstrument()->getNumberParameter("shortest-tof")[0]);
  unsigned int longest_tof = static_cast<unsigned int>(
      m_ws->getInstrument()->getNumberParameter("longest-tof")[0]);
  // Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec &xRef = axis.access();
  xRef.resize(2, 0.0);

  xRef[0] = shortest_tof; // Just to make sure the bins hold it all
  xRef[1] = longest_tof;
  // Set the binning axis using this.
  m_ws->setAllX(axis);
}

/**
 * From the Parameters XML file gets number-of-x-pixels and number-of-y-pixels
 * and calculates the detector size/shape
 */
unsigned int LoadSwans::setDetectorSize() {
  const unsigned int x_size = static_cast<unsigned int>(
      m_ws->getInstrument()->getNumberParameter("number-of-x-pixels")[0]);
  const unsigned int y_size = static_cast<unsigned int>(
      m_ws->getInstrument()->getNumberParameter("number-of-y-pixels")[0]);
  return x_size * y_size;
}

} // namespace DataHandling
} // namespace Mantid
