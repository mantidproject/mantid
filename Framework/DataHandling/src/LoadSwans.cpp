#include "MantidDataHandling/LoadSwans.h"
#include "MantidAPI/Axis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidAPI/RegisterFileLoader.h"

#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>
#include <boost/algorithm/string/predicate.hpp> //starts_with
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadSwans)

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

  std::string filename = Poco::Path(descriptor.filename()).getFileName();

  if (descriptor.extension().compare(".dat") == 0 &&
      boost::starts_with(filename, "RUN"))
    return 100;
  else
    return 0;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadSwans::init() {
  declareProperty(
      make_unique<FileProperty>("FilenameData", "", FileProperty::Load, ".dat"),
      "The name of the text file to read, including its full or "
      "relative path. The file extension must be .dat.");

  declareProperty(make_unique<FileProperty>("FilenameMetaData", "",
                                            API::FileProperty::OptionalLoad,
                                            "meta.dat"),
                  "The name of the text file to read, including its full or "
                  "relative path. The file extension must be meta.dat.");

  declareProperty(
      make_unique<FileProperty>("FilenameAutoRecord", "",
                                API::FileProperty::OptionalLoad, ".txt"),
      "The name of the AutoRecord.txt file to read, including its full or "
      "relative path. The file extension must be .txt");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterByTofMin", 10000.0, Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the minimum accepted value in microseconds.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "FilterByTofMax", 30000.0, Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the maximum accepted value in microseconds.");

  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadSwans::exec() {

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
  const double angle = m_ws->run().getPropertyValueAsType<double>("angle");

  g_log.information() << "LoadSwans::placeDetectorInSpace: Moving detector "
                      << componentName << " " << distance << " meters and "
                      << angle << " degrees." << std::endl;

  LoadHelper helper;
  constexpr double deg2rad = M_PI / 180.0;
  V3D pos = helper.getComponentPosition(m_ws, componentName);
  double angle_rad = angle * deg2rad;
  V3D newpos(distance * sin(angle_rad), pos.Y(), distance * cos(angle_rad));
  helper.moveComponent(m_ws, componentName, newpos);

  // Apply a local rotation to stay perpendicular to the beam
  const V3D axis(0.0, 1.0, 0.0);
  Quat rotation(angle, axis);
  helper.rotateComponent(m_ws, componentName, rotation);

  // keep the Z distance as property
  m_ws->mutableRun().addProperty<double>("sample_detector_distance", distance);
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
  std::map<uint32_t, std::vector<uint32_t>> eventMap;

  while (input.is_open()) {
    if (input.eof())
      break;
    uint32_t tof = 0;
    input.read(reinterpret_cast<char *>(&tof), sizeof(tof));
    uint32_t pos = 0;
    input.read(reinterpret_cast<char *>(&pos), sizeof(pos));

    // The 1e9 added to the tof ticks is used to signal a “problem” with the
    // event.
    // 2e6 is the maximum allowed tof tick for 5Hz operation, thus a very safe
    // upperlimit.
    if (tof > 2e6)
      tof -= static_cast<uint32_t>(1e9);
    tof = static_cast<uint32_t>(tof * 0.1);
    if (pos < 400000) {
      g_log.warning() << "Detector index invalid: " << pos << std::endl;
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

  // if the filename for metadata was not given, strip the extension of the data
  // file, and add _meta.dat
  if (filename == "") {
    std::string filenameData = getPropertyValue("FilenameData");
    size_t lastindex = filenameData.find_last_of(".");
    std::string rawname = filenameData.substr(0, lastindex);
    filename = rawname + "_meta.dat";
    g_log.information("Assuming meta data file: " + filename);
  }

  std::ifstream infile(filename);
  if (infile.fail()) {
    g_log.error("Error reading file " + filename);
    throw Exception::FileError("Unable to read meta data in File:", filename);
  }
  std::string line;
  while (getline(infile, line)) {
    // line with data, need to be parsed by white spaces
    if (!line.empty() && line[0] != '#') {
      g_log.debug() << "Metadata parsed line: " << line << std::endl;
      auto tokenizer = Mantid::Kernel::StringTokenizer(
          line, "\t ", Mantid::Kernel::StringTokenizer::TOK_TRIM |
                           Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
      for (const auto &token : tokenizer) {
        metadata.push_back(boost::lexical_cast<double>(token));
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

/**
 * Loads the /SNS/VULCAN/IPTS-16013/shared/AutoRecord.txt file *
 * If not given will try to look for it from the filename
 * /SNS/VULCAN/IPTS-16013/shared/SANS_detector/RUN80814.dat
 *
 * The AutoRecord is of the format:
 * RUN     IPTS    Title   Notes   Sample  ITEM    StartTime       Duration
 *ProtonCharge    TotalCounts     Monitor1        Monitor2        X       Y
 *Z       O       HROT    VROT    BandCentre      BandWidth       Frequency
 *Guide   IX      IY      IZ      IHA     IVA     Collimator
 *MTSDisplacement MTSForce        MTSStrain       MTSStress       MTSAngle
 *MTSTorque       MTSLaser        MTSlaserstrain  MTSDisplaceoffset
 *MTSAngleceoffset        MTST1   MTST2   MTST3   MTST4   MTSHighTempStrain
 *FurnaceT        FurnaceOT       FurnacePower    VacT    VacOT
 *EuroTherm1Powder        EuroTherm1SP    EuroTherm1Temp  EuroTherm2Powder
 *EuroTherm2SP    EuroTherm2Temp
 * 80680   IPTS-16013      SWANS TESTING   SWANS TESTING   No sample       -1.0
 *2016-02-12 09:35:45.856802666-EST       147.697776      179951088095.549500
 *678.000000      2.000000        2.000000        1.411   -25.707 -152.067
 *45.0    0.0     0.0     2.0     2.88    30.0    181.993 100.003 -0.125201
 *-1.458688       4.997994        11.990609       0.0     2.25265546341
 *2267.793997     0.010018        47.1121006667   -2.408037       11.7260375
 *-0.010656       2.3415954       1.135141        1.470575        28.412535
 *29.690638       542.133681      542.133681      0.00477137777778        0.0
 *0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
 *
 *
 * It builds a dictionary for this run
 */

void LoadSwans::setAutoRecordAsWorkspaceProperties(std::string runNumber) {

  std::string filename = getPropertyValue("FilenameAutoRecord");

  // if the filename for AutoRecord was not given, find it from the data file.
  if (filename == "") {
    std::string filenameData = getPropertyValue("FilenameData");
    size_t lastIndexForIPTS = filenameData.find("IPTS");
    size_t lastIndexForIPTSBar = filenameData.find("/", lastIndexForIPTS);
    std::string rawname = filenameData.substr(0, lastIndexForIPTSBar);
    filename = rawname + "/shared/AutoRecord.txt";
    g_log.information("Assuming AutoRecord file: " + filename);
  }

  std::vector<std::string> header;
  std::vector<std::string> values;
  std::ifstream infile(filename);
  if (infile.fail()) {
    g_log.error("Error reading AutoRecord file " + filename);
  } else {
    int count = 0;
    std::string line;
    while (getline(infile, line)) {
      if (!line.empty()) {
        auto tokenizer = Mantid::Kernel::StringTokenizer(
            line, "\t", Mantid::Kernel::StringTokenizer::TOK_TRIM |
                            Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
        if (count == 0) {
          g_log.debug() << "HEADER: " << line << std::endl;
          for (const auto &token : tokenizer)
            header.push_back(token);
        } else {
          if (tokenizer[0] == runNumber) {
            g_log.debug() << "Value for run " << runNumber << ": " << line
                          << std::endl;
            for (const auto &token : tokenizer)
              values.push_back(token);
            break;
          }
        }
      }
      count++;
    }
    // Sets parsed data as properties in the ws
    API::Run &runDetails = m_ws->mutableRun();
    std::vector<std::string>::const_iterator ih, iv;
    for (ih = header.begin(), iv = values.begin();
         ih < header.end() && iv < values.end(); ++ih, ++iv) {
      try {
        double value = boost::lexical_cast<double>(*iv);
        runDetails.addProperty<double>(*ih, value, true);
      } catch (const boost::bad_lexical_cast &) {
        runDetails.addProperty(*ih, *iv, true);
      }
    }
  }
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
  runDetails.addProperty<double>("run_number", metadata[0]);
  runDetails.addProperty<double>("wavelength", metadata[1]);
  runDetails.addProperty<double>("chopper_frequency",
                                 metadata[2]); // chopper frequency
  runDetails.addProperty<double>("angle", metadata[5]);
  setAutoRecordAsWorkspaceProperties(
      boost::lexical_cast<std::string>(metadata[0]));
}

/*
 * Puts all events from the map into the WS
 *
 */
void LoadSwans::loadDataIntoTheWorkspace(
    const std::map<uint32_t, std::vector<uint32_t>> &eventMap) {

  m_ws->initialize(m_detector_size, 1, 1);
  m_ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  m_ws->setYUnit("Counts");
  m_ws->setTitle("SWANS Event Workspace");

  for (const auto &position : eventMap) {
    EventList &el = m_ws->getEventList(position.first);
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
  double shortest_tof = getProperty("FilterByTofMin");
  double longest_tof = getProperty("FilterByTofMax");
  // Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec &xRef = axis.access();
  xRef = {shortest_tof, longest_tof};
  // Set the binning axis using this.
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
