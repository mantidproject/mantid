// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadHFIRSANS.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/XmlHandler.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

#include <MantidKernel/StringTokenizer.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>
#include <Poco/Path.h>
#include <Poco/SAX/InputSource.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;

namespace Mantid {
namespace DataHandling {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using Types::Core::DateAndTime;
using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadHFIRSANS)

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadHFIRSANS::confidence(Kernel::FileDescriptor &descriptor) const {
  if (descriptor.extension() != ".xml")
    return 0;

  std::istream &is = descriptor.data();
  int confidence(0);

  { // start of inner scope
    Poco::XML::InputSource src(is);
    // Set up the DOM parser and parse xml file
    DOMParser pParser;
    Poco::AutoPtr<Document> pDoc;
    try {
      pDoc = pParser.parse(&src);
    } catch (Poco::Exception &e) {
      throw Kernel::Exception::FileError("Unable to parse File (" +
                                             descriptor.filename() + ")",
                                         e.displayText());
    } catch (...) {
      throw Kernel::Exception::FileError("Unable to parse File:",
                                         descriptor.filename());
    }
    // Get pointer to root element
    Element *pRootElem = pDoc->documentElement();
    if (pRootElem) {
      if (pRootElem->tagName() == "SPICErack") {
        confidence = 80;
      }
    }
  } // end of inner scope

  return confidence;
}

/// Overwrites Algorithm Init method.
void LoadHFIRSANS::init() {
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Load, ".xml"),
                  "The name of the input xml file to load");
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "The name of the Output workspace");

  // Optionally, we can specify the wavelength and wavelength spread and
  // overwrite
  // the value in the data file (used when the data file is not populated)
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Wavelength", EMPTY_DBL(), mustBePositive,
                  "Optional wavelength value to use when loading the data file "
                  "(Angstrom). This value will be used instead of the value "
                  "found in the data file.");
  declareProperty("WavelengthSpread", EMPTY_DBL(), mustBePositive,
                  "Optional wavelength spread value to use when loading the "
                  "data file (Angstrom). This value will be used instead of "
                  "the value found in the data file.");
  declareProperty(
      "SampleDetectorDistance", EMPTY_DBL(),
      "Sample to detector distance to use (overrides meta data), in mm");
}

/*******************************************************************************
 * Main method.
 */
void LoadHFIRSANS::exec() {

  // Parse the XML metadata
  setInputFileAsHandler();
  setTimes();
  setWavelength();
  createWorkspace();
  storeMetaDataIntoWS();
  runLoadInstrument();
  // ugly hack for Biosans wing detector:
  // it tests if there is metadata tagged with the wing detector
  // if so, puts the detector in the right angle
  if (m_metadata.find("Motor_Positions/det_west_wing_rot") !=
      m_metadata.end()) {
    rotateDetector();
    runLoadInstrument();
  }
  moveDetector();
  setBeamDiameter();
  setProperty("OutputWorkspace", m_workspace);
}

/**
 * - Reads the input file
 * - parses the data and metadata
 * - Stores everything in an XML handler
 * - The metadata is stored in a map
 */
void LoadHFIRSANS::setInputFileAsHandler() {
  // Set up the XmlHandler handler and parse xml file
  std::string fileName = getPropertyValue("Filename");
  try {
    m_xmlHandler = XmlHandler(fileName);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", fileName);
  }
  m_metadata = m_xmlHandler.get_metadata(m_tags_to_ignore);
  setSansSpiceXmlFormatVersion();
}

/***
 * 2016/11/09 : There is a new tag sans_spice_xml_format_version in the XML
 * It identifies changes in the XML format.
 * Useful to test tags rather than using the date.
 * @param metadata
 */
void LoadHFIRSANS::setSansSpiceXmlFormatVersion() {

  if (m_metadata.find("Header/sans_spice_xml_format_version") !=
      m_metadata.end()) {
    m_sansSpiceXmlFormatVersion = boost::lexical_cast<double>(
        m_metadata["Header/sans_spice_xml_format_version"]);
  }
  g_log.debug() << "Sans_spice_xml_format_version == "
                << m_sansSpiceXmlFormatVersion << "\n";
}

void LoadHFIRSANS::setTimes() {
  // start_time
  std::map<std::string, std::string> attributes =
      m_xmlHandler.get_attributes_from_tag("/");

  m_startTime = DateAndTime(attributes["start_time"]);
  m_endTime = DateAndTime(attributes["end_time"]);
}

