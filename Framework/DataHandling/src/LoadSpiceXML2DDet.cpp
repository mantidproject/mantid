#include "MantidDataHandling/LoadSpiceXML2DDet.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string.hpp>

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/SAX/InputSource.h>

#include <algorithm>
#include <fstream>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::API;
using namespace Mantid::Kernel;

DECLARE_ALGORITHM(LoadSpiceXML2DDet)

const char STRING = 's';
const char FLOAT32 = 'f';
const char INT32 = 'i';

//----------------------------------------------------------------------------------------------
/** Constructor for SpiceXMLNode
 * @brief SpiceXMLNode::SpiceXMLNode
 * @param nodename
 */
SpiceXMLNode::SpiceXMLNode(const std::string &nodename)
    : m_value(""), m_unit(""), m_typechar('s'), m_typefullname("") {
  m_name = nodename;
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SpiceXMLNode::~SpiceXMLNode() {}

//----------------------------------------------------------------------------------------------
/** Set node value in string format
 * @brief SpiceXMLNode::setValue
 * @param strvalue
 */
void SpiceXMLNode::setValue(const std::string &strvalue) { m_value = strvalue; }

//----------------------------------------------------------------------------------------------
/** Set XML node parameters
 * @brief SpiceXMLNode::setValues
 * @param nodetype
 * @param nodeunit
 * @param nodedescription
 */
void SpiceXMLNode::setParameters(const std::string &nodetype,
                                 const std::string &nodeunit,
                                 const std::string &nodedescription) {
  // data type
  if (nodetype.compare("FLOAT32") == 0) {
    m_typefullname = nodetype;
    m_typechar = FLOAT32;
  } else if (nodetype.compare("INT32") == 0) {
    m_typefullname = nodetype;
    m_typechar = INT32;
  }

  // unit
  if (nodeunit.size() > 0) {
    m_unit = nodeunit;
  }

  // description
  if (nodedescription.size() > 0)
    m_description = nodedescription;

  return;
}

//----------------------------------------------------------------------------------------------
/** Check whether XML has unit set
 */
bool SpiceXMLNode::hasUnit() const { return (m_unit.size() > 0); }

//----------------------------------------------------------------------------------------------
/** Check whether XML node has value set
 * @brief SpiceXMLNode::hasValue
 * @return
 */
bool SpiceXMLNode::hasValue() const { return (m_value.size() > 0); }

//----------------------------------------------------------------------------------------------
/** Is this node of string type?
 * @brief SpiceXMLNode::isString
 * @return
 */
bool SpiceXMLNode::isString() const { return (m_typechar == STRING); }

//----------------------------------------------------------------------------------------------
/** Is this node of integer type?
 * @brief SpiceXMLNode::isInteger
 * @return
 */
bool SpiceXMLNode::isInteger() const { return (m_typechar == INT32); }

//----------------------------------------------------------------------------------------------
/** Is this node of double type?
 * @brief SpiceXMLNode::isDouble
 * @return
 */
bool SpiceXMLNode::isDouble() const { return (m_typechar == FLOAT32); }

//----------------------------------------------------------------------------------------------
/** Get name of XML node
 * @brief SpiceXMLNode::getName
 * @return
 */
const std::string SpiceXMLNode::getName() const { return m_name; }

//----------------------------------------------------------------------------------------------
/** Get unit of XML node
 * @brief SpiceXMLNode::getUnit
 * @return
 */
const std::string SpiceXMLNode::getUnit() const { return m_unit; }

//----------------------------------------------------------------------------------------------
/** Get node's description
 * @brief SpiceXMLNode::getDescription
 * @return
 */
const std::string SpiceXMLNode::getDescription() const { return m_description; }

//----------------------------------------------------------------------------------------------
/** Get node's value in string
 * @brief SpiceXMLNode::getValue
 * @return
 */
const std::string SpiceXMLNode::getValue() const { return m_value; }

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadSpiceXML2DDet::LoadSpiceXML2DDet()
    : m_detXMLFileName(), m_detXMLNodeName(), m_numPixelX(0), m_numPixelY(0),
      m_loadInstrument(false), m_detSampleDistanceShift(0.0),
      m_hasScanTable(false), m_ptNumber4Log(0), m_idfFileName() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadSpiceXML2DDet::~LoadSpiceXML2DDet() {}

//----------------------------------------------------------------------------------------------
const std::string LoadSpiceXML2DDet::name() const {
  return "LoadSpiceXML2DDet";
}

//----------------------------------------------------------------------------------------------
int LoadSpiceXML2DDet::version() const { return 1; }

//----------------------------------------------------------------------------------------------
const std::string LoadSpiceXML2DDet::category() const {
  return "DataHandling\\XML";
}

//----------------------------------------------------------------------------------------------
const std::string LoadSpiceXML2DDet::summary() const {
  return "Load 2-dimensional detector data file in XML format from SPICE. ";
}

//----------------------------------------------------------------------------------------------
/** Declare properties
 * @brief LoadSpiceXML2DDet::init
 */
void LoadSpiceXML2DDet::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::FileAction::Load,
                                ".xml"),
      "XML file name for one scan including 2D detectors counts from SPICE");

  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "",
                                                      Direction::Output),
      "Name of output matrix workspace. "
      "Output workspace will be an X by Y Workspace2D if instrument "
      "is not loaded. ");

  declareProperty(
      "DetectorLogName", "Detector",
      "Log name (i.e., XML node name) for detector counts in XML file."
      "By default, the name is 'Detector'");

  declareProperty(
      make_unique<ArrayProperty<size_t>>("DetectorGeometry"),
      "A size-2 unsigned integer array [X, Y] for detector geometry. "
      "Such that the detector contains X x Y pixels.");

  declareProperty(
      "LoadInstrument", true,
      "Flag to load instrument to output workspace. "
      "HFIR's HB3A will be loaded if InstrumentFileName is not specified.");

  declareProperty(
      make_unique<FileProperty>("InstrumentFilename", "",
                                FileProperty::OptionalLoad, ".xml"),
      "The filename (including its full or relative path) of an instrument "
      "definition file. The file extension must either be .xml or .XML when "
      "specifying an instrument definition file. Note Filename or "
      "InstrumentName must be specified but not both.");

  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(
          "SpiceTableWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Name of TableWorkspace loaded from SPICE scan file by LoadSpiceAscii.");

  declareProperty("PtNumber", 0,
                  "Pt. value for the row to get sample log from. ");

  declareProperty(
      "ShiftedDetectorDistance", 0.,
      "Amount of shift of the distance between source and detector centre."
      "It is used to apply instrument calibration.");
}

