//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/XmlHandler.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Strings.h"

#include <boost/regex.hpp>
#include <boost/shared_array.hpp>
#include <Poco/Path.h>
#include <MantidKernel/StringTokenizer.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Text.h>
#include <Poco/SAX/InputSource.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

//-----------------------------------------------------------------------

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::NodeList;
using Poco::XML::Node;
using Poco::XML::Text;

namespace Mantid {
namespace DataHandling {
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
  MantidVec &X = ws->dataX(specID);
  MantidVec &Y = ws->dataY(specID);
  MantidVec &E = ws->dataE(specID);
  // The following is mostly to make Mantid happy by defining a histogram with
  // a single bin around the neutron wavelength
  X[0] = wavelength - dwavelength / 2.0;
  X[1] = wavelength + dwavelength / 2.0;
  Y[0] = value;
  E[0] = error;
  ws->getSpectrum(specID)->setSpectrumNo(specID);
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSpice2D::confidence(Kernel::FileDescriptor &descriptor) const {
  if (descriptor.extension().compare(".xml") != 0)
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
    } catch (...) {
      throw Kernel::Exception::FileError("Unable to parse File:",
                                         descriptor.filename());
    }
    // Get pointer to root element
    Element *pRootElem = pDoc->documentElement();
    if (pRootElem) {
      if (pRootElem->tagName().compare("SPICErack") == 0) {
        confidence = 80;
      }
    }
  } // end of inner scope

  return confidence;
}

/// Constructor
LoadSpice2D::LoadSpice2D()
    : m_wavelength_input(0), m_wavelength_spread_input(0), m_numberXPixels(0),
      m_numberYPixels(0), m_wavelength(0), m_dwavelength(0) {}

/// Destructor
LoadSpice2D::~LoadSpice2D() {}

/// Overwrites Algorithm Init method.
void LoadSpice2D::init() {
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
  std::map<std::string, std::string> metadata =
      m_xmlHandler.get_metadata("Detector");
  setWavelength(metadata);

  std::vector<int> data = getData("//Data/Detector");
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
  runLoadInstrument(instrument, m_workspace);
  runLoadMappingTable(m_workspace, m_numberXPixels, m_numberYPixels);

  // sample_detector_distances
  double detector_distance = detectorDistance(metadata);
  moveDetector(detector_distance);
}

/**
 * Parse the 2 integers of the form: INT32[192,256]
 * @param dims_str : INT32[192,256]
 */
void LoadSpice2D::parseDetectorDimensions(const std::string &dims_str) {

  // Read in the detector dimensions from the Detector tag

  boost::regex b_re_sig("INT\\d+\\[(\\d+),(\\d+)\\]");
  if (boost::regex_match(dims_str, b_re_sig)) {
    boost::match_results<std::string::const_iterator> match;
    boost::regex_search(dims_str, match, b_re_sig);
    // match[0] is the full string
    Kernel::Strings::convert(match[1], m_numberXPixels);
    Kernel::Strings::convert(match[2], m_numberYPixels);
  }
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
                << m_wavelength_input << " , " << m_wavelength_input
                << std::endl;

  std::string fileName = getPropertyValue("Filename");
  // Set up the XmlHandler handler and parse xml file
  try {
    m_xmlHandler = XmlHandler(fileName);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", fileName);
  }
}

/**
 * Gets the wavelenght and wavelength spread from the  metadata
 * and sets them as class attributes
 */
void LoadSpice2D::setWavelength(std::map<std::string, std::string> &metadata) {
  // Read in wavelength and wavelength spread

  g_log.debug() << "setWavelength: " << m_wavelength_input << " , "
                << m_wavelength_input << std::endl;

  if (isEmpty(m_wavelength_input)) {
    std::string s = metadata["Header/wavelength"];
    from_string<double>(m_wavelength, s, std::dec);
    s = metadata["Header/wavelength_spread"];
    from_string<double>(m_dwavelength, s, std::dec);

    g_log.debug() << "setWavelength: " << m_wavelength << " , " << m_dwavelength
                  << std::endl;

  } else {
    m_wavelength = m_wavelength_input;
    m_dwavelength = m_wavelength_spread_input;
  }
}
/**
 * Parses the data dimensions and stores them as member variables
 * Reads the data and returns a vector
 */
