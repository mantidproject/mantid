//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidAPI/AlgorithmFactory.h"

#include <boost/regex.hpp>
#include <boost/shared_array.hpp>
#include <Poco/Path.h>
#include <Poco/StringTokenizer.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Text.h>
#include <Poco/SAX/InputSource.h>

#include <iostream>
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
 *  Convenience function to read in from an XML element
 *  @param t: variable to feed the data in
 *  @param pRootElem: element to read from
 *  @param element: name of the element to read
 *  @param fileName: file path
 */
template <class T>
bool from_element(T &t, Element *pRootElem, const std::string &element,
                  const std::string &fileName) {
  Element *sasEntryElem = pRootElem->getChildElement(element);
  if (!sasEntryElem)
    throw Kernel::Exception::NotFoundError(
        element + " element not found in Spice XML file", fileName);
  std::stringstream value_str(sasEntryElem->innerText());
  return !(value_str >> t).fail();
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
LoadSpice2D::LoadSpice2D() {}

/// Destructor
LoadSpice2D::~LoadSpice2D() {}

/// Overwrites Algorithm Init method.
void LoadSpice2D::init() {
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, ".xml"),
      "The name of the input xml file to load");
  declareProperty(new API::WorkspaceProperty<API::Workspace>(
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

/// Overwrites Algorithm exec method
void LoadSpice2D::exec() {
  std::string fileName = getPropertyValue("Filename");

  const double wavelength_input = getProperty("Wavelength");
  const double wavelength_spread_input = getProperty("WavelengthSpread");

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Poco::AutoPtr<Document> pDoc;
  try {
    pDoc = pParser.parse(fileName);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", fileName);
  }
  // Get pointer to root element
  Element *pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes()) {
    throw Kernel::Exception::NotFoundError("No root element in Spice XML file",
                                           fileName);
  }

  // Read in start time
  const std::string start_time = pRootElem->getAttribute("start_time");

  Element *sasEntryElem = pRootElem->getChildElement("Header");
  throwException(sasEntryElem, "Header", fileName);

  // Read in scan title
  Element *element = sasEntryElem->getChildElement("Scan_Title");
  throwException(element, "Scan_Title", fileName);
  std::string wsTitle = element->innerText();

  // Read in instrument name
  element = sasEntryElem->getChildElement("Instrument");
  throwException(element, "Instrument", fileName);
  std::string instrument = element->innerText();

  // Read sample thickness
  double sample_thickness = 0;
  from_element<double>(sample_thickness, sasEntryElem, "Sample_Thickness",
                       fileName);

  double source_apert = 0.0;
  from_element<double>(source_apert, sasEntryElem, "source_aperture_size",
                       fileName);

  double sample_apert = 0.0;
  from_element<double>(sample_apert, sasEntryElem, "sample_aperture_size",
                       fileName);

  double source_distance = 0.0;
  from_element<double>(source_distance, sasEntryElem, "source_distance",
                       fileName);

  // Read in wavelength and wavelength spread
  double wavelength = 0;
  double dwavelength = 0;
  if (isEmpty(wavelength_input)) {
    from_element<double>(wavelength, sasEntryElem, "wavelength", fileName);
    from_element<double>(dwavelength, sasEntryElem, "wavelength_spread",
                         fileName);
  } else {
    wavelength = wavelength_input;
    dwavelength = wavelength_spread_input;
  }

  // Read in positions
  sasEntryElem = pRootElem->getChildElement("Motor_Positions");
  throwException(sasEntryElem, "Motor_Positions", fileName);

  // Read in the number of guides
  int nguides = 0;
  from_element<int>(nguides, sasEntryElem, "nguides", fileName);

  // Read in sample-detector distance in mm
  double distance = 0;
  from_element<double>(distance, sasEntryElem, "sample_det_dist", fileName);
  distance *= 1000.0;

  // Read in beam trap positions
  double highest_trap = 0;
  double trap_pos = 0;

  from_element<double>(trap_pos, sasEntryElem, "trap_y_25mm", fileName);
  double beam_trap_diam = 25.4;

  from_element<double>(highest_trap, sasEntryElem, "trap_y_101mm", fileName);
  if (trap_pos > highest_trap) {
    highest_trap = trap_pos;
    beam_trap_diam = 101.6;
  }

  from_element<double>(trap_pos, sasEntryElem, "trap_y_50mm", fileName);
  if (trap_pos > highest_trap) {
    highest_trap = trap_pos;
    beam_trap_diam = 50.8;
  }

  from_element<double>(trap_pos, sasEntryElem, "trap_y_76mm", fileName);
  if (trap_pos > highest_trap) {
    beam_trap_diam = 76.2;
  }

  // Read in counters
  sasEntryElem = pRootElem->getChildElement("Counters");
  throwException(sasEntryElem, "Counters", fileName);

  double countingTime = 0;
  from_element<double>(countingTime, sasEntryElem, "time", fileName);
  double monitorCounts = 0;
  from_element<double>(monitorCounts, sasEntryElem, "monitor", fileName);

  // Read in the data image
  Element *sasDataElem = pRootElem->getChildElement("Data");
  throwException(sasDataElem, "Data", fileName);

  // Read in the data buffer
  element = sasDataElem->getChildElement("Detector");
  throwException(element, "Detector", fileName);
  std::string data_str = element->innerText();

  // Read in the detector dimensions from the Detector tag
  int numberXPixels = 0;
  int numberYPixels = 0;
  std::string data_type = element->getAttribute("type");
  boost::regex b_re_sig("INT\\d+\\[(\\d+),(\\d+)\\]");
  if (boost::regex_match(data_type, b_re_sig)) {
    boost::match_results<std::string::const_iterator> match;
    boost::regex_search(data_type, match, b_re_sig);
    // match[0] is the full string
    Kernel::Strings::convert(match[1], numberXPixels);
    Kernel::Strings::convert(match[2], numberYPixels);
  }
  if (numberXPixels == 0 || numberYPixels == 0)
    g_log.notice() << "Could not read in the number of pixels!" << std::endl;

  // We no longer read from the meta data because that data is wrong
  // from_element<int>(numberXPixels, sasEntryElem, "Number_of_X_Pixels",
  // fileName);
  // from_element<int>(numberYPixels, sasEntryElem, "Number_of_Y_Pixels",
  // fileName);

  // Store sample-detector distance
  declareProperty("SampleDetectorDistance", distance,
                  Kernel::Direction::Output);

  // Create the output workspace

  // Number of bins: we use a single dummy TOF bin
  int nBins = 1;
  // Number of detectors: should be pulled from the geometry description. Use
  // detector pixels for now.
  // The number of spectram also includes the monitor and the timer.
  int numSpectra = numberXPixels * numberYPixels + LoadSpice2D::nMonitors;

  DataObjects::Workspace2D_sptr ws =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", numSpectra,
                                                   nBins + 1, nBins));
  ws->setTitle(wsTitle);
  ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Wavelength");
  ws->setYUnit("");
  API::Workspace_sptr workspace =
      boost::static_pointer_cast<API::Workspace>(ws);
  setProperty("OutputWorkspace", workspace);

  // Parse out each pixel. Pixels can be separated by white space, a tab, or an
  // end-of-line character
  Poco::StringTokenizer pixels(data_str, " \n\t",
                               Poco::StringTokenizer::TOK_TRIM |
                                   Poco::StringTokenizer::TOK_IGNORE_EMPTY);
  Poco::StringTokenizer::Iterator pixel = pixels.begin();

  // Check that we don't keep within the size of the workspace
  size_t pixelcount = pixels.count();
  if (pixelcount != static_cast<size_t>(numberXPixels * numberYPixels)) {
    throw Kernel::Exception::FileError("Inconsistent data set: "
                                       "There were more data pixels found than "
                                       "declared in the Spice XML meta-data.",
                                       fileName);
  }
  if (numSpectra == 0) {
    throw Kernel::Exception::FileError(
        "Empty data set: the data file has no pixel data.", fileName);
  }

  // Go through all detectors/channels
  int ipixel = 0;

  // Store monitor count
  store_value(ws, ipixel++, monitorCounts,
              monitorCounts > 0 ? sqrt(monitorCounts) : 0.0, wavelength,
              dwavelength);

  // Store counting time
  store_value(ws, ipixel++, countingTime, 0.0, wavelength, dwavelength);

  // Store detector pixels
  while (pixel != pixels.end()) {
    // int ix = ipixel%npixelsx;
    // int iy = (int)ipixel/npixelsx;

    // Get the count value and assign it to the right bin
    double count = 0.0;
    from_string<double>(count, *pixel, std::dec);

    // Data uncertainties, computed according to the HFIR/IGOR reduction code
    // The following is what I would suggest instead...
    // error = count > 0 ? sqrt((double)count) : 0.0;
    double error = sqrt(0.5 + fabs(count - 0.5));

    store_value(ws, ipixel, count, error, wavelength, dwavelength);

    ++pixel;
    ipixel++;
  }

  // run load instrument
  runLoadInstrument(instrument, ws);
  runLoadMappingTable(ws, numberXPixels, numberYPixels);

  // Set the run properties
  ws->mutableRun().addProperty("sample-detector-distance", distance, "mm",
                               true);
  ws->mutableRun().addProperty("beam-trap-diameter", beam_trap_diam, "mm",
                               true);
  ws->mutableRun().addProperty("number-of-guides", nguides, true);
  ws->mutableRun().addProperty("source-sample-distance", source_distance, "mm",
                               true);
  ws->mutableRun().addProperty("source-aperture-diameter", source_apert, "mm",
                               true);
  ws->mutableRun().addProperty("sample-aperture-diameter", sample_apert, "mm",
                               true);
  ws->mutableRun().addProperty("sample-thickness", sample_thickness, "cm",
                               true);
  ws->mutableRun().addProperty("wavelength", wavelength, "Angstrom", true);
  ws->mutableRun().addProperty("wavelength-spread", dwavelength, "Angstrom",
                               true);
  ws->mutableRun().addProperty("timer", countingTime, "sec", true);
  ws->mutableRun().addProperty("monitor", monitorCounts, "", true);
  ws->mutableRun().addProperty("start_time", start_time, "", true);
  ws->mutableRun().addProperty("run_start", start_time, "", true);

  // Move the detector to the right position
  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");

  // Finding the name of the detector object.
  std::string detID =
      ws->getInstrument()->getStringParameter("detector-name")[0];

  g_log.information("Moving " + detID);
  try {
    mover->setProperty<API::MatrixWorkspace_sptr>("Workspace", ws);
    mover->setProperty("ComponentName", detID);
    mover->setProperty("Z", distance / 1000.0);
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
void
LoadSpice2D::runLoadInstrument(const std::string &inst_name,
                               DataObjects::Workspace2D_sptr localWorkspace) {

  API::IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", inst_name);
    loadInst->setProperty<API::MatrixWorkspace_sptr>("Workspace",
                                                     localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", false);
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
void
LoadSpice2D::runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace,
                                 int nxbins, int nybins) {
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

  // Generate mapping of detector/channel IDs to spectrum ID

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