//----------------------------------------------------------------------------------------------
/** Process inputs arguments
 * @brief processInputs
 */
void LoadSpiceXML2DDet::processInputs() {
  m_detXMLFileName = getPropertyValue("Filename");
  m_detXMLNodeName = getPropertyValue("DetectorLogName");
  std::vector<size_t> vec_pixelgeom = getProperty("DetectorGeometry");
  if (vec_pixelgeom.size() != 2) {
    throw std::runtime_error("Input pixels geometry is not correct in format.");
  }
  m_numPixelX = vec_pixelgeom[0];
  m_numPixelY = vec_pixelgeom[1];

  m_loadInstrument = getProperty("LoadInstrument");

  m_idfFileName = getPropertyValue("InstrumentFilename");
  m_detSampleDistanceShift = getProperty("ShiftedDetectorDistance");

  // Retreive sample environment data from SPICE scan table workspace
  std::string spicetablewsname = getPropertyValue("SpiceTableWorkspace");
  if (spicetablewsname.size() > 0)
    m_hasScanTable = true;
  else
    m_hasScanTable = false;

  m_ptNumber4Log = getProperty("PtNumber");

  return;
}

//----------------------------------------------------------------------------------------------
/** Set up sample logs especially 2theta and diffr for loading instrument
 * @brief LoadSpiceXML2DDet::setupSampleLogs
 * @param outws :: workspace to have sample logs to set up
 * @return
 */