/**
 * Sets the wavelength as class atributes
 * */
void LoadHFIRSANS::setWavelength() {

  double wavelength_input = getProperty("Wavelength");
  double wavelength_spread_input = getProperty("WavelengthSpread");

  if (isEmpty(wavelength_input)) {
    m_wavelength = boost::lexical_cast<double>(m_metadata["Header/wavelength"]);
  } else {
    m_wavelength = wavelength_input;
  }

  if (isEmpty(wavelength_spread_input)) {
    m_dwavelength =
        boost::lexical_cast<double>(m_metadata["Header/wavelength_spread"]);
    // 20160720: New wavelength will be a ratio
    // UGLY HACK! Comparing dates...
    DateAndTime changingDate("2016-06-13 00:00:00");
    if (m_startTime >= changingDate) {
      g_log.debug() << "Using wavelength spread as a ratio..." << '\n';
      m_dwavelength = m_wavelength * m_dwavelength;
    }
  } else {
    m_dwavelength = wavelength_spread_input;
  }

  g_log.debug() << "Final Wavelength: " << m_wavelength
                << " :: Wavelength Spread: " << m_dwavelength << '\n';
}

/**
 * Parse the 2 integers of the form: INT32[192,256]
 * @param dims_str : INT32[192,256]
 */
std::pair<int, int>
LoadHFIRSANS::parseDetectorDimensions(const std::string &dims_str) {

  // Read in the detector dimensions from the Detector tag

  std::pair<int, int> dims = std::make_pair(0, 0);

  boost::regex b_re_sig(R"(INT\d+\[(\d+),(\d+)\])");
  if (boost::regex_match(dims_str, b_re_sig)) {
    boost::match_results<std::string::const_iterator> match;
    boost::regex_search(dims_str, match, b_re_sig);
    // match[0] is the full string
    Kernel::Strings::convert(match[1], dims.first);
    Kernel::Strings::convert(match[2], dims.second);
  }
  if (dims.first == 0 || dims.second == 0)
    g_log.notice() << "Could not read in the number of pixels!" << '\n';
  return dims;
}

/**
 * Loads the data from the XML file
 */

std::vector<int> LoadHFIRSANS::getData(const std::string &dataXpath) {

  // data container
  std::vector<int> data;
  unsigned int totalDataSize = 0;

  // let's see how many detectors we have
  std::vector<std::string> detectors = m_xmlHandler.get_subnodes(dataXpath);
  g_log.debug() << "Number the detectors found in Xpath " << dataXpath << " = "
                << detectors.size() << '\n';

  // iterate every detector in the xml file
  for (const auto &detector : detectors) {
    std::string detectorXpath =
        std::string(dataXpath).append("/").append(detector);
    // type : INT32[192,256]
    std::map<std::string, std::string> attributes =
        m_xmlHandler.get_attributes_from_tag(detectorXpath);
    std::pair<int, int> dims = parseDetectorDimensions(attributes["type"]);

    // Horrible hack:
    // Some old files had a: //Data/DetectorWing with dimensions:
    // 16 x 256 = 4096. This must be ignored as it is not in the IDF
    // The real wing detector is larger than that
    if (detectorXpath.find("DetectorWing") != std::string::npos &&
        dims.first * dims.second <= 4096)
      break;

    totalDataSize += dims.first * dims.second;
    g_log.debug() << "Parsing detector XPath " << detectorXpath
                  << " with dimensions: " << dims.first << " x " << dims.second
                  << " = " << dims.first * dims.second << '\n';

    std::string data_str = m_xmlHandler.get_text_from_tag(detectorXpath);
    g_log.debug() << "The size of detector contents (xpath = " << detectorXpath
                  << ") is " << data_str.size() << " bytes." << '\n';

    // convert string data into a vector<int>
    std::stringstream iss(data_str);
    double number;
    while (iss >> number) {
      data.push_back(static_cast<int>(number));
    }
    g_log.debug() << "Detector XPath: " << detectorXpath
                  << " parsed. Total size of data processed up to now = "
                  << data.size() << " from a total of " << totalDataSize
                  << '\n';
  }

  if (data.size() != totalDataSize) {
    g_log.error() << "Total data size = " << totalDataSize
                  << ". Parsed data size = " << data.size() << '\n';
    throw Kernel::Exception::NotImplementedError(
        "Inconsistent data set: There were more data pixels found than "
        "declared in the Spice XML meta-data.");
  }
  return data;
}

