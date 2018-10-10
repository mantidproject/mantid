// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSpice2D2.h"
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
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using Poco::XML::Document;
using Poco::XML::DOMParser;
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
DECLARE_FILELOADER_ALGORITHM(LoadSpice2D2)


/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSpice2D2::confidence(Kernel::FileDescriptor &descriptor) const {
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
void LoadSpice2D2::init() {
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
void LoadSpice2D2::exec() {

  // Parse the XML metadata
  setInputFileAsHandler();
  setTimes();
  setWavelength();
  createWorkspace();
  storeMetaDataIntoWS();
  


  setProperty("OutputWorkspace", m_workspace);
}

/**
 * - Reads the input file 
 * - parses the data and metadata 
 * - Stores everything in an XML handler
 * - The metadata is stored in a map
 */
void LoadSpice2D2::setInputFileAsHandler() {
  // Set up the XmlHandler handler and parse xml file
  std::string fileName = getPropertyValue("Filename");
  try {
    m_xmlHandler = XmlHandler(fileName);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", fileName);
  }
  m_metadata = m_xmlHandler.get_metadata(m_tags_to_ignore);
}

void LoadSpice2D2::setTimes() {
  // start_time
  std::map<std::string, std::string> attributes =
      m_xmlHandler.get_attributes_from_tag("/");

  m_startTime = DateAndTime(attributes["start_time"]);
  m_endTime = DateAndTime(attributes["end_time"]);
}

/**
 * Sets the wavelength as class atributes
 * */
void LoadSpice2D2::setWavelength() {

  double wavelength_input = getProperty("Wavelength");
  double wavelength_spread_input = getProperty("WavelengthSpread");

  if (isEmpty(wavelength_input)) {
    m_wavelength = boost::lexical_cast<double>(m_metadata["Header/wavelength"]);
  }
  else {
    m_wavelength = wavelength_input;
  }

  if (isEmpty(wavelength_spread_input)) {
    m_dwavelength = boost::lexical_cast<double>(m_metadata["Header/wavelength_spread"]);
    // 20160720: New wavelength will be a ratio
    // UGLY HACK! Comparing dates...
    DateAndTime changingDate("2016-06-13 00:00:00");
    if (m_startTime >= changingDate) {
      g_log.debug() << "Using wavelength spread as a ratio..." << '\n';
      m_dwavelength = m_wavelength * m_dwavelength;
    }
  }
  else {
    m_dwavelength = wavelength_spread_input;
  }

  g_log.debug() << "Final Wavelength: " << m_wavelength << 
    " :: Wavelength Spread: " << m_dwavelength << '\n';
}

/**
 * Parse the 2 integers of the form: INT32[192,256]
 * @param dims_str : INT32[192,256]
 */
std::pair<int, int>
LoadSpice2D2::parseDetectorDimensions(const std::string &dims_str) {

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

std::vector<int> LoadSpice2D2::getData(const std::string &dataXpath) {

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
 * @param ws: workspace
 * @param specID: ID of the spectrum to store the value in
 * @param value: value to store [count]
 * @param error: error on the value [count]
 * @param wavelength: wavelength value [Angstrom]
 * @param dwavelength: error on the wavelength [Angstrom]
 */
void LoadSpice2D2::storeValue(int specID, double value,
                 double error, double wavelength, double dwavelength) {
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

void LoadSpice2D2::createWorkspace(){
 

  std::vector<int> data = getData("//Data");
  int numSpectra = static_cast<int>(data.size()) + LoadSpice2D2::nMonitors;

  m_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", numSpectra, 2, 1));
  m_workspace->setTitle(m_metadata["Header/Scan_Title"]);
  m_workspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("Wavelength");
  m_workspace->setYUnit("Counts");

  double monitorCounts = boost::lexical_cast<double>(m_metadata["Counters/monitor"]);
  double countingTime = boost::lexical_cast<double>(m_metadata["Counters/time"]);

  int specID = 0;
  // Store monitor counts in the beggining
  storeValue(specID++, monitorCounts,
              monitorCounts > 0 ? sqrt(monitorCounts) : 0.0, m_wavelength,
              m_dwavelength);
  
  storeValue(specID++, countingTime, 0.0, m_wavelength,
              m_dwavelength);

  // Store detector pixels
  for (auto count : data) {
    // Data uncertainties, computed according to the HFIR/IGOR reduction code
    // The following is what I would suggest instead...
    // error = count > 0 ? sqrt((double)count) : 0.0;
    double error = sqrt(0.5 + fabs(static_cast<double>(count) - 0.5));
    storeValue(specID++, count, error, m_wavelength,
                m_dwavelength);
  }
}

template <class T>
void LoadSpice2D2::addRunProperty(const std::string &name, const T &value,
                                 const std::string &units) {
  m_workspace->mutableRun().addProperty(name, value, units, true);
}

void LoadSpice2D2::storeMetaDataIntoWS(){
  addRunProperty<double>("wavelength", m_wavelength, "Angstrom");
  addRunProperty<double>("wavelength-spread", m_dwavelength, "Angstrom");
  addRunProperty<double>("wavelength-spread-ratio",
                         m_dwavelength / m_wavelength);

}

} // namespace DataHandling
} // namespace Mantid