bool LoadSpiceXML2DDet::setupSampleLogs(API::MatrixWorkspace_sptr outws) {
  // With given spice scan table, 2-theta is read from there.
  if (m_hasScanTable) {
    ITableWorkspace_sptr spicetablews = getProperty("SpiceTableWorkspace");
    setupSampleLogFromSpiceTable(outws, spicetablews, m_ptNumber4Log);
  }

  // Process 2theta
  bool return_true = true;
  if (!outws->run().hasProperty("2theta") &&
      outws->run().hasProperty("_2theta")) {
    // Set up 2theta if it is not set up yet
    Kernel::DateAndTime anytime(1000);
    double logvalue =
        atof(outws->run().getProperty("_2theta")->value().c_str());
    TimeSeriesProperty<double> *newlogproperty =
        new TimeSeriesProperty<double>("2theta");
    newlogproperty->addValue(anytime, logvalue);
    outws->mutableRun().addProperty(newlogproperty);
    g_log.information() << "Set 2theta from _2theta (as XML node) with value "
                        << logvalue << "\n";
  } else if (!outws->run().hasProperty("2theta") &&
             !outws->run().hasProperty("_2theta")) {
    // Neither 2theta nor _2theta
    g_log.warning("No 2theta is set up for loading instrument.");
    return_true = false;
  }

  // set up Sample-detetor distance calibration
  Kernel::DateAndTime anytime(1000);
  double sampledetdistance = m_detSampleDistanceShift;
  TimeSeriesProperty<double> *distproperty =
      new TimeSeriesProperty<double>("diffr");
  distproperty->addValue(anytime, sampledetdistance);
  outws->mutableRun().addProperty(distproperty);

  return return_true;
}

//----------------------------------------------------------------------------------------------
/** Main execution
 * @brief LoadSpiceXML2DDet::exec
 */
void LoadSpiceXML2DDet::exec() {
  // Load input
  processInputs();

  // Parse detector XML file
  std::vector<SpiceXMLNode> vec_xmlnode = parseSpiceXML(m_detXMLFileName);

  // Create output workspace
  MatrixWorkspace_sptr outws;
  outws = createMatrixWorkspace(vec_xmlnode, m_numPixelX, m_numPixelY,
                                m_detXMLNodeName, m_loadInstrument);

  // Set up log for loading instrument
  bool can_set_instrument = setupSampleLogs(outws);

  if (m_loadInstrument && can_set_instrument) {
    loadInstrument(outws, m_idfFileName);
    double wavelength;
    bool has_wavelength = getHB3AWavelength(outws, wavelength);
    if (has_wavelength) {
      setXtoLabQ(outws, wavelength);
    }
  }

  setProperty("OutputWorkspace", outws);
}

//----------------------------------------------------------------------------------------------
/** Parse SPICE XML file for one Pt./measurement
 * @brief LoadSpiceXML2DDet::parseSpiceXML
 * @param xmlfilename :: name of the XML file to parse
 * @return vector of SpiceXMLNode containing information in XML file
 */