/**
 * Convenience function to store a detector value into a given spectrum.
 * Note that this type of data doesn't use TOD, so that we use a single dummy
 * bin in X. Each detector is defined as a spectrum of length 1.
 * @param specID: ID of the spectrum to store the value in
 * @param value: value to store [count]
 * @param error: error on the value [count]
 * @param wavelength: wavelength value [Angstrom]
 * @param dwavelength: error on the wavelength [Angstrom]
 */
void LoadHFIRSANS::storeValue(int specID, double value, double error,
                              double wavelength, double dwavelength) {
  auto &X = m_workspace->mutableX(specID);
  auto &Y = m_workspace->mutableY(specID);
  auto &E = m_workspace->mutableE(specID);
  // The following is mostly to make Mantid happy by defining a histogram with
  // a single bin around the neutron wavelength
  X[0] = wavelength - dwavelength / 2.0;
  X[1] = wavelength + dwavelength / 2.0;
  Y[0] = value;
  E[0] = error;
  m_workspace->getSpectrum(specID).setSpectrumNo(specID);
}

void LoadHFIRSANS::createWorkspace() {

  std::vector<int> data = getData("//Data");
  int numSpectra = static_cast<int>(data.size()) + m_nMonitors;

  m_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", numSpectra, 2,
                                               1));
  m_workspace->setTitle(m_metadata["Header/Scan_Title"]);
  m_workspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("Wavelength");
  m_workspace->setYUnit("Counts");

  double monitorCounts =
      boost::lexical_cast<double>(m_metadata["Counters/monitor"]);
  double countingTime =
      boost::lexical_cast<double>(m_metadata["Counters/time"]);

  int specID = 0;
  // Store monitor counts in the beggining
  storeValue(specID++, monitorCounts,
             monitorCounts > 0 ? sqrt(monitorCounts) : 0.0, m_wavelength,
             m_dwavelength);

  storeValue(specID++, countingTime, 0.0, m_wavelength, m_dwavelength);

  // Store detector pixels
  for (auto count : data) {
    // Data uncertainties, computed according to the HFIR/IGOR reduction code
    // The following is what I would suggest instead...
    // error = count > 0 ? sqrt((double)count) : 0.0;
    double error = sqrt(0.5 + fabs(static_cast<double>(count) - 0.5));
    storeValue(specID++, count, error, m_wavelength, m_dwavelength);
  }
}

template <class T>
void LoadHFIRSANS::addRunProperty(const std::string &name, const T &value,
                                  const std::string &units) {
  m_workspace->mutableRun().addProperty(name, value, units, true);
}

/**
 * Sets the beam trap as Run Property
 * There's several beamstrap position. We have to find the maximum of every
 *motor above certain treshold.
 * The maximum motor position will be the trap in use.
 *
 * Notes:
 * Resting positions:
 * GPSANS: 1.0
 * BIOSANS: 9.999980
 *
 * Working positions:
 * GPSANS: 548.999969
 * BIOSANS: 544.999977
 */
void LoadHFIRSANS::setBeamTrapRunProperty() {

  std::vector<double> trapDiameters = {76.2, 50.8, 76.2, 101.6};
  // default use the shortest trap
  double trapDiameterInUse = trapDiameters[1];

  std::vector<double> trapMotorPositions;
  trapMotorPositions.push_back(
      boost::lexical_cast<double>(m_metadata["Motor_Positions/trap_y_25mm"]));
  trapMotorPositions.push_back(
      boost::lexical_cast<double>(m_metadata["Motor_Positions/trap_y_50mm"]));
  trapMotorPositions.push_back(
      boost::lexical_cast<double>(m_metadata["Motor_Positions/trap_y_76mm"]));
  trapMotorPositions.push_back(
      boost::lexical_cast<double>(m_metadata["Motor_Positions/trap_y_101mm"]));

  // Check how many traps are in use (store indexes):
  std::vector<size_t> trapIndexInUse;
  for (size_t i = 0; i < trapMotorPositions.size(); i++) {
    if (trapMotorPositions[i] > 26.0) {
      // Resting positions are below 25. Make sure we have one trap in use!
      trapIndexInUse.push_back(i);
    }
  }

  g_log.debug() << "trapIndexInUse length:" << trapIndexInUse.size() << "\n";

  // store trap diameters in use
  std::vector<double> trapDiametersInUse;
  trapDiametersInUse.reserve(trapIndexInUse.size());
  for (auto index : trapIndexInUse) {
    trapDiametersInUse.push_back(trapDiameters[index]);
  }

  g_log.debug() << "trapDiametersInUse length:" << trapDiametersInUse.size()
                << "\n";

  // The maximum value for the trapDiametersInUse is the trap in use
  std::vector<double>::iterator trapDiameterInUseIt =
      std::max_element(trapDiametersInUse.begin(), trapDiametersInUse.end());
  if (trapDiameterInUseIt != trapDiametersInUse.end())
    trapDiameterInUse = *trapDiameterInUseIt;

  g_log.debug() << "trapDiameterInUse:" << trapDiameterInUse << "\n";

  addRunProperty<double>("beam-trap-diameter", trapDiameterInUse, "mm");
}

