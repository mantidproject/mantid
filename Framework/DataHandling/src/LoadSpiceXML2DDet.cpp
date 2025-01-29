// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSpiceXML2DDet.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string.hpp>

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/SAX/InputSource.h>

#include <algorithm>
#include <fstream>
#include <utility>

using namespace std;

namespace Mantid::DataHandling {

using namespace Mantid::API;
using namespace Mantid::Kernel;

DECLARE_ALGORITHM(LoadSpiceXML2DDet)

const char STRING = 's';
const char FLOAT32 = 'f';
const char INT32 = 'i';

/** Constructor for SpiceXMLNode
 * @brief SpiceXMLNode::SpiceXMLNode
 * @param nodename
 */
SpiceXMLNode::SpiceXMLNode(std::string nodename) : m_name{std::move(nodename)}, m_typechar('s') {}

/** Set node value in string format
 * @brief SpiceXMLNode::setValue
 * @param strvalue
 */
void SpiceXMLNode::setValue(const std::string &strvalue) { m_value = strvalue; }

/** Set XML node parameters
 * @brief SpiceXMLNode::setValues
 * @param nodetype
 * @param nodeunit
 * @param nodedescription
 */
void SpiceXMLNode::setParameters(const std::string &nodetype, const std::string &nodeunit,
                                 const std::string &nodedescription) {
  // data type
  if (nodetype == "FLOAT32") {
    m_typefullname = nodetype;
    m_typechar = FLOAT32;
  } else if (nodetype == "INT32") {
    m_typefullname = nodetype;
    m_typechar = INT32;
  }

  // unit
  if (!nodeunit.empty()) {
    m_unit = nodeunit;
  }

  // description
  if (!nodedescription.empty())
    m_description = nodedescription;
}

/** Check whether XML has unit set
 */
bool SpiceXMLNode::hasUnit() const { return (!m_unit.empty()); }

/** Check whether XML node has value set
 * @brief SpiceXMLNode::hasValue
 * @return
 */
bool SpiceXMLNode::hasValue() const { return (!m_value.empty()); }

/** Is this node of string type?
 * @brief SpiceXMLNode::isString
 * @return
 */
bool SpiceXMLNode::isString() const { return (m_typechar == STRING); }

/** Is this node of integer type?
 * @brief SpiceXMLNode::isInteger
 * @return
 */
bool SpiceXMLNode::isInteger() const { return (m_typechar == INT32); }

/** Is this node of double type?
 * @brief SpiceXMLNode::isDouble
 * @return
 */
bool SpiceXMLNode::isDouble() const { return (m_typechar == FLOAT32); }

/** Get name of XML node
 * @brief SpiceXMLNode::getName
 * @return
 */
const std::string &SpiceXMLNode::getName() const { return m_name; }

/** Get unit of XML node
 * @brief SpiceXMLNode::getUnit
 * @return
 */
const std::string &SpiceXMLNode::getUnit() const { return m_unit; }

/** Get node's description
 * @brief SpiceXMLNode::getDescription
 * @return
 */
const std::string &SpiceXMLNode::getDescription() const { return m_description; }

/** Get node's value in string
 * @brief SpiceXMLNode::getValue
 * @return
 */
const std::string &SpiceXMLNode::getValue() const { return m_value; }

/** Constructor
 */
LoadSpiceXML2DDet::LoadSpiceXML2DDet()
    : m_detXMLFileName(), m_detXMLNodeName(), m_numPixelX(0), m_numPixelY(0), m_loadInstrument(false),
      m_detSampleDistanceShift(0.0), m_hasScanTable(false), m_ptNumber4Log(0), m_idfFileName() {}

/** Destructor
 */
LoadSpiceXML2DDet::~LoadSpiceXML2DDet() = default;

const std::string LoadSpiceXML2DDet::name() const { return "LoadSpiceXML2DDet"; }

int LoadSpiceXML2DDet::version() const { return 1; }

const std::string LoadSpiceXML2DDet::category() const { return "DataHandling\\XML"; }

const std::string LoadSpiceXML2DDet::summary() const {
  return "Load 2-dimensional detector data file in XML format from SPICE. ";
}

/** Declare properties
 * @brief LoadSpiceXML2DDet::init
 */
void LoadSpiceXML2DDet::init() {
  std::vector<std::string> exts;
  exts.emplace_back(".xml");
  exts.emplace_back(".bin");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::FileAction::Load, exts),
                  "XML file name for one scan including 2D detectors counts from SPICE");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of output matrix workspace. "
                  "Output workspace will be an X by Y Workspace2D if instrument "
                  "is not loaded. ");

  declareProperty("DetectorLogName", "Detector",
                  "Log name (i.e., XML node name) for detector counts in XML file."
                  "By default, the name is 'Detector'");

  declareProperty(std::make_unique<ArrayProperty<size_t>>("DetectorGeometry"),
                  "A size-2 unsigned integer array [X, Y] for detector geometry. "
                  "Such that the detector contains X x Y pixels."
                  "If the input data is a binary file, input for DetectorGeometry will be "
                  "overridden "
                  "by detector geometry specified in the binary file");

  declareProperty("LoadInstrument", true,
                  "Flag to load instrument to output workspace. "
                  "HFIR's HB3A will be loaded if InstrumentFileName is not specified.");

  declareProperty(std::make_unique<FileProperty>("InstrumentFilename", "", FileProperty::OptionalLoad, ".xml"),
                  "The filename (including its full or relative path) of an instrument "
                  "definition file. The file extension must either be .xml or .XML when "
                  "specifying an instrument definition file. Note Filename or "
                  "InstrumentName must be specified but not both.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("SpiceTableWorkspace", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "Name of TableWorkspace loaded from SPICE scan file by LoadSpiceAscii.");

  declareProperty("PtNumber", 0, "Pt. value for the row to get sample log from. ");

  declareProperty("UserSpecifiedWaveLength", EMPTY_DBL(),
                  "User can specify the wave length of the instrument if it is "
                  "drifted from the designed value."
                  "It happens often.");

  declareProperty("ShiftedDetectorDistance", 0.,
                  "Amount of shift of the distance between source and detector centre."
                  "It is used to apply instrument calibration.");

  declareProperty("DetectorCenterXShift", 0.0,
                  "The amount of shift of "
                  "detector center along X "
                  "direction in the unit meter.");

  declareProperty("DetectorCenterYShift", 0.0,
                  "The amount of shift of "
                  "detector center along Y "
                  "direction in the unit meter.");
}