std::vector<int>
LoadSpice2D::getData(const std::string &dataXpath = "//Data/Detector") {
  // type : INT32[192,256]
  std::map<std::string, std::string> attributes =
      m_xmlHandler.get_attributes_from_tag(dataXpath);
  parseDetectorDimensions(attributes["type"]);
  if (m_numberXPixels == 0 || m_numberYPixels == 0)
    g_log.notice() << "Could not read in the number of pixels!" << std::endl;

  std::string data_str = m_xmlHandler.get_text_from_tag(dataXpath);
  // convert string data into a vector<int>
  std::vector<int> data;
  std::stringstream iss(data_str);
  int number;
  while (iss >> number) {
    data.push_back(number);
  }

  if (data.size() != static_cast<size_t>(m_numberXPixels * m_numberYPixels))
    throw Kernel::Exception::NotImplementedError(
        "Inconsistent data set: There were more data pixels found than "
        "declared in the Spice XML meta-data.");
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
  int numSpectra = m_numberXPixels * m_numberYPixels + LoadSpice2D::nMonitors;

  m_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", numSpectra,
                                               nBins + 1, nBins));
  m_workspace->setTitle(title);
  m_workspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("Wavelength");
  m_workspace->setYUnit("");
  API::Workspace_sptr workspace =
      boost::static_pointer_cast<API::Workspace>(m_workspace);
  setProperty("OutputWorkspace", workspace);

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
 */
void LoadSpice2D::setBeamTrapRunProperty(
    std::map<std::string, std::string> &metadata) {

  // Read in beam trap positions
  double trap_pos = 0;
  from_string<double>(trap_pos, metadata["Motor_Positions/trap_y_25mm"],
                      std::dec);

  double beam_trap_diam = 25.4;

  double highest_trap = 0;
  from_string<double>(trap_pos, metadata["Motor_Positions/trap_y_101mm"],
                      std::dec);

  if (trap_pos > highest_trap) {
    highest_trap = trap_pos;
    beam_trap_diam = 101.6;
  }

  from_string<double>(trap_pos, metadata["Motor_Positions/trap_y_50mm"],
                      std::dec);
  if (trap_pos > highest_trap) {
    highest_trap = trap_pos;
    beam_trap_diam = 50.8;
  }

  from_string<double>(trap_pos, metadata["Motor_Positions/trap_y_76mm"],
                      std::dec);
  if (trap_pos > highest_trap) {
    beam_trap_diam = 76.2;
  }

  addRunProperty<double>("beam-trap-diameter", beam_trap_diam, "mm");
}
void LoadSpice2D::setMetadataAsRunProperties(
    std::map<std::string, std::string> &metadata) {
  setBeamTrapRunProperty(metadata);

  // start_time
  std::map<std::string, std::string> attributes =
      m_xmlHandler.get_attributes_from_tag("/");
  addRunProperty<std::string>(attributes, "start_time", "start_time", "");
  addRunProperty<std::string>(attributes, "start_time", "run_start", "");

  // sample thickness
  addRunProperty<double>(metadata, "Header/Sample_Thickness",
                         "sample-thickness", "mm");

  addRunProperty<double>(metadata, "Header/source_aperture_size",
                         "source-aperture-diameter", "mm");
  addRunProperty<double>(metadata, "Header/sample_aperture_size",
                         "sample-aperture-diameter", "mm");
  addRunProperty<double>(metadata, "Header/source_distance",
                         "source-sample-distance", "mm");
  addRunProperty<int>(metadata, "Motor_Positions/nguides", "number-of-guides",
                      "");

  addRunProperty<double>("wavelength", m_wavelength, "Angstrom");
  addRunProperty<double>("wavelength-spread", m_dwavelength, "Angstrom");

  addRunProperty<double>(metadata, "Counters/monitor", "monitor", "");
  addRunProperty<double>(metadata, "Counters/time", "timer", "sec");
}