std::vector<SpiceXMLNode>
LoadSpiceXML2DDet::parseSpiceXML(const std::string &xmlfilename) {
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
    // const Poco::XML::XMLString nodevalue = pNode->nodeValue();

    // get number of children
    size_t numchildren = pNode->childNodes()->length();
    if (numchildren > 1) {
      g_log.debug() << "Parent node " << nodename << " has " << numchildren
                    << " children."
                    << "\n";
      if (nodename.compare("SPICErack") == 0) {
        // SPICErack is the main parent node.  start_time and end_time are there
        unsigned long numattr = pNode->attributes()->length();
        for (unsigned long j = 0; j < numattr; ++j) {
          std::string attname = pNode->attributes()->item(j)->nodeName();
          std::string attvalue = pNode->attributes()->item(j)->innerText();
          SpiceXMLNode xmlnode(attname);
          xmlnode.setValue(attvalue);
          vecspicenode.push_back(xmlnode);
          g_log.debug() << "SPICErack attribute " << j << " Name = " << attname
                        << ", Value = " << attvalue << "\n";
        }
      }

    } else if (numchildren == 1) {
      std::string innertext = pNode->innerText();
      unsigned long numattr = pNode->attributes()->length();
      g_log.debug() << "  Child node " << nodename << "'s attributes: "
                    << "\n";

      SpiceXMLNode xmlnode(nodename);
      std::string nodetype("");
      std::string nodeunit("");
      std::string nodedescription("");

      for (unsigned long j = 0; j < numattr; ++j) {
        std::string atttext = pNode->attributes()->item(j)->innerText();
        std::string attname = pNode->attributes()->item(j)->nodeName();
        g_log.debug() << "     attribute " << j << " name = " << attname << ", "
                      << "value = " << atttext << "\n";
        if (attname.compare("type") == 0) {
          // type
          nodetype = atttext;
        } else if (attname.compare("unit") == 0) {
          // unit
          nodeunit = atttext;
        } else if (attname.compare("description") == 0) {
          // description
          nodedescription = atttext;
        }
      }
      xmlnode.setParameters(nodetype, nodeunit, nodedescription);
      xmlnode.setValue(innertext);

      vecspicenode.push_back(xmlnode);
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
/** Create MatrixWorkspace from Spice XML file
 * @brief LoadSpiceXML2DDet::createMatrixWorkspace
 * @param vecxmlnode :: vector of SpiceXMLNode obtained from XML file
 * @param numpixelx :: number of pixel in x-direction
 * @param numpixely :: number of pixel in y-direction
 * @param detnodename :: the XML node's name for detector counts.
 * @param loadinstrument :: flag to load instrument to output workspace or not.
 * @return
 */
MatrixWorkspace_sptr LoadSpiceXML2DDet::createMatrixWorkspace(
    const std::vector<SpiceXMLNode> &vecxmlnode, const size_t &numpixelx,
    const size_t &numpixely, const std::string &detnodename,
    const bool &loadinstrument) {

  // Create matrix workspace
  MatrixWorkspace_sptr outws;

  if (loadinstrument) {
    size_t numspec = numpixelx * numpixely;
    outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", numspec, 2, 1));
  } else {
    outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", numpixely, numpixelx,
                                            numpixelx));
  }

  // Go through all XML nodes to process
  size_t numxmlnodes = vecxmlnode.size();
  bool parsedDet = false;
  for (size_t n = 0; n < numxmlnodes; ++n) {
    // Process node for detector's count
    const SpiceXMLNode &xmlnode = vecxmlnode[n];
    if (xmlnode.getName().compare(detnodename) == 0) {
      // Get node value string (256x256 as a whole)
      const std::string detvaluestr = xmlnode.getValue();

      // Split
      std::vector<std::string> vecLines;
      boost::split(vecLines, detvaluestr, boost::algorithm::is_any_of("\n"));
      g_log.debug() << "There are " << vecLines.size() << " lines"
                    << "\n";

      // XML file records data in the order of column-major
      size_t icol = 0;
      for (size_t i = 0; i < vecLines.size(); ++i) {
        std::string &line = vecLines[i];

        // Skip empty line
        if (line.size() == 0) {
          g_log.debug() << "\tFound empty Line at " << i << "\n";
          continue;
        }

        // Check whether it exceeds boundary
        if (icol == numpixelx) {
          std::stringstream errss;
          errss << "Number of non-empty rows (" << icol + 1
                << ") in detector data "
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
          errss << "Row " << icol << " contains " << veccounts.size()
                << " items other than " << numpixely
                << " counts specified by user.";
          throw std::runtime_error(errss.str());
        }

        // scan per row
        for (size_t j_row = 0; j_row < veccounts.size(); ++j_row) {
          double counts = atof(veccounts[j_row].c_str());

          if (loadinstrument) {
            size_t wsindex = j_row * numpixely + icol;
            // size_t wsindex = icol * numpixelx + j_row;
            outws->dataX(wsindex)[0] = static_cast<double>(wsindex);
            outws->dataY(wsindex)[0] = counts;
            if (counts > 0)
              outws->dataE(wsindex)[0] = sqrt(counts);
            else
              outws->dataE(wsindex)[0] = 1.0;

          } else {
            outws->dataX(j_row)[icol] = static_cast<double>(j_row);
            outws->dataY(j_row)[icol] = counts;
            if (counts > 0)
              outws->dataE(j_row)[icol] = sqrt(counts);
            else
              outws->dataE(j_row)[icol] = 1.0;
          }
        }

        // Update irow
        icol += 1;
      }

      // Set flag
      parsedDet = true;
    } else {
      // Parse to log: because there is no start time.  so all logs are single
      // value type
      const std::string nodename = xmlnode.getName();
      const std::string nodevalue = xmlnode.getValue();
      if (xmlnode.isDouble()) {
        double dvalue = atof(nodevalue.c_str());
        outws->mutableRun().addProperty(
            new PropertyWithValue<double>(nodename, dvalue));
        g_log.debug() << "Log name / xml node : " << xmlnode.getName()
                      << " (double) value = " << dvalue << "\n";
      } else if (xmlnode.isInteger()) {
        int ivalue = atoi(nodevalue.c_str());
        outws->mutableRun().addProperty(
            new PropertyWithValue<int>(nodename, ivalue));
        g_log.debug() << "Log name / xml node : " << xmlnode.getName()
                      << " (int) value = " << ivalue << "\n";
      } else {
        outws->mutableRun().addProperty(
            new PropertyWithValue<std::string>(nodename, nodevalue));
        g_log.debug() << "Log name / xml node : " << xmlnode.getName()
                      << " (string) value = " << nodevalue << "\n";
      }
    }
  }

  // Raise exception if no detector node is found
  if (!parsedDet) {
    std::stringstream errss;
    errss << "Unable to find an XML node of name " << detnodename
          << ". Unable to load 2D detector XML file.";
    throw std::runtime_error(errss.str());
  }

  return outws;
}