/** Process inputs arguments
 * @brief processInputs
 */
void LoadSpiceXML2DDet::processInputs() {
  m_detXMLFileName = getPropertyValue("Filename");
  m_detXMLNodeName = getPropertyValue("DetectorLogName");
  std::vector<size_t> vec_pixelgeom = getProperty("DetectorGeometry");
  if (vec_pixelgeom.size() == 2) {
    m_numPixelX = vec_pixelgeom[0];
    m_numPixelY = vec_pixelgeom[1];
  } else if (vec_pixelgeom.empty()) {
    m_numPixelX = 0;
    m_numPixelY = 0;
  } else {
    throw std::runtime_error("Input pixels geometry is not correct in format. "
                             "It either has 2 integers or left empty to get "
                             "determined automatically.");
  }
  g_log.debug() << "User input poixels numbers: " << m_numPixelX << ", " << m_numPixelY << "\n";

  m_loadInstrument = getProperty("LoadInstrument");

  m_idfFileName = getPropertyValue("InstrumentFilename");
  m_detSampleDistanceShift = getProperty("ShiftedDetectorDistance");

  // Retreive sample environment data from SPICE scan table workspace
  std::string spicetablewsname = getPropertyValue("SpiceTableWorkspace");
  if (!spicetablewsname.empty())
    m_hasScanTable = true;
  else
    m_hasScanTable = false;

  m_ptNumber4Log = getProperty("PtNumber");

  m_userSpecifiedWaveLength = getProperty("UserSpecifiedWaveLength");

  m_detXShift = getProperty("DetectorCenterXShift");
  m_detYShift = getProperty("DetectorCenterYShift");
}

/** Set up sample logs especially 2theta and diffr for loading instrument
 * @brief LoadSpiceXML2DDet::setupSampleLogs
 * @param outws :: workspace to have sample logs to set up
 * @return
 */
