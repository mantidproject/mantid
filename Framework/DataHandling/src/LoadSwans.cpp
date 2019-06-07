// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSwans.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/StringTokenizer.h"

#include <algorithm>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <map>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Types::Event::TofEvent;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadSwans)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadSwans::LoadSwans() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadSwans::name() const { return "LoadSwans"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadSwans::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadSwans::category() const {
  return "DataHandling\\Text;SANS\\DataHandling";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadSwans::summary() const { return "Loads SNS SWANS Data"; }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSwans::confidence(Kernel::FileDescriptor &descriptor) const {
  // since this is a test loader, the confidence will always be 0!
  // I don't want the Load algorithm to pick this one!
  if (descriptor.extension() != ".dat")
    return 1;
  else
    return 0;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadSwans::init() {
  declareProperty(std::make_unique<FileProperty>("FilenameData", "",
                                                 FileProperty::Load, ".dat"),
                  "The name of the text file to read, including its full or "
                  "relative path. The file extension must be .dat.");

  declareProperty(std::make_unique<FileProperty>(
                      "FilenameMetaData", "", FileProperty::Load, "meta.dat"),
                  "The name of the text file to read, including its full or "
                  "relative path. The file extension must be meta.dat.");

  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadSwans::exec() {
  m_ws = boost::make_shared<Mantid::DataObjects::EventWorkspace>();
  // Load instrument here to get the necessary Parameters from the XML file
  loadInstrument();
  m_detector_size = getDetectorSize();
  std::map<uint32_t, std::vector<uint32_t>> eventMap = loadData();
  std::vector<double> metadata = loadMetaData();
  setMetaDataAsWorkspaceProperties(metadata);
  loadDataIntoTheWorkspace(eventMap);
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
 * detector-name, detector-sample-distance
 * Also the metadata file has to have the rotation angle value
 */
void LoadSwans::placeDetectorInSpace() {

  std::string componentName =
      m_ws->getInstrument()->getStringParameter("detector-name")[0];
  const double distance = static_cast<double>(
      m_ws->getInstrument()->getNumberParameter("detector-sample-distance")[0]);
  // Make the angle negative to accommodate the sense of rotation.
  const double angle = -m_ws->run().getPropertyValueAsType<double>("angle");

  g_log.information() << "Moving detector " << componentName << " " << distance
                      << " meters and " << angle << " degrees.\n";

  LoadHelper helper;
  constexpr double deg2rad = M_PI / 180.0;
  V3D pos = helper.getComponentPosition(m_ws, componentName);
  double angle_rad = angle * deg2rad;
  V3D newpos(distance * sin(angle_rad), pos.Y(), distance * cos(angle_rad));
  helper.moveComponent(m_ws, componentName, newpos);

  // Apply a local rotation to stay perpendicular to the beam
  constexpr V3D axis(0.0, 1.0, 0.0);
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

  std::string filename = getPropertyValue("FilenameData");
  std::ifstream input(filename, std::ifstream::binary | std::ios::ate);
  input.seekg(0);

  m_ws->initialize(m_detector_size, 1, 1);

  std::map<uint32_t, std::vector<uint32_t>> eventMap;

  while (input.is_open()) {
    if (input.eof())
      break;
    uint32_t tof = 0;
    input.read(reinterpret_cast<char *>(&tof), sizeof(tof));
    tof -= static_cast<uint32_t>(1e9);
    tof = static_cast<uint32_t>(tof * 0.1);

    uint32_t pos = 0;
    input.read(reinterpret_cast<char *>(&pos), sizeof(pos));
    if (pos < 400000) {
      g_log.warning() << "Detector index invalid: " << pos << '\n';
      continue;
    }
    pos -= 400000;
    eventMap[pos].push_back(tof);
  }
  return eventMap;
}

/**
 * Load metadata file witch to date is just a line of of double values
 * Parses this file and put it into a vector
 * @return vector with the file contents
 */
std::vector<double> LoadSwans::loadMetaData() {
  std::vector<double> metadata;
  std::string filename = getPropertyValue("FilenameMetaData");
  std::ifstream infile(filename);
  if (infile.fail()) {
    g_log.error("Error reading file " + filename);
    throw Exception::FileError("Unable to read data in File:", filename);
  }
  std::string line;
  while (getline(infile, line)) {
    // line with data, need to be parsed by white spaces
    if (!line.empty() && line[0] != '#') {
      g_log.debug() << "Metadata parsed line: " << line << '\n';
      auto tokenizer = Mantid::Kernel::StringTokenizer(
          line, "\t ",
          Mantid::Kernel::StringTokenizer::TOK_TRIM |
              Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
      metadata.reserve(tokenizer.size());
      for (const auto &token : tokenizer) {
        metadata.emplace_back(boost::lexical_cast<double>(token));
      }
    }
  }
  if (metadata.size() < 6) {
    g_log.error("Expecting length >=6 for metadata arguments!");
    throw Exception::NotFoundError(
        "Number of arguments for metadata must be at least 6. Found: ",
        metadata.size());
  }
  return metadata;
}

/*
 * Known metadata positions to date:
 * 0 - run number
 * 1 - wavelength
 * 2 - chopper frequency
 * 3 - time offset
 * 4 - ??
 * 5 - angle
 */
void LoadSwans::setMetaDataAsWorkspaceProperties(
    const std::vector<double> &metadata) {
  API::Run &runDetails = m_ws->mutableRun();
  runDetails.addProperty<double>("wavelength", metadata[1]);
  runDetails.addProperty<double>("angle", metadata[5]);
}

/*
 * Puts all events from the map into the WS
 *
 */
void LoadSwans::loadDataIntoTheWorkspace(
    const std::map<uint32_t, std::vector<uint32_t>> &eventMap) {
  for (const auto &position : eventMap) {
    EventList &el = m_ws->getSpectrum(position.first);
    el.setSpectrumNo(position.first);
    el.setDetectorID(position.first);
    for (const auto &event : position.second) {
      el.addEventQuickly(TofEvent(event));
    }
  }
}

/**
 * Get shortest and longest tof from the Parameters file and sets the time
 * axis
 * Properties must be present in the Parameters file: shortest-tof,
 * longest-tof
 */
void LoadSwans::setTimeAxis() {
  const unsigned int shortest_tof = static_cast<unsigned int>(
      m_ws->getInstrument()->getNumberParameter("shortest-tof")[0]);
  const unsigned int longest_tof = static_cast<unsigned int>(
      m_ws->getInstrument()->getNumberParameter("longest-tof")[0]);
  // Now, create a default X-vector for histogramming, with just 2 bins.
  auto axis = HistogramData::BinEdges{static_cast<double>(shortest_tof),
                                      static_cast<double>(longest_tof)};
  m_ws->setAllX(axis);
}

/**
 * From the Parameters XML file gets number-of-x-pixels and number-of-y-pixels
 * and calculates the detector size/shape
 */
unsigned int LoadSwans::getDetectorSize() {
  const unsigned int x_size = static_cast<unsigned int>(
      m_ws->getInstrument()->getNumberParameter("number-of-x-pixels")[0]);
  const unsigned int y_size = static_cast<unsigned int>(
      m_ws->getInstrument()->getNumberParameter("number-of-y-pixels")[0]);
  return x_size * y_size;
}

} // namespace DataHandling
} // namespace Mantid