//----------------------------------------------------------------------------------------------
/** Set up sample logs from table workspace loaded where SPICE data file is
 * loaded
 * @brief LoadSpiceXML2DDet::setupSampleLogFromSpiceTable
 * @param matrixws
 * @param spicetablews
 * @param ptnumber
 */
void LoadSpiceXML2DDet::setupSampleLogFromSpiceTable(
    MatrixWorkspace_sptr matrixws, ITableWorkspace_sptr spicetablews,
    int ptnumber) {
  size_t numrows = spicetablews->rowCount();
  std::vector<std::string> colnames = spicetablews->getColumnNames();
  // FIXME - Shouldn't give a better value?
  Kernel::DateAndTime anytime(1000);

  bool foundlog = false;
  for (size_t ir = 0; ir < numrows; ++ir) {
    // loop over the table workspace to find the row of the spcified pt number
    int localpt = spicetablews->cell<int>(ir, 0);
    if (localpt != ptnumber)
      continue;

    // set the properties to matrix workspace including all columns
    for (size_t ic = 1; ic < colnames.size(); ++ic) {
      double logvalue = spicetablews->cell<double>(ir, ic);
      std::string &logname = colnames[ic];
      auto newlogproperty = new TimeSeriesProperty<double>(logname);
      newlogproperty->addValue(anytime, logvalue);
      matrixws->mutableRun().addProperty(newlogproperty);
    }

    // Break as the experiment pointer is found
    foundlog = true;
    break;
  }

  if (!foundlog)
    g_log.warning() << "Pt. " << ptnumber
                    << " is not found.  Log is not loaded to output workspace."
                    << "\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Get wavelength if the instrument is HB3A
 * @brief LoadSpiceXML2DDet::getHB3AWavelength
 * @param dataws
 * @param wavelength
 * @return
 */
bool LoadSpiceXML2DDet::getHB3AWavelength(MatrixWorkspace_sptr dataws,
                                          double &wavelength) {
  bool haswavelength(false);
  wavelength = -1.;

  // FIXME - Now it only search for _m1.  In future,
  //         it is better to searc both m1 and _m1

  if (dataws->run().hasProperty("_m1")) {
    g_log.notice("[DB] Data workspace has property _m1!");
    Kernel::TimeSeriesProperty<double> *ts =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
            dataws->run().getProperty("_m1"));

    if (ts && ts->size() > 0) {
      double m1pos = ts->valuesAsVector()[0];
      if (fabs(m1pos - (-25.870000)) < 0.2) {
        wavelength = 1.003;
        haswavelength = true;
      } else if (fabs(m1pos - (-39.17)) < 0.2) {
        wavelength = 1.5424;
        haswavelength = true;
      } else {
        g_log.warning() << "m1 position " << m1pos
                        << " does not have defined mapping to "
                        << "wavelength."
                        << "\n";
      }
    } else if (!ts) {
      g_log.warning("Log _m1 is not TimeSeriesProperty.  Treat it as a single "
                    "value property.");
      double m1pos = atof(dataws->run().getProperty("_m1")->value().c_str());
      if (fabs(m1pos - (-25.870000)) < 0.2) {
        wavelength = 1.003;
        haswavelength = true;
      } else if (fabs(m1pos - (-39.17)) < 0.2) {
        wavelength = 1.5424;
        haswavelength = true;
      } else {
        g_log.warning() << "m1 position " << m1pos
                        << " does not have defined mapping to "
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

//----------------------------------------------------------------------------------------------
/** Set x axis to momentum (lab frame Q)
 * @brief LoadSpiceXML2DDet::setXtoLabQ
 * @param dataws
 * @param wavelength
 */
void LoadSpiceXML2DDet::setXtoLabQ(API::MatrixWorkspace_sptr dataws,
                                   const double &wavelength) {

  size_t numspec = dataws->getNumberHistograms();
  for (size_t iws = 0; iws < numspec; ++iws) {
    double ki = 2. * M_PI / wavelength;
    dataws->dataX(iws)[0] = ki;
    dataws->dataX(iws)[1] = ki + 0.00001;
  }

  dataws->getAxis(0)->setUnit("Momentum");

  return;
}

//----------------------------------------------------------------------------------------------
/** Load instrument
 * @brief LoadSpiceXML2DDet::loadInstrument
 * @param matrixws
 * @param idffilename
 */
void LoadSpiceXML2DDet::loadInstrument(API::MatrixWorkspace_sptr matrixws,
                                       const std::string &idffilename) {
  // load instrument
  API::IAlgorithm_sptr loadinst = createChildAlgorithm("LoadInstrument");
  loadinst->initialize();
  loadinst->setProperty("Workspace", matrixws);
  if (idffilename.size() > 0) {
    loadinst->setProperty("Filename", idffilename);
  } else
    loadinst->setProperty("InstrumentName", "HB3A");
  loadinst->setProperty("RewriteSpectraMap",
                        Mantid::Kernel::OptionalBool(true));
  loadinst->execute();
  if (loadinst->isExecuted())
    matrixws = loadinst->getProperty("Workspace");
  else
    g_log.error("Unable to load instrument to output workspace");

  return;
}

} // namespace DataHandling
} // namespace Mantid