bool LoadSpiceXML2DDet::setupSampleLogs(const API::MatrixWorkspace_sptr &outws) {
  // With given spice scan table, 2-theta is read from there.
  if (m_hasScanTable) {
    ITableWorkspace_sptr spicetablews = getProperty("SpiceTableWorkspace");
    setupSampleLogFromSpiceTable(outws, spicetablews, m_ptNumber4Log);
  }

  Types::Core::DateAndTime anytime(1000);

  // Process 2theta
  bool return_true = true;
  if (!outws->run().hasProperty("2theta") && outws->run().hasProperty("_2theta")) {
    // Set up 2theta if it is not set up yet
    double logvalue = std::stod(outws->run().getProperty("_2theta")->value());
    auto *newlogproperty = new TimeSeriesProperty<double>("2theta");
    newlogproperty->addValue(anytime, logvalue);
    outws->mutableRun().addProperty(std::move(newlogproperty));
    g_log.information() << "Set 2theta from _2theta (as XML node) with value " << logvalue << "\n";
  } else if (!outws->run().hasProperty("2theta") && !outws->run().hasProperty("_2theta")) {
    // Neither 2theta nor _2theta
    g_log.warning("No 2theta is set up for loading instrument.");
    return_true = false;
  }

  // set up the caibrated detector center to beam
  auto *det_dx = new TimeSeriesProperty<double>("deltax");
  det_dx->addValue(anytime, m_detXShift);
  outws->mutableRun().addProperty(std::move(det_dx));

  auto *det_dy = new TimeSeriesProperty<double>("deltay");
  det_dy->addValue(anytime, m_detYShift);
  outws->mutableRun().addProperty(std::move(det_dy));

  // set up Sample-detetor distance calibration
  double sampledetdistance = m_detSampleDistanceShift;
  auto *distproperty = new TimeSeriesProperty<double>("diffr");
  distproperty->addValue(anytime, sampledetdistance);
  outws->mutableRun().addProperty(std::move(distproperty));

  return return_true;
}

//----------------------------------------------------------------------------------------------
/** Main execution
 * @brief LoadSpiceXML2DDet::exec
 */
void LoadSpiceXML2DDet::exec() {
  // Load input
  processInputs();

  // check the file end
  MatrixWorkspace_sptr outws;
  if (m_detXMLFileName.substr(m_detXMLFileName.find_last_of('.') + 1) == "bin") {
    std::vector<unsigned int> vec_counts = binaryParseIntegers(m_detXMLFileName);
    outws = createMatrixWorkspace(vec_counts);
  } else {
    // Parse detector XML file
    std::vector<SpiceXMLNode> vec_xmlnode = xmlParseSpice(m_detXMLFileName);

    // Create output workspace
    if (m_numPixelX * m_numPixelY > 0)
      outws = xmlCreateMatrixWorkspaceKnownGeometry(vec_xmlnode, m_numPixelX, m_numPixelY, m_detXMLNodeName,
                                                    m_loadInstrument);
    else
      outws = xmlCreateMatrixWorkspaceUnknowGeometry(vec_xmlnode, m_detXMLNodeName, m_loadInstrument);
  }

  // Set up log for loading instrument
  bool can_set_instrument = setupSampleLogs(outws);

  if (m_loadInstrument && can_set_instrument) {
    loadInstrument(outws, m_idfFileName);
    // set wave length to user specified wave length
    double wavelength = m_userSpecifiedWaveLength;
    // if user does not specify wave length then try to get wave length from log
    // sample _m1 (or m1 as well in future)
    bool has_wavelength = !(wavelength == EMPTY_DBL());
    if (!has_wavelength)
      has_wavelength = getHB3AWavelength(outws, wavelength);

    if (has_wavelength) {
      setXtoLabQ(outws, wavelength);
    }
  }

  setProperty("OutputWorkspace", outws);
}

/** Parse SPICE XML file for one Pt./measurement
 * @brief LoadSpiceXML2DDet::parseSpiceXML
 * @param xmlfilename :: name of the XML file to parse
 * @return vector of SpiceXMLNode containing information in XML file
 */
