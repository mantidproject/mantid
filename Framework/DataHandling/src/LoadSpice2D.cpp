// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSpice2D.h"
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

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/StringTokenizer.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>
#include <Poco/Path.h>
#include <Poco/SAX/InputSource.h>

#include <algorithm>
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
DECLARE_FILELOADER_ALGORITHM(LoadSpice2D)

// Parse string and convert to numeric type
template <class T>
bool from_string(T &t, const std::string &s,
                 std::ios_base &(*f)(std::ios_base &)) {
  std::istringstream iss(s);
  return !(iss >> f >> t).fail();
}

/**
 * Convenience function to store a detector value into a given spectrum.
 * Note that this type of data doesn't use TOD, so that we use a single dummy
 * bin in X. Each detector is defined as a spectrum of length 1.
 * @param ws: workspace
 * @param specID: ID of the spectrum to store the value in
 * @param value: value to store [count]
 * @param error: error on the value [count]
 * @param wavelength: wavelength value [Angstrom]
 * @param dwavelength: error on the wavelength [Angstrom]
 */
void store_value(DataObjects::Workspace2D_sptr ws, int specID, double value,
                 double error, double wavelength, double dwavelength) {
  auto &X = ws->mutableX(specID);
  auto &Y = ws->mutableY(specID);
  auto &E = ws->mutableE(specID);
  // The following is mostly to make Mantid happy by defining a histogram with
  // a single bin around the neutron wavelength
  X[0] = wavelength - dwavelength / 2.0;
  X[1] = wavelength + dwavelength / 2.0;
  Y[0] = value;
  E[0] = error;
  ws->getSpectrum(specID).setSpectrumNo(specID);
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSpice2D::confidence(Kernel::FileDescriptor &descriptor) const {
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
void LoadSpice2D::init() {
  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Load, ".xml"),
                  "The name of the input xml file to load");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>(
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
  declareProperty("WavelengthSpread", 0.1, mustBePositive,
                  "Optional wavelength spread value to use when loading the "
                  "data file (Angstrom). This value will be used instead of "
                  "the value found in the data file.");
}

/*
 * Main method.
 * Creates an XML handler. All tag values will be a map.
 * Creates and loads the workspace with the data
 *
 */
void LoadSpice2D::exec() {

  setInputPropertiesAsMemberProperties();
  setTimes();
  const std::vector<std::string> tags_to_ignore{"Detector", "DetectorWing"};
  std::map<std::string, std::string> metadata =
      m_xmlHandler.get_metadata(tags_to_ignore);

  setSansSpiceXmlFormatVersion(metadata);
  setWavelength(metadata);

  std::vector<int> data = getData("//Data");

  double monitorCounts = 0;
  from_string<double>(monitorCounts, metadata["Counters/monitor"], std::dec);
  double countingTime = 0;
  from_string<double>(countingTime, metadata["Counters/time"], std::dec);
  createWorkspace(data, metadata["Header/Scan_Title"], monitorCounts,
                  countingTime);

  // Add all metadata to the WS
  addMetadataAsRunProperties(metadata);

  setMetadataAsRunProperties(metadata);

  // run load instrument
  std::string instrument = metadata["Header/Instrument"];

  // ugly hack for Biosans wing detector:
  // it tests if there is metadata tagged with the wing detector
  // if so, puts the detector in the right angle
  if (metadata.find("Motor_Positions/det_west_wing_rot") != metadata.end()) {
    double angle = boost::lexical_cast<double>(
        metadata["Motor_Positions/det_west_wing_rot"]);
    rotateDetector(-angle);
  }
  // sample_detector_distances
  detectorDistance(metadata);
  detectorTranslation(metadata);
  runLoadInstrument(instrument, m_workspace);
  setProperty("OutputWorkspace", m_workspace);
}

/**
 * Parse the 2 integers of the form: INT32[192,256]
 * @param dims_str : INT32[192,256]
 */
std::pair<int, int>
LoadSpice2D::parseDetectorDimensions(const std::string &dims_str) {

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
 * Adds map of the form key:value
 * as Workspace run properties
 */
void LoadSpice2D::addMetadataAsRunProperties(
    const std::map<std::string, std::string> &metadata) {

  for (const auto &keyValuePair : metadata) {
    std::string key = keyValuePair.first;
    std::replace(key.begin(), key.end(), '/', '_');
    m_workspace->mutableRun().addProperty(key, keyValuePair.second, true);
  }
}

/**
 * Get the input algorithm properties and sets them as class attributes
 */
void LoadSpice2D::setInputPropertiesAsMemberProperties() {

  m_wavelength_input = getProperty("Wavelength");
  m_wavelength_spread_input = getProperty("WavelengthSpread");

  g_log.debug() << "setInputPropertiesAsMemberProperties: "
                << m_wavelength_input << " , " << m_wavelength_input << '\n';

  std::string fileName = getPropertyValue("Filename");
  // Set up the XmlHandler handler and parse xml file
  try {
    m_xmlHandler = XmlHandler(fileName);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", fileName);
  }
}

/**
 * Gets the wavelength and wavelength spread from the  metadata
 * and sets them as class attributes
 */
void LoadSpice2D::setWavelength(std::map<std::string, std::string> &metadata) {
  // Read in wavelength and wavelength spread

  g_log.debug() << "setWavelength: " << m_wavelength_input << " , "
                << m_wavelength_input << '\n';

  if (isEmpty(m_wavelength_input)) {
    std::string s = metadata["Header/wavelength"];
    from_string<double>(m_wavelength, s, std::dec);
    s = metadata["Header/wavelength_spread"];
    from_string<double>(m_dwavelength, s, std::dec);

    // 20160720: New wavelength will be a ratio
    // UGLY HACK! Comparing dates...
    DateAndTime changingDate("2016-06-13 00:00:00");
    if (m_startTime >= changingDate) {
      g_log.debug() << "Using wavelength spread as a ratio" << '\n';
      m_dwavelength = m_wavelength * m_dwavelength;
    }

    g_log.debug() << "setWavelength: " << m_wavelength << " , " << m_dwavelength
                  << '\n';

  } else {
    m_wavelength = m_wavelength_input;
    m_dwavelength = m_wavelength_spread_input;
  }
}
/**
 * Parses the data dimensions and stores them as member variables
 * Reads the data and returns a vector
 */
std::vector<int> LoadSpice2D::getData(const std::string &dataXpath = "//Data") {

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
    // 16 x 256 = 4096. This must be ignored as it is not in the IDF.
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
 * Creates workspace and loads the data along with 2 monitors!
 */
void LoadSpice2D::createWorkspace(const std::vector<int> &data,
                                  const std::string &title,
                                  double monitor1_counts,
                                  double monitor2_counts) {
  int nBins = 1;
  int numSpectra = static_cast<int>(data.size()) + LoadSpice2D::nMonitors;

  m_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", numSpectra,
                                               nBins + 1, nBins));
  m_workspace->setTitle(title);
  m_workspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("Wavelength");
  m_workspace->setYUnit("");

  int specID = 0;
  // Store monitor counts in the beggining
  store_value(m_workspace, specID++, monitor1_counts,
              monitor1_counts > 0 ? sqrt(monitor1_counts) : 0.0, m_wavelength,
              m_dwavelength);
  store_value(m_workspace, specID++, monitor2_counts, 0.0, m_wavelength,
              m_dwavelength);

  // Store detector pixels
  for (auto count : data) {
    // Data uncertainties, computed according to the HFIR/IGOR reduction code
    // The following is what I would suggest instead...
    // error = count > 0 ? sqrt((double)count) : 0.0;
    double error = sqrt(0.5 + fabs(static_cast<double>(count) - 0.5));
    store_value(m_workspace, specID++, count, error, m_wavelength,
                m_dwavelength);
  }
}

/**
 * Inserts a property in the run with a new name!
 */
template <class T>
T LoadSpice2D::addRunProperty(std::map<std::string, std::string> &metadata,
                              const std::string &oldName,
                              const std::string &newName,
                              const std::string &units) {

  std::string value_str = metadata[oldName];
  T value;
  from_string<T>(value, value_str, std::dec);
  m_workspace->mutableRun().addProperty(newName, value, units, true);
  return value;
}

template <class T>
void LoadSpice2D::addRunProperty(const std::string &name, const T &value,
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
void LoadSpice2D::setBeamTrapRunProperty(
    std::map<std::string, std::string> &metadata) {

  std::vector<double> trapDiameters = {76.2, 50.8, 76.2, 101.6};
  // default use the shortest trap
  double trapDiameterInUse = trapDiameters[1];

  std::vector<double> trapMotorPositions;
  trapMotorPositions.push_back(
      boost::lexical_cast<double>(metadata["Motor_Positions/trap_y_25mm"]));
  trapMotorPositions.push_back(
      boost::lexical_cast<double>(metadata["Motor_Positions/trap_y_50mm"]));
  trapMotorPositions.push_back(
      boost::lexical_cast<double>(metadata["Motor_Positions/trap_y_76mm"]));
  trapMotorPositions.push_back(
      boost::lexical_cast<double>(metadata["Motor_Positions/trap_y_101mm"]));

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
    trapDiametersInUse.emplace_back(trapDiameters[index]);
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

void LoadSpice2D::setTimes() {
  // start_time
  std::map<std::string, std::string> attributes =
      m_xmlHandler.get_attributes_from_tag("/");

  m_startTime = DateAndTime(attributes["start_time"]);
  m_endTime = DateAndTime(attributes["end_time"]);
}

void LoadSpice2D::setMetadataAsRunProperties(
    std::map<std::string, std::string> &metadata) {

  setBeamTrapRunProperty(metadata);

  addRunProperty<std::string>("start_time", m_startTime.toISO8601String(), "");
  addRunProperty<std::string>("run_start", m_startTime.toISO8601String(), "");

  m_workspace->mutableRun().setStartAndEndTime(m_startTime, m_endTime);

  // sample thickness
  // XML 1.03: source distance is now in meters
  double sample_thickness;
  from_string<double>(sample_thickness, metadata["Header/Sample_Thickness"],
                      std::dec);
  if (m_sansSpiceXmlFormatVersion >= 1.03) {
    g_log.debug()
        << "sans_spice_xml_format_version >= 1.03 :: sample_thickness "
           "in mm. Converting to cm...";
    sample_thickness *= 0.1;
  }
  addRunProperty<double>("sample-thickness", sample_thickness, "cm");

  addRunProperty<double>(metadata, "Header/Sample_Thickness",
                         "sample-thickness", "mm");

  addRunProperty<double>(metadata, "Header/source_aperture_size",
                         "source-aperture-diameter", "mm");
  addRunProperty<double>(metadata, "Header/sample_aperture_size",
                         "sample-aperture-diameter", "mm");

  // XML 1.03: source distance is now in meters
  double source_distance;
  from_string<double>(source_distance, metadata["Header/source_distance"],
                      std::dec);
  if (m_sansSpiceXmlFormatVersion >= 1.03) {
    g_log.debug() << "sans_spice_xml_format_version >= 1.03 :: source_distance "
                     "in meters. Converting to mm...";
    source_distance *= 1000.0;
  }
  addRunProperty<double>("source-sample-distance", source_distance, "mm");

  addRunProperty<int>(metadata, "Motor_Positions/nguides", "number-of-guides",
                      "");

  addRunProperty<double>("wavelength", m_wavelength, "Angstrom");
  addRunProperty<double>("wavelength-spread", m_dwavelength, "Angstrom");
  addRunProperty<double>("wavelength-spread-ratio",
                         m_dwavelength / m_wavelength);

  addRunProperty<double>(metadata, "Counters/monitor", "monitor", "");
  addRunProperty<double>(metadata, "Counters/time", "timer", "sec");
}

/**
 * Calculates the detector distances and sets them as Run properties
 * Here fog starts:
 * GPSANS: distance = sample_det_dist + offset!
 * BioSANS: distance = sample_det_dist + offset + sample_to_flange!
 * Mathieu is using sample_det_dist to move the detector later
 * So I'll do the same (Ricardo)
 * June 14th 2016:
 * New changes:
 * sample_det_dist is not available
 * flange_det_dist is new = old sample_det_dist + offset
 * offset is not used
 * GPSANS: distance = flange_det_dist! (sample_to_flange is 0 for GPSANS)
 * BioSANS: distance = flange_det_dist + sample_to_flange!
 * For back compatibility I'm setting the offset to 0 and not reading it from
 * the file
 * Last Changes:
 * If SDD tag is available in the metadata set that as sample detector distance
 * Puts a numeric series in the log with the value of sample_detector_distance
 */
void LoadSpice2D::detectorDistance(
    std::map<std::string, std::string> &metadata) {

  double sample_detector_distance = 0, sample_detector_distance_offset = 0,
         sample_si_window_distance = 0;

  // check if it's the new format
  if (metadata.find("Motor_Positions/sample_det_dist") != metadata.end()) {
    // Old Format
    from_string<double>(sample_detector_distance,
                        metadata["Motor_Positions/sample_det_dist"], std::dec);
    sample_detector_distance *= 1000.0;
    addRunProperty<double>("sample-detector-distance", sample_detector_distance,
                           "mm");
    sample_detector_distance_offset =
        addRunProperty<double>(metadata, "Header/tank_internal_offset",
                               "sample-detector-distance-offset", "mm");
    sample_si_window_distance = addRunProperty<double>(
        metadata, "Header/sample_to_flange", "sample-si-window-distance", "mm");

  } else {
    // New format:
    from_string<double>(sample_detector_distance,
                        metadata["Motor_Positions/flange_det_dist"], std::dec);
    sample_detector_distance *= 1000.0;
    addRunProperty<double>("sample-detector-distance-offset", 0, "mm");
    addRunProperty<double>("sample-detector-distance", sample_detector_distance,
                           "mm");
    sample_si_window_distance = addRunProperty<double>(
        metadata, "Header/sample_to_flange", "sample-si-window-distance", "mm");
  }

  double total_sample_detector_distance;
  if (metadata.find("Motor_Positions/sdd") != metadata.end()) {

    // When sdd exists overrides all the distances
    from_string<double>(total_sample_detector_distance,
                        metadata["Motor_Positions/sdd"], std::dec);
    total_sample_detector_distance *= 1000.0;
    sample_detector_distance = total_sample_detector_distance;

    addRunProperty<double>("sample-detector-distance-offset", 0, "mm");
    addRunProperty<double>("sample-detector-distance", sample_detector_distance,
                           "mm");
    addRunProperty<double>("sample-si-window-distance", 0, "mm");

    g_log.debug() << "Sample-Detector-Distance from SDD tag = "
                  << total_sample_detector_distance << '\n';

  } else {
    total_sample_detector_distance = sample_detector_distance +
                                     sample_detector_distance_offset +
                                     sample_si_window_distance;
  }
  addRunProperty<double>("total-sample-detector-distance",
                         total_sample_detector_distance, "mm");

  // Add to the log!
  API::Run &runDetails = m_workspace->mutableRun();
  auto *p = new Mantid::Kernel::TimeSeriesProperty<double>("sdd");
  p->addValue(DateAndTime::getCurrentTime(), total_sample_detector_distance);
  runDetails.addLogData(p);

  // Store sample-detector distance
  declareProperty("SampleDetectorDistance", sample_detector_distance,
                  Kernel::Direction::Output);
}
/**
 * Puts a numeric series in the log with the value of detector translation
 */
void LoadSpice2D::detectorTranslation(
    std::map<std::string, std::string> &metadata) {

  // detectorTranslations
  double detectorTranslation = 0;
  from_string<double>(detectorTranslation,
                      metadata["Motor_Positions/detector_trans"], std::dec);
  // Add to the log!
  API::Run &runDetails = m_workspace->mutableRun();
  auto *p =
      new Mantid::Kernel::TimeSeriesProperty<double>("detector-translation");
  p->addValue(DateAndTime::getCurrentTime(), detectorTranslation);
  runDetails.addLogData(p);

  g_log.debug() << "Detector Translation = " << detectorTranslation << " mm."
                << '\n';
}

/** Run the Child Algorithm LoadInstrument (as for LoadRaw)
 * @param inst_name :: The name written in the Nexus file
 * @param localWorkspace :: The workspace to insert the instrument into
 */
void LoadSpice2D::runLoadInstrument(
    const std::string &inst_name,
    DataObjects::Workspace2D_sptr localWorkspace) {

  API::IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", inst_name);
    loadInst->setProperty<API::MatrixWorkspace_sptr>("Workspace",
                                                     localWorkspace);
    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(true));
    loadInst->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm");
  } catch (std::runtime_error &) {
    g_log.information(
        "Unable to successfully run LoadInstrument Child Algorithm");
  }
}

/* This method throws not found error if a element is not found in the xml
 * file
 * @param elem :: pointer to  element
 * @param name ::  element name
 * @param fileName :: xml file name
 */
void LoadSpice2D::throwException(Poco::XML::Element *elem,
                                 const std::string &name,
                                 const std::string &fileName) {
  if (!elem) {
    throw Kernel::Exception::NotFoundError(
        name + " element not found in Spice XML file", fileName);
  }
}

/**
 * This will rotate the detector named componentName around z-axis
 *
 *
 * @param angle in degrees
 */
void LoadSpice2D::rotateDetector(const double &angle) {

  g_log.notice() << "Rotating Wing Detector " << angle << " degrees." << '\n';

  API::Run &runDetails = m_workspace->mutableRun();
  auto *p = new Mantid::Kernel::TimeSeriesProperty<double>("rotangle");
  //	auto p = boost::make_shared <Mantid::Kernel::TimeSeriesProperty<double>
  //>("rotangle");

  p->addValue(DateAndTime::getCurrentTime(), angle);
  runDetails.addLogData(p);
}

/***
 * 2016/11/09 : There is a new tag sans_spice_xml_format_version in the XML
 * It identifies changes in the XML format.
 * Useful to test tags rather than using the date.
 * @param metadata
 */
void LoadSpice2D::setSansSpiceXmlFormatVersion(
    std::map<std::string, std::string> &metadata) {

  if (metadata.find("Header/sans_spice_xml_format_version") != metadata.end()) {
    m_sansSpiceXmlFormatVersion = boost::lexical_cast<double>(
        metadata["Header/sans_spice_xml_format_version"]);
  }
  g_log.debug() << "Sans_spice_xml_format_version == "
                << m_sansSpiceXmlFormatVersion << "\n";
}
} // namespace DataHandling
} // namespace Mantid