/**
 * Add all metadata parsed values as log entries
 * Add any other metadata needed
 * */
void LoadHFIRSANS::storeMetaDataIntoWS() {

  for (const auto &keyValuePair : m_metadata) {
    std::string key = keyValuePair.first;
    std::replace(key.begin(), key.end(), '/', '_');
    m_workspace->mutableRun().addProperty(key, keyValuePair.second, true);
  }

  addRunProperty<std::string>("start_time", m_startTime.toISO8601String(), "");
  addRunProperty<std::string>("run_start", m_startTime.toISO8601String(), "");
  m_workspace->mutableRun().setStartAndEndTime(m_startTime, m_endTime);

  setBeamTrapRunProperty();

  addRunProperty<double>("wavelength", m_wavelength, "Angstrom");
  addRunProperty<double>("wavelength-spread", m_dwavelength, "Angstrom");
  addRunProperty<double>("wavelength-spread-ratio",
                         m_dwavelength / m_wavelength);

  addRunProperty<double>(
      "monitor", boost::lexical_cast<double>(m_metadata["Counters/monitor"]));
  addRunProperty<double>(
      "timer", boost::lexical_cast<double>(m_metadata["Counters/time"]), "sec");

  // sample thickness
  // XML 1.03: source distance is now in meters
  double sample_thickness =
      boost::lexical_cast<double>(m_metadata["Header/Sample_Thickness"]);
  if (m_sansSpiceXmlFormatVersion >= 1.03) {
    g_log.debug() << "sans_spice_xml_format_version >= 1.03 :: "
                     "sample_thickness in mm. Converting to cm...";
    sample_thickness *= 0.1;
  }
  addRunProperty<double>("sample-thickness", sample_thickness, "cm");

  addRunProperty<double>(
      "source-aperture-diameter",
      boost::lexical_cast<double>(m_metadata["Header/source_aperture_size"]),
      "mm");
  addRunProperty<double>(
      "sample-aperture-diameter",
      boost::lexical_cast<double>(m_metadata["Header/sample_aperture_size"]),
      "mm");
  // XML 1.03: source distance is now in meters
  double source_distance =
      boost::lexical_cast<double>(m_metadata["Header/source_distance"]);
  if (m_sansSpiceXmlFormatVersion >= 1.03) {
    g_log.debug() << "sans_spice_xml_format_version >= 1.03 :: source_distance "
                     "in meters. Converting to mm...";
    source_distance *= 1000.0;
  }
  addRunProperty<double>("source-sample-distance", source_distance, "mm");
  addRunProperty<double>(
      "number-of-guides",
      boost::lexical_cast<double>(m_metadata["Motor_Positions/nguides"]));
}

/**
 * Run the Child Algorithm LoadInstrument
 */
void LoadHFIRSANS::runLoadInstrument() {

  const std::string &instrumentName = m_metadata["Header/Instrument"];

  API::IAlgorithm_sptr loadInstrumentAlgorithm =
      createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInstrumentAlgorithm->setPropertyValue("InstrumentName", instrumentName);
    loadInstrumentAlgorithm->setProperty<API::MatrixWorkspace_sptr>(
        "Workspace", m_workspace);
    loadInstrumentAlgorithm->setProperty("RewriteSpectraMap",
                                         Mantid::Kernel::OptionalBool(true));
    loadInstrumentAlgorithm->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm");
  } catch (std::runtime_error &) {
    g_log.information(
        "Unable to successfully run LoadInstrument Child Algorithm");
  }
}