std::vector<SpiceXMLNode> LoadSpiceXML2DDet::xmlParseSpice(const std::string &xmlfilename) {
  // Declare output
  std::vector<SpiceXMLNode> vecspicenode;

  // Open file
  std::ifstream ifs;
  ifs.open(xmlfilename.c_str());
  if (!ifs.is_open()) {
    std::stringstream ess;
    ess << "File " << xmlfilename << " cannot be opened.";
    throw std::runtime_error(ess.str());
  }

  // Parse
  Poco::XML::InputSource src(ifs);

  Poco::XML::DOMParser parser;
  Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);

  // Go though XML
  Poco::XML::NodeIterator nodeIter(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = nodeIter.nextNode();
  while (pNode) {
    const Poco::XML::XMLString nodename = pNode->nodeName();

    // get number of children
    Poco::AutoPtr<Poco::XML::NodeList> children = pNode->childNodes();
    const size_t numchildren = children->length();
    if (numchildren > 1) {
      g_log.debug() << "Parent node " << nodename << " has " << numchildren << " children."
                    << "\n";
      if (nodename == "SPICErack") {
        // SPICErack is the main parent node.  start_time and end_time are there
        Poco::AutoPtr<Poco::XML::NamedNodeMap> attributes = pNode->attributes();
        unsigned long numattr = attributes->length();
        for (unsigned long j = 0; j < numattr; ++j) {
          std::string attname = attributes->item(j)->nodeName();
          std::string attvalue = attributes->item(j)->innerText();
          SpiceXMLNode xmlnode(attname);
          xmlnode.setValue(attvalue);
          vecspicenode.emplace_back(xmlnode);
          g_log.debug() << "SPICErack attribute " << j << " Name = " << attname << ", Value = " << attvalue << "\n";
        }
      }

    } else if (numchildren == 1) {
      std::string innertext = pNode->innerText();
      Poco::AutoPtr<Poco::XML::NamedNodeMap> attributes = pNode->attributes();
      unsigned long numattr = attributes->length();
      g_log.debug() << "  Child node " << nodename << "'s attributes: "
                    << "\n";

      SpiceXMLNode xmlnode(nodename);
      std::string nodetype;
      std::string nodeunit;
      std::string nodedescription;

      for (unsigned long j = 0; j < numattr; ++j) {
        std::string atttext = attributes->item(j)->innerText();
        std::string attname = attributes->item(j)->nodeName();
        g_log.debug() << "     attribute " << j << " name = " << attname << ", "
                      << "value = " << atttext << "\n";
        if (attname == "type") {
          // type
          nodetype = atttext;
        } else if (attname == "unit") {
          // unit
          nodeunit = atttext;
        } else if (attname == "description") {
          // description
          nodedescription = atttext;
        }
      }
      xmlnode.setParameters(nodetype, nodeunit, nodedescription);
      xmlnode.setValue(innertext);

      vecspicenode.emplace_back(xmlnode);
    } else {
      // An unexpected case but no guarantee for not happening
      g_log.error("Funny... No child node.");
    }

    // Move to next node
    pNode = nodeIter.nextNode();
  } // ENDWHILE

  // Close file
  ifs.close();

  return vecspicenode;
}

//----------------------------------------------------------------------------------------------
/// parse binary integer file
std::vector<unsigned int> LoadSpiceXML2DDet::binaryParseIntegers(std::string &binary_file_name) {
  // check binary file size
  ifstream infile(binary_file_name.c_str(), ios::binary);
  streampos begin, end;
  begin = infile.tellg();
  infile.seekg(0, ios::end);
  end = infile.tellg();
  g_log.information() << "File size is: " << (end - begin) << " bytes.\n";

  size_t num_unsigned_int = static_cast<size_t>(end - begin) / sizeof(unsigned int);
  if (num_unsigned_int <= 2)
    throw std::runtime_error("Input binary file size is too small (<= 2 unsigned int)");

  size_t num_dets = num_unsigned_int - 2;
  g_log.information() << "File contains " << num_unsigned_int << " unsigned integers and thus " << num_dets
                      << " detectors.\n";

  // define output vector
  std::vector<unsigned int> vec_counts(num_dets);
  infile.seekg(0, ios::beg);

  // read each integer... time consuming
  //  int max_count = 0;
  // char buffer[sizeof(int)];
  unsigned int buffer;
  unsigned int total_counts(0);

  // read detector size (row and column)
  infile.read((char *)&buffer, sizeof(buffer));
  auto num_rows = static_cast<size_t>(buffer);
  infile.read((char *)&buffer, sizeof(buffer));
  auto num_cols = static_cast<size_t>(buffer);
  if (num_rows * num_cols != num_dets) {
    g_log.error() << "Input binary file " << binary_file_name << " has inconsistent specification "
                  << "on detector size. "
                  << "First 2 unsigned integers are " << num_rows << ", " << num_cols
                  << ", while the detector number specified in the file is " << num_dets << "\n";
    throw std::runtime_error("Input binary file has inconsistent specification on detector size.");
  }

  for (size_t i = 0; i < num_dets; ++i) {
    // infile.read(buffer, sizeof(int));
    infile.read((char *)&buffer, sizeof(buffer));
    vec_counts[i] = buffer;
    total_counts += buffer;
  }

  g_log.information() << "For detector " << num_rows << " x " << num_cols << ", total counts = " << total_counts
                      << "\n";

  return vec_counts;
}

