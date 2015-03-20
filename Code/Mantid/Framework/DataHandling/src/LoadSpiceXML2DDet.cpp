#include "MantidDataHandling/LoadSpiceXML2DDet.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"

#include "Poco/SAX/InputSource.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/AutoPtr.h"

#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"

#include "Poco/DOM/Node.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NamedNodeMap.h"

#include <algorithm>
#include <fstream>
#include <boost/algorithm/string.hpp>

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
LoadSpiceXML2DDet::LoadSpiceXML2DDet() {}

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
const std::string LoadSpiceXML2DDet::category() const { return "DataHandling"; }

//----------------------------------------------------------------------------------------------
const std::string LoadSpiceXML2DDet::summary() const {
  return "Load 2-dimensional detector data file in XML format from SPICE. ";
}

//----------------------------------------------------------------------------------------------
/** Declare properties
 * @brief LoadSpiceXML2DDet::init
 */
void LoadSpiceXML2DDet::init() {
  std::vector<std::string> xmlext;
  xmlext.push_back(".xml");
  declareProperty(
      new FileProperty("Filename", "", FileProperty::FileAction::Load, xmlext),
      "XML file name for one scan including 2D detectors counts from SPICE");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Name of output matrix workspace. ");

  declareProperty("DetectorLogName", "Detector",
                  "Log name for detector counts.");

  declareProperty(
      new ArrayProperty<size_t>("DetectorGeometry"),
      "A size-2 unsigned integer array [X, Y] for detector geometry. "
      "Such that the detector contains X x Y pixels.");
}

//----------------------------------------------------------------------------------------------
/** Main execution
 * @brief LoadSpiceXML2DDet::exec
 */
void LoadSpiceXML2DDet::exec() {
  // Load input
  const std::string xmlfilename = getProperty("Filename");
  const std::string detlogname = getProperty("DetectorLogName");
  std::vector<size_t> vec_pixelgeom = getProperty("DetectorGeometry");
  if (vec_pixelgeom.size() != 2) {
    throw std::runtime_error("Input pixels geometry is not correct in format.");
  }
  size_t numpixelX = vec_pixelgeom[0];
  size_t numpixelY = vec_pixelgeom[1];

  // Parse
  std::vector<SpiceXMLNode> vec_xmlnode;
  parseSpiceXML(xmlfilename, vec_xmlnode);

  // Create output workspace
  MatrixWorkspace_sptr outws =
      createMatrixWorkspace(vec_xmlnode, numpixelX, numpixelY, detlogname);

  setProperty("OutputWorkspace", outws);
}

//----------------------------------------------------------------------------------------------
/** Parse SPICE XML file for one Pt./measurement
 * @brief LoadSpiceXML2DDet::parseSpiceXML
 * @param xmlfilename :: name of the XML file to parse
 * @param vecspicenode :: output vector of SpiceXMLNode containing information
 * in XML file
 */
void LoadSpiceXML2DDet::parseSpiceXML(const std::string &xmlfilename,
                                      std::vector<SpiceXMLNode> &vecspicenode) {
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

  return;
}

//----------------------------------------------------------------------------------------------
/** Create MatrixWorkspace from Spice XML file
 * @brief LoadSpiceXML2DDet::createMatrixWorkspace
 * @param vecxmlnode :: vector of SpiceXMLNode obtained from XML file
 * @param numpixelx :: number of pixel in x-direction
 * @param numpixely :: number of pixel in y-direction
 * @param detnodename :: the XML node's name for detector counts.
 * @return
 */
MatrixWorkspace_sptr LoadSpiceXML2DDet::createMatrixWorkspace(
    const std::vector<SpiceXMLNode> &vecxmlnode, const size_t &numpixelx,
    const size_t &numpixely, const std::string &detnodename) {

  // Create matrix workspace
  MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", numpixely, numpixelx,
                                          numpixelx));

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

      size_t irow = 0;
      for (size_t i = 0; i < vecLines.size(); ++i) {
        std::string &line = vecLines[i];

        // Skip empty line
        if (line.size() == 0) {
          g_log.debug() << "\tFound empty Line at " << i << "\n";
          continue;
        }

        // Check whether it exceeds boundary
        if (irow == numpixely) {
          std::stringstream errss;
          errss << "Number of non-empty rows (" << irow + 1
                << ") in detector data "
                << "exceeds user defined geometry size " << numpixely << ".";
          throw std::runtime_error(errss.str());
        }

        // Split line
        std::vector<std::string> veccounts;
        boost::split(veccounts, line, boost::algorithm::is_any_of(" \t"));

        // check
        if (veccounts.size() != numpixelx) {
          std::stringstream errss;
          errss << "Row " << irow << " contains " << veccounts.size()
                << " items other than " << numpixelx
                << " counts specified by user.";
          throw std::runtime_error(errss.str());
        }

        for (size_t j = 0; j < veccounts.size(); ++j) {
          double y = atof(veccounts[j].c_str());
          outws->dataX(irow)[j] = static_cast<double>(j);
          outws->dataY(irow)[j] = y;
          if (y > 0)
            outws->dataE(irow)[j] = sqrt(y);
          else
            outws->dataE(irow)[j] = 1.0;
        }

        // Update irow
        irow += 1;
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

} // namespace DataHandling
} // namespace Mantid