/**
 * This will rotate the detector named componentName around z-axis
 */
void LoadHFIRSANS::rotateDetector() {

  // The angle is negative!
  double angle = -boost::lexical_cast<double>(
      m_metadata["Motor_Positions/det_west_wing_rot"]);

  g_log.notice() << "Rotating Wing Detector " << angle << " degrees." << '\n';

  API::Run &runDetails = m_workspace->mutableRun();
  auto *p = new Mantid::Kernel::TimeSeriesProperty<double>("rotangle");
  //	auto p = boost::make_shared <Mantid::Kernel::TimeSeriesProperty<double>
  //>("rotangle");
  p->addValue(DateAndTime::getCurrentTime(), angle);
  runDetails.addLogData(p);
}

/**
 * Calculates the detector distances and sets them as Run properties
 * @return : sample_detector_distance
 */
void LoadHFIRSANS::setDetectorDistance() {

  m_sampleDetectorDistance = getProperty("SampleDetectorDistance");

  if (!isEmpty(m_sampleDetectorDistance)) {
    // SDD is as input
    g_log.debug() << "Getting the SampleDetectorDistance = "
                  << m_sampleDetectorDistance
                  << " from the Algorithm input property.\n";
  } else if (m_metadata.find("Motor_Positions/sdd") != m_metadata.end()) {
    // Newest version: SDD as a specific tag
    m_sampleDetectorDistance =
        boost::lexical_cast<double>(m_metadata["Motor_Positions/sdd"]);
    m_sampleDetectorDistance *= 1000.0;
  } else if (m_metadata.find("Motor_Positions/sample_det_dist") !=
             m_metadata.end()) {
    // Old Format
    double sampleDetectorDistancePartial = boost::lexical_cast<double>(
        m_metadata["Motor_Positions/sample_det_dist"]);
    sampleDetectorDistancePartial *= 1000.0;

    double sampleDetectorDistanceOffset =
        boost::lexical_cast<double>(m_metadata["Header/tank_internal_offset"]);

    double sampleDetectorDistanceWindow =
        boost::lexical_cast<double>(m_metadata["Header/sample_to_flange"]);

    m_sampleDetectorDistance = sampleDetectorDistancePartial +
                               sampleDetectorDistanceOffset +
                               sampleDetectorDistanceWindow;
  } else {
    // New format:
    m_sampleDetectorDistance = boost::lexical_cast<double>(
        m_metadata["Motor_Positions/sample_det_dist"]);
    m_sampleDetectorDistance *= 1000.0;
  }
  g_log.debug() << "Sample Detector Distance = " << m_sampleDetectorDistance
                << " mm." << '\n';
  addRunProperty<double>("sample-detector-distance", m_sampleDetectorDistance,
                         "mm");
  // return m_sampleDetectorDistance;
}

/**
 * Places the detector at the right sample_detector_distance
 */
void LoadHFIRSANS::moveDetector() {

  setDetectorDistance();
  double translation_distance =
      boost::lexical_cast<double>(m_metadata["Motor_Positions/detector_trans"]);
  translation_distance /= 1000.0;
  g_log.debug() << "Detector Translation = " << translation_distance
                << " meters." << '\n';

  // Move the detector to the right position
  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");

  // Finding the name of the detector object.
  std::string detID =
      m_workspace->getInstrument()->getStringParameter("detector-name")[0];

  g_log.information() << "Moving: " << detID
                      << " Z=" << m_sampleDetectorDistance / 1000.0
                      << " X=" << translation_distance << '\n';
  try {
    mover->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_workspace);
    mover->setProperty("ComponentName", detID);
    mover->setProperty("Z", m_sampleDetectorDistance / 1000.0);
    mover->setProperty("X", -translation_distance);
    mover->execute();
  } catch (std::invalid_argument &e) {
    g_log.error("Invalid argument to MoveInstrumentComponent Child Algorithm");
    g_log.error(e.what());
  } catch (std::runtime_error &e) {
    g_log.error(
        "Unable to successfully run MoveInstrumentComponent Child Algorithm");
    g_log.error(e.what());
  }
}

/**
 * From the parameters file get a string parameter
 * */