//----------------------------------------------------------------------------------------------
MatrixWorkspace_sptr LoadSpiceXML2DDet::createMatrixWorkspace(const std::vector<unsigned int> &vec_counts) {
  // Create matrix workspace
  size_t numspec = vec_counts.size();
  MatrixWorkspace_sptr outws =
      std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", numspec, 2, 1));

  g_log.information("Workspace created");

  // set up value
  for (size_t i = 0; i < numspec; ++i) {
    outws->mutableX(i)[0] = 0.;
    outws->mutableX(i)[1] = 1;
    auto counts = static_cast<double>(vec_counts[i]);
    outws->mutableY(i)[0] = counts;
    if (counts > 0.5)
      outws->mutableE(i)[0] = sqrt(counts);
    else
      outws->mutableE(i)[0] = 1.0;
  }

  return outws;
}

//-----
/** Create MatrixWorkspace from Spice XML file
 * @brief LoadSpiceXML2DDet::createMatrixWorkspace
 * @param vecxmlnode :: vector of SpiceXMLNode obtained from XML file
 * @param numpixelx :: number of pixel in x-direction
 * @param numpixely :: number of pixel in y-direction
 * @param detnodename :: the XML node's name for detector counts.
 * @param loadinstrument :: flag to load instrument to output workspace or not.
 * @return
 */
MatrixWorkspace_sptr
LoadSpiceXML2DDet::xmlCreateMatrixWorkspaceKnownGeometry(const std::vector<SpiceXMLNode> &vecxmlnode,
                                                         const size_t &numpixelx, const size_t &numpixely,
                                                         const std::string &detnodename, const bool &loadinstrument) {

  // TODO FIXME - If version 2 works, then this version will be discarded

  // Create matrix workspace
  MatrixWorkspace_sptr outws;

  if (loadinstrument) {
    size_t numspec = numpixelx * numpixely;
    outws =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", numspec, 2, 1));
  } else {
    outws = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", numpixely, numpixelx, numpixelx));
  }

  // Go through all XML nodes to process
  size_t numxmlnodes = vecxmlnode.size();
  bool parsedDet = false;
  double max_counts = 0.;
  for (size_t n = 0; n < numxmlnodes; ++n) {
    // Process node for detector's count
    const SpiceXMLNode &xmlnode = vecxmlnode[n];
    if (xmlnode.getName() == detnodename) {
      // Get node value string (256x256 as a whole)
      const std::string detvaluestr = xmlnode.getValue();

      // Split
      std::vector<std::string> vecLines;
      boost::split(vecLines, detvaluestr, boost::algorithm::is_any_of("\n"));
      g_log.debug() << "There are " << vecLines.size() << " lines"
                    << "\n";

      // XML file records data in the order of column-major
      size_t i_col = 0;
      for (size_t i = 0; i < vecLines.size(); ++i) {
        std::string &line = vecLines[i];

        // Skip empty line
        if (line.empty()) {
          g_log.debug() << "\tFound empty Line at " << i << "\n";
          continue;
        }

        // Check whether it exceeds boundary
        if (i_col == numpixelx) {
          std::stringstream errss;
          errss << "Number of non-empty rows (" << i_col + 1 << ") in detector data "
                << "exceeds user defined geometry size " << numpixelx << ".";
          throw std::runtime_error(errss.str());
        }

        // Split line
        std::vector<std::string> veccounts;
        boost::split(veccounts, line, boost::algorithm::is_any_of(" \t"));

        // check number of counts per column should not exceeds number of pixels
        // in Y direction
        if (veccounts.size() != numpixely) {
          std::stringstream errss;
          errss << "[Version 1] Row " << i_col << " contains " << veccounts.size() << " items other than " << numpixely
                << " counts specified by user.";
          throw std::runtime_error(errss.str());
        }

        // scan per column
        for (size_t j_row = 0; j_row < veccounts.size(); ++j_row) {
          double counts = std::stod(veccounts[j_row]);
          size_t rowIndex, columnIndex;

          if (loadinstrument) {
            // the detector ID and ws index are set up in column-major too!
            rowIndex = i_col * numpixelx + j_row;
            columnIndex = 0;
          } else {
            rowIndex = j_row;
            columnIndex = i_col;
          }

          outws->mutableX(rowIndex)[columnIndex] = static_cast<double>(columnIndex);
          outws->mutableY(rowIndex)[columnIndex] = counts;

          if (counts > 0)
            outws->mutableE(rowIndex)[columnIndex] = sqrt(counts);
          else
            outws->mutableE(rowIndex)[columnIndex] = 1.0;

          // record max count
          if (counts > max_counts) {
            max_counts = counts;
          }
        }

        // Update column index (i.e., column number)
        i_col += 1;
      } // END-FOR (i-vec line)

      // Set flag
      parsedDet = true;
    } else {
      // Parse to log: because there is no start time.  so all logs are single
      // value type
      const std::string nodename = xmlnode.getName();
      const std::string nodevalue = xmlnode.getValue();
      if (xmlnode.isDouble()) {
        double dvalue = std::stod(nodevalue);
        outws->mutableRun().addProperty(new PropertyWithValue<double>(nodename, dvalue));
        g_log.debug() << "Log name / xml node : " << xmlnode.getName() << " (double) value = " << dvalue << "\n";
      } else if (xmlnode.isInteger()) {
        int ivalue = std::stoi(nodevalue);
        outws->mutableRun().addProperty(new PropertyWithValue<int>(nodename, ivalue));
        g_log.debug() << "Log name / xml node : " << xmlnode.getName() << " (int) value = " << ivalue << "\n";
      } else {
        std::string str_value(nodevalue);
        if (nodename == "start_time") {
          // replace 2015-01-17 13:36:45 by  2015-01-17T13:36:45
          str_value = nodevalue;
          str_value.replace(10, 1, "T");
          g_log.debug() << "Replace start_time " << nodevalue << " by Mantid time format " << str_value << "\n";
        }
        outws->mutableRun().addProperty(new PropertyWithValue<std::string>(nodename, str_value));
      }
    }
  }

  // Raise exception if no detector node is found
  if (!parsedDet) {
    std::stringstream errss;
    errss << "Unable to find an XML node of name " << detnodename << ". Unable to load 2D detector XML file.";
    throw std::runtime_error(errss.str());
  }

  g_log.notice() << "Maximum detector count on it is " << max_counts << "\n";

  return outws;
}