/**
 * Calculates the detector distances and sets them as Run properties
 * Here fog starts:
 * BioSANS: distance = sample_det_dist + offset!
 * GPSANS: distance = sample_det_dist + offset + sample_to_flange!
 * Mathieu is using sample_det_dist to move the detector later
 * So I'll do the same (Ricardo)
 * @return : sample_detector_distance
 */
double
LoadSpice2D::detectorDistance(std::map<std::string, std::string> &metadata) {

  // sample_detector_distances
  double sample_detector_distance = 0;
  from_string<double>(sample_detector_distance,
                      metadata["Motor_Positions/sample_det_dist"], std::dec);
  sample_detector_distance *= 1000.0;
  addRunProperty<double>("sample-detector-distance", sample_detector_distance,
                         "mm");

  double sample_detector_distance_offset =
      addRunProperty<double>(metadata, "Header/tank_internal_offset",
                             "sample-detector-distance-offset", "mm");

  double sample_si_window_distance = addRunProperty<double>(
      metadata, "Header/sample_to_flange", "sample-si-window-distance", "mm");

  double total_sample_detector_distance = sample_detector_distance +
                                          sample_detector_distance_offset +
                                          sample_si_window_distance;
  addRunProperty<double>("total-sample-detector-distance",
                         total_sample_detector_distance, "mm");

  // Store sample-detector distance
  declareProperty("SampleDetectorDistance", sample_detector_distance,
                  Kernel::Direction::Output);

  return sample_detector_distance;
}

/**
 * Places the detector at the right sample_detector_distance
 */
void LoadSpice2D::moveDetector(double sample_detector_distance) {
  // Move the detector to the right position
  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");

  // Finding the name of the detector object.
  std::string detID =
      m_workspace->getInstrument()->getStringParameter("detector-name")[0];

  g_log.information("Moving " + detID);
  try {
    mover->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_workspace);
    mover->setProperty("ComponentName", detID);
    mover->setProperty("Z", sample_detector_distance / 1000.0);
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
                          Mantid::Kernel::OptionalBool(false));
    loadInst->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm");
  } catch (std::runtime_error &) {
    g_log.information(
        "Unable to successfully run LoadInstrument Child Algorithm");
  }
}

/**
 * Populate spectra mapping to detector IDs
 *
 * TODO: Get the detector size information from the workspace directly
 *
 * @param localWorkspace: Workspace2D object
 * @param nxbins: number of bins in X
 * @param nybins: number of bins in Y
 */
void LoadSpice2D::runLoadMappingTable(
    DataObjects::Workspace2D_sptr localWorkspace, int nxbins, int nybins) {
  // Get the number of monitor channels
  boost::shared_ptr<const Geometry::Instrument> instrument =
      localWorkspace->getInstrument();
  std::vector<detid_t> monitors = instrument->getMonitors();
  const int nMonitors = static_cast<int>(monitors.size());

  // Number of monitors should be consistent with data file format
  if (nMonitors != LoadSpice2D::nMonitors) {
    std::stringstream error;
    error << "Geometry error for " << instrument->getName()
          << ": Spice data format defines " << LoadSpice2D::nMonitors
          << " monitors, " << nMonitors << " were/was found";
    throw std::runtime_error(error.str());
  }

  // Generate mapping of detector/channel IDs to workspace index

  // Detector/channel counter
  int icount = 0;

  // Monitor: IDs start at 1 and increment by 1
  for (int i = 0; i < nMonitors; i++) {
    localWorkspace->getSpectrum(icount)->setDetectorID(icount + 1);
    icount++;
  }

  // Detector pixels
  for (int ix = 0; ix < nxbins; ix++) {
    for (int iy = 0; iy < nybins; iy++) {
      localWorkspace->getSpectrum(icount)
          ->setDetectorID(1000000 + iy * 1000 + ix);
      icount++;
    }
  }
}

/* This method throws not found error if a element is not found in the xml file
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
}
}
