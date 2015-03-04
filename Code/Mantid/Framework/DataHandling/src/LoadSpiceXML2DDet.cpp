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

SpiceXMLNode::SpiceXMLNode(const std::string &nodename)
    : m_value(""), m_unit(""), m_typechar('s'), m_typefullname("") {
  m_name = nodename;
}

SpiceXMLNode::~SpiceXMLNode() {}

void SpiceXMLNode::setValue(const std::string &strvalue) { m_value = strvalue; }

void SpiceXMLNode::setValues(const std::string &nodetype,
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

bool SpiceXMLNode::hasUnit() const { return (m_unit.size() > 0); }

bool SpiceXMLNode::hasValue() const { return (m_value.size() > 0); }

bool SpiceXMLNode::isString() const { return (m_typechar == STRING); }

bool SpiceXMLNode::isInteger() const { return (m_typechar == INT32); }

bool SpiceXMLNode::isDouble() const { return (m_typechar == FLOAT32); }

const std::string SpiceXMLNode::getName() const { return m_name; }
const std::string SpiceXMLNode::getUnit() const { return m_unit; }
const std::string SpiceXMLNode::getDescription() const { return m_description; }
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
/**
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
/**
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
  std::map<std::string, SpiceXMLNode> map_xmlnode;
  std::string detvaluestr("");
  parseSpiceXML(xmlfilename, map_xmlnode);

  size_t n = std::count(detvaluestr.begin(), detvaluestr.end(), '\n');
  g_log.notice() << "[DB] detector string value = " << n << "\n" << detvaluestr
                 << "\n";

  // Create output workspace
  MatrixWorkspace_sptr outws =
      createMatrixWorkspace(map_xmlnode, numpixelX, numpixelY, detlogname);

  setProperty("OutputWorkspace", outws);
}

void LoadSpiceXML2DDet::parseSpiceXML(
    const std::string &xmlfilename,
    std::map<std::string, SpiceXMLNode> &logstringmap) {
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
      g_log.notice() << "Parent node " << nodename << " has " << numchildren
                     << " children."
                     << "\n";
    } else if (numchildren == 1) {
      std::string innertext = pNode->innerText();
      size_t numattr = pNode->attributes()->length();
      g_log.notice() << "  Child node " << nodename << "'s attributes: "
                     << "\n";

      SpiceXMLNode xmlnode(nodename);
      std::string nodetype("");
      std::string nodeunit("");
      std::string nodedescription("");

      for (size_t j = 0; j < numattr; ++j) {
        std::string atttext = pNode->attributes()->item(j)->innerText();
        std::string attname = pNode->attributes()->item(j)->nodeName();
        g_log.notice() << "     attribute " << j << " name = " << attname
                       << ", "
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
      xmlnode.setValues(nodetype, nodeunit, nodedescription);
      xmlnode.setValue(innertext);

      logstringmap.insert(std::make_pair(nodename, xmlnode));
    } else {
      g_log.error("Funny... No child node.");
    }

    // Move to next node
    pNode = nodeIter.nextNode();
  } // ENDWHILE

  // Close file
  ifs.close();

  return;
}

MatrixWorkspace_sptr LoadSpiceXML2DDet::createMatrixWorkspace(
    const std::map<std::string, SpiceXMLNode> &mapxmlnode,
    const size_t &numpixelx, const size_t &numpixely,
    const std::string &detnodename) {

  // Create matrix workspace
  MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", numpixely, numpixelx,
                                          numpixelx));

  // Examine xml nodes
  std::map<std::string, SpiceXMLNode>::const_iterator miter;
  for (miter = mapxmlnode.begin(); miter != mapxmlnode.end(); ++miter) {
    SpiceXMLNode xmlnode = miter->second;
    g_log.notice() << "[DB] Log name / xml node : " << xmlnode.getName()
                   << "\n";
  }

  // Parse the detector counts node
  miter = mapxmlnode.find(detnodename);
  if (miter != mapxmlnode.end()) {
    // Get node value string (256x256 as a whole)
    SpiceXMLNode detnode = miter->second;
    const std::string detvaluestr = detnode.getValue();
    size_t numlines = static_cast<size_t>(
        std::count(detvaluestr.begin(), detvaluestr.end(), '\n'));
    g_log.notice() << "[DB] Detector counts string contains " << numlines
                   << "\n";

    // Split
    std::vector<std::string> vecLines;
    boost::split(vecLines, detvaluestr, boost::algorithm::is_any_of("\n"));
    g_log.notice() << "There are " << vecLines.size() << " lines"
                   << "\n";

    size_t irow = 0;
    for (size_t i = 0; i < vecLines.size(); ++i) {
      std::string &line = vecLines[i];

      // Skip empty line
      if (line.size() == 0) {
        g_log.notice() << "Empty Line at " << i << "\n";
        continue;
      }

      // Check whether it exceeds boundary
      if (irow == numpixely) {
        throw std::runtime_error("Number of non-empty rows in detector data "
                                 "exceeds user defined geometry size.");
      }

      // Split line
      std::vector<std::string> veccounts;
      boost::split(veccounts, line, boost::algorithm::is_any_of(" \t"));
      // g_log.notice() << "Number of items of line " << i << " is " <<
      // veccounts.size() << "\n";

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
  } else {
    std::stringstream errss;
    errss << "Unable to find an XML node of name " << detnodename
          << ". Unable to load 2D detector XML file.";
    throw std::runtime_error(errss.str());
  }

  return outws;
}

/** Parse a node containing 'type'.  Conver the value to the proper type
 * indicated by type
 * If type is not specified as either INT32 or FLOAT32, then it will be string
 * and no conversion
 * is applied.
 * @brief LoadSpiceXML2DDet::convertNode
 * @param nodetype
 * @param isdouble
 * @param dvalue
 * @param isint
 * @param ivalue
 */
void LoadSpiceXML2DDet::convertNode(const std::string &nodetype, bool &isdouble,
                                    double &dvalue, bool &isint, int &ivalue) {
  if (nodetype.compare("FLOAT32") == 0) {
    dvalue = atof(nodetype.c_str());
    isdouble = true;
    isint = false;
  } else if (nodetype.compare("INT32") == 0) {
    ivalue = atoi(nodetype.c_str());
    isdouble = false;
    isint = true;
  } else {
    isdouble = false;
    isint = false;
  }

  return;
}

} // namespace DataHandling
} // namespace Mantid