/** create the output matrix workspace without knowledge of detector geometry
 *
 */
MatrixWorkspace_sptr
LoadSpiceXML2DDet::xmlCreateMatrixWorkspaceUnknowGeometry(const std::vector<SpiceXMLNode> &vecxmlnode,
                                                          const std::string &detnodename, const bool &loadinstrument) {

  // Create matrix workspace
  MatrixWorkspace_sptr outws;

  // Go through all XML nodes to process
  size_t numxmlnodes = vecxmlnode.size();
  bool parsedDet = false;
  double max_counts = 0.;

  // define log value map
  std::map<std::string, std::string> str_log_map;
  std::map<std::string, double> dbl_log_map;
  std::map<std::string, int> int_log_map;

  for (size_t n = 0; n < numxmlnodes; ++n) {
    // Process node for detector's count
    const SpiceXMLNode &xmlnode = vecxmlnode[n];
    if (xmlnode.getName() == detnodename) {
      // Get node value string (256x256 as a whole)
      const std::string detvaluestr = xmlnode.getValue();

      outws = this->xmlParseDetectorNode(detvaluestr, loadinstrument, max_counts);

      // Set flag
      parsedDet = true;
    } else {
      // Parse to log: because there is no start time.  so all logs are single
      // value type
      const std::string nodename = xmlnode.getName();
      const std::string nodevalue = xmlnode.getValue();
      if (xmlnode.isDouble()) {
        double dvalue = std::stod(nodevalue);
        dbl_log_map.emplace(nodename, dvalue);
      } else if (xmlnode.isInteger()) {
        int ivalue = std::stoi(nodevalue);
        int_log_map.emplace(nodename, ivalue);
      } else {
        if (nodename == "start_time") {
          // replace 2015-01-17 13:36:45 by  2015-01-17T13:36:45
          std::string str_value(nodevalue);
          str_value.replace(10, 1, "T");
          g_log.debug() << "Replace start_time " << nodevalue << " by Mantid time format " << str_value << "\n";
          str_log_map.emplace(nodename, str_value);
        } else
          str_log_map.emplace(nodename, nodevalue);
      } // END-IF-ELSE (node value type)
    } // END-IF-ELSE (detector-node or log node)
  } // END-FOR (xml nodes)

  if (outws) {
    // Add the property to output workspace
    for (const auto &log_entry : str_log_map) {
      outws->mutableRun().addProperty(new PropertyWithValue<std::string>(log_entry.first, log_entry.second));
    }
    for (const auto &log_entry : int_log_map) {
      outws->mutableRun().addProperty(new PropertyWithValue<int>(log_entry.first, log_entry.second));
    }
    for (const auto &log_entry : dbl_log_map) {
      outws->mutableRun().addProperty(new PropertyWithValue<double>(log_entry.first, log_entry.second));
    }
  }

  // Raise exception if no detector node is found
  if (!parsedDet) {
    std::stringstream errss;
    errss << "Unable to find an XML node of name " << detnodename << ". Unable to load 2D detector XML file.";
    throw std::runtime_error(errss.str());
  }

  g_log.notice() << "Maximum detector count on it is " << max_counts << "\n";

  return outws;
}