std::string
LoadHFIRSANS::getInstrumentStringParameter(const std::string &parameter) {
  std::vector<std::string> pars =
      m_workspace->getInstrument()->getStringParameter(parameter);
  if (pars.empty()) {
    g_log.warning() << "Parameter not found: " << parameter
                    << " in the instrument parameter file.\n";
    return std::string();
  } else {
    g_log.debug() << "Found the parameter: " << parameter << " = " << pars[0]
                  << " in the instrument parameter file.\n";
    return pars[0];
  }
}

/**
 * From the parameters file get a double parameter
 * */
double
LoadHFIRSANS::getInstrumentDoubleParameter(const std::string &parameter) {
  std::vector<double> pars =
      m_workspace->getInstrument()->getNumberParameter(parameter);
  if (pars.empty()) {
    g_log.warning() << "Parameter not found: " << parameter
                    << " in the instrument parameter file.\n";
    return std::numeric_limits<double>::quiet_NaN();
  } else {
    g_log.debug() << "Found the parameter: " << parameter << " = " << pars[0]
                  << " in the instrument parameter file.\n";
    return pars[0];
  }
}

double LoadHFIRSANS::getSourceToSampleDistance() {
  const int nguides = static_cast<int>(
      boost::lexical_cast<double>(m_metadata["Motor_Positions/nguides"]));
  // m_workspace->run().getPropertyValueAsType<int>("number-of-guides");

  // aperture-distances: array from the instrument parameters
  std::string pars = getInstrumentStringParameter("aperture-distances");

  double SSD = 0;
  Mantid::Kernel::StringTokenizer tok(
      pars, ",", Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  if (tok.count() > 0 && tok.count() < 10 && nguides >= 0 && nguides < 9) {
    const std::string distance_as_string = tok[8 - nguides];
    try {
      auto distance_as_string_copy =
          boost::algorithm::trim_copy(distance_as_string);
      SSD = boost::lexical_cast<double>(distance_as_string_copy);
    } catch (boost::bad_lexical_cast const &e) {
      g_log.error(e.what());
      throw Kernel::Exception::InstrumentDefinitionError(
          "Bad value for source-to-sample distance");
    }
  } else
    throw Kernel::Exception::InstrumentDefinitionError(
        "Unable to get source-to-sample distance");

  // Check for an offset
  double sourceSampleDistanceOffset =
      getInstrumentDoubleParameter("source-distance-offset");
  if (!std::isnan(sourceSampleDistanceOffset)) {
    SSD += sourceSampleDistanceOffset;
  }
  g_log.debug() << "Source Sample Distance = " << SSD << ".\n";
  return SSD;
}

/**
 * Compute beam diameter at the detector
 * */
void LoadHFIRSANS::setBeamDiameter() {

  double SourceToSampleDistance = 0.0;

  try {
    SourceToSampleDistance = getSourceToSampleDistance();
    m_workspace->mutableRun().addProperty("source-sample-distance",
                                          SourceToSampleDistance, "mm", true);
    g_log.information() << "Computed SSD from number of guides: "
                        << SourceToSampleDistance << " mm \n";
  } catch (...) {
    Mantid::Kernel::Property *prop =
        m_workspace->run().getProperty("source-sample-distance");
    Mantid::Kernel::PropertyWithValue<double> *dp =
        dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    SourceToSampleDistance = *dp;
    g_log.warning() << "Could not compute SSD from number of guides, taking: "
                    << SourceToSampleDistance << " mm \n";
  }

  const double sampleAperture =
      boost::lexical_cast<double>(m_metadata["Header/sample_aperture_size"]);
  const double sourceAperture =
      boost::lexical_cast<double>(m_metadata["Header/source_aperture_size"]);
  g_log.debug() << "Computing beam diameter. m_sampleDetectorDistance="
                << m_sampleDetectorDistance
                << " SourceToSampleDistance=" << SourceToSampleDistance
                << " sourceAperture= " << sourceAperture
                << " sampleAperture=" << sampleAperture << "\n";

  const double beamDiameter = m_sampleDetectorDistance /
                                  SourceToSampleDistance *
                                  (sourceAperture + sampleAperture) +
                              sampleAperture;
  m_workspace->mutableRun().addProperty("beam-diameter", beamDiameter, "mm",
                                        true);
}

} // namespace DataHandling
} // namespace Mantid