API::MatrixWorkspace_sptr LoadSpiceXML2DDet::xmlParseDetectorNode(const std::string &detvaluestr, bool loadinstrument,
                                                                  double &max_counts) {
  // Split to lines
  std::vector<std::string> vecLines;
  boost::split(vecLines, detvaluestr, boost::algorithm::is_any_of("\n"));
  g_log.debug() << "There are " << vecLines.size() << " lines"
                << "\n";

  // determine the number of pixels at X direction (bear in mind that the XML
  // file records data in column major)
  size_t num_empty_line = 0;
  size_t num_weird_line = 0;
  for (const auto &vecLine : vecLines) {
    if (vecLine.empty())
      ++num_empty_line;
    else if (vecLine.size() < 100)
      ++num_weird_line;
  }
  size_t num_pixel_x = vecLines.size() - num_empty_line - num_weird_line;
  g_log.information() << "There are " << num_empty_line << " lines and " << num_weird_line
                      << " lines are not regular.\n";

  // read the first line to determine the number of pixels at X direction
  size_t first_regular_line = 0;
  if (vecLines[first_regular_line].size() < 100)
    ++first_regular_line;
  std::vector<std::string> veccounts;
  boost::split(veccounts, vecLines[first_regular_line], boost::algorithm::is_any_of(" \t"));
  size_t num_pixel_y = veccounts.size();

  // create output workspace
  MatrixWorkspace_sptr outws;

  if (loadinstrument) {
    size_t numspec = num_pixel_x * num_pixel_y;
    outws =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", numspec, 2, 1));
  } else {
    outws = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", num_pixel_y, num_pixel_x, num_pixel_x));
  }

  // XML file records data in the order of column-major
  // FIXME - This may waste the previous result by parsing first line
  size_t i_col = 0;
  max_counts = 0;
  for (size_t i = first_regular_line; i < vecLines.size(); ++i) {
    std::string &line = vecLines[i];

    // skip lines with length less than 100
    if (line.size() < 100) {
      g_log.debug() << "\tSkipped line of size " << line.size() << " at " << i << "\n";
      continue;
    }

    // Check whether it exceeds boundary
    if (i_col == num_pixel_x) {
      std::stringstream errss;
      errss << "Number of non-empty rows (" << i_col + 1 << ") in detector data "
            << "exceeds user defined geometry size " << num_pixel_x << ".";
      throw std::runtime_error(errss.str());
    }

    boost::split(veccounts, line, boost::algorithm::is_any_of(" \t"));

    // check number of counts per column should not exceeds number of pixels
    // in Y direction
    if (veccounts.size() != num_pixel_y) {
      std::stringstream errss;
      errss << "Row " << i_col << " contains " << veccounts.size() << " items other than " << num_pixel_y
            << " counts specified by user.";
      throw std::runtime_error(errss.str());
    }

    // scan per column
    for (size_t j_row = 0; j_row < veccounts.size(); ++j_row) {
      double counts = std::stod(veccounts[j_row]);
      size_t rowIndex, columnIndex;

      if (loadinstrument) {
        // the detector ID and ws index are set up in column-major too!
        rowIndex = i_col * num_pixel_x + j_row;
        columnIndex = 0;
      } else {
        rowIndex = j_row;
        columnIndex = i_col;
      }

      outws->mutableX(rowIndex)[columnIndex] = static_cast<double>(columnIndex);
      outws->mutableY(rowIndex)[columnIndex] = counts;

      if (counts > 0)
        outws->mutableE(rowIndex)[columnIndex] = sqrt(counts);
      else
        outws->mutableE(rowIndex)[columnIndex] = 1.0;

      // record max count
      if (counts > max_counts) {
        max_counts = counts;
      }
    }

    // Update column index (i.e., column number)
    i_col += 1;
  } // END-FOR (i-vec line)

  return outws;
}

/** Set up sample logs from table workspace loaded where SPICE data file is
 * loaded
 * @brief LoadSpiceXML2DDet::setupSampleLogFromSpiceTable
 * @param matrixws
 * @param spicetablews
 * @param ptnumber
 */
void LoadSpiceXML2DDet::setupSampleLogFromSpiceTable(const MatrixWorkspace_sptr &matrixws,
                                                     const ITableWorkspace_sptr &spicetablews, int ptnumber) {
  size_t numrows = spicetablews->rowCount();
  std::vector<std::string> colnames = spicetablews->getColumnNames();
  // FIXME - Shouldn't give a better value?
  Types::Core::DateAndTime anytime(1000);

  bool foundlog = false;
  for (size_t ir = 0; ir < numrows; ++ir) {
    // loop over the table workspace to find the row of the spcified pt number
    int localpt = spicetablews->cell<int>(ir, 0);
    if (localpt != ptnumber)
      continue;

    // set the properties to matrix workspace including all columns
    for (size_t ic = 1; ic < colnames.size(); ++ic) {
      double logvalue = spicetablews->cell<double>(ir, ic);
      auto newlogproperty = new TimeSeriesProperty<double>(colnames[ic]);
      newlogproperty->addValue(anytime, logvalue);
      matrixws->mutableRun().addProperty(std::move(newlogproperty));
    }

    // Break as the experiment pointer is found
    foundlog = true;
    break;
  }

  if (!foundlog)
    g_log.warning() << "Pt. " << ptnumber << " is not found.  Log is not loaded to output workspace."
                    << "\n";
}

/** Get wavelength if the instrument is HB3A
 * @brief LoadSpiceXML2DDet::getHB3AWavelength
 * @param dataws
 * @param wavelength
 * @return
 */
bool LoadSpiceXML2DDet::getHB3AWavelength(const MatrixWorkspace_sptr &dataws, double &wavelength) {
  bool haswavelength(false);
  wavelength = -1.;

  // FIXME - Now it only search for _m1.  In future,
  //         it is better to searc both m1 and _m1

  if (dataws->run().hasProperty("_m1")) {
    g_log.notice("[DB] Data workspace has property _m1!");
    auto *ts = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(dataws->run().getProperty("_m1"));

    if (ts && ts->size() > 0) {
      double m1pos = ts->valuesAsVector()[0];
      if (fabs(m1pos - (-25.870000)) < 0.2) {
        wavelength = 1.003;
        haswavelength = true;
      } else if (fabs(m1pos - (-39.17)) < 0.2) {
        wavelength = 1.5424;
        haswavelength = true;
      } else {
        g_log.warning() << "m1 position " << m1pos << " does not have defined mapping to "
                        << "wavelength."
                        << "\n";
      }
    } else if (!ts) {
      g_log.warning("Log _m1 is not TimeSeriesProperty.  Treat it as a single "
                    "value property.");
      double m1pos = std::stod(dataws->run().getProperty("_m1")->value());
      if (fabs(m1pos - (-25.870000)) < 0.2) {
        wavelength = 1.003;
        haswavelength = true;
      } else if (fabs(m1pos - (-39.17)) < 0.2) {
        wavelength = 1.5424;
        haswavelength = true;
      } else {
        g_log.warning() << "m1 position " << m1pos << " does not have defined mapping to "
                        << "wavelength."
                        << "\n";
      }
    } else {
      g_log.error("Log _m1 is empty.");
    }
  } else {
    g_log.warning() << "No _m1 log is found."
                    << "\n";
  }

  if (!haswavelength)
    g_log.warning("No wavelength is setup!");
  else
    g_log.notice() << "[DB] Wavelength = " << wavelength << "\n";

  return haswavelength;
}

/** Set x axis to momentum (lab frame Q)
 * @brief LoadSpiceXML2DDet::setXtoLabQ
 * @param dataws
 * @param wavelength
 */
void LoadSpiceXML2DDet::setXtoLabQ(const API::MatrixWorkspace_sptr &dataws, const double &wavelength) {

  size_t numspec = dataws->getNumberHistograms();
  for (size_t iws = 0; iws < numspec; ++iws) {
    double ki = 2. * M_PI / wavelength;
    auto &x = dataws->mutableX(iws);
    x[0] = ki;
    x[1] = ki + 0.00001;
  }

  dataws->getAxis(0)->setUnit("Momentum");
}

/** Load instrument
 * @brief LoadSpiceXML2DDet::loadInstrument
 * @param matrixws
 * @param idffilename
 */
void LoadSpiceXML2DDet::loadInstrument(const API::MatrixWorkspace_sptr &matrixws, const std::string &idffilename) {
  // load instrument
  auto loadinst = createChildAlgorithm("LoadInstrument");
  loadinst->initialize();
  loadinst->setProperty("Workspace", matrixws);
  if (!idffilename.empty()) {
    loadinst->setProperty("Filename", idffilename);
  } else
    loadinst->setProperty("InstrumentName", "HB3A");
  loadinst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
  loadinst->execute();
  if (!loadinst->isExecuted())
    g_log.error("Unable to load instrument to output workspace");
}

} // namespace Mantid::DataHandling
