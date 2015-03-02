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

const bool SpiceXMLNode::hasUnit() const { return (m_unit.size() > 0); }

const bool SpiceXMLNode::hasValue() const { return (m_value.size() > 0); }

const bool SpiceXMLNode::isString() const { return (m_typechar == STRING); }

const bool SpiceXMLNode::isInteger() const { return (m_typechar == INT32); }

const bool SpiceXMLNode::isDouble() const { return (m_typechar == FLOAT32); }

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

  declareProperty("DetectorLogName", "detector",
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
  std::map<std::string, std::string> map_logstr;
  std::string detvaluestr("");
  parseSpiceXML(xmlfilename, detlogname, detvaluestr, map_logstr);

  size_t n = std::count(detvaluestr.begin(), detvaluestr.end(), '\n');
  g_log.notice() << "[DB] detector string value = " << n << "\n" << detvaluestr
                 << "\n";

  // Create output workspace
  MatrixWorkspace_sptr outws = createMatrixWorkspace();

  setProperty("OutputWorkspace", outws);
}

void LoadSpiceXML2DDet::parseSpiceXML(
    const std::string &xmlfilename, const std::string &detlogname,
    std::string &detstring, std::map<std::string, std::string> &logstringmap) {
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
      size_t numattr = pNode->attributes()->length();
      g_log.notice() << "  Child node " << nodename << "'s attributes: "
                     << "\n";
      for (size_t j = 0; j < numattr; ++j) {
        std::string atttext = pNode->attributes()->item(j)->innerText();
        std::string attname = pNode->attributes()->item(j)->nodeName();
        g_log.notice() << "     attribute " << j << " name = " << attname
                       << ", "
                       << "value = " << atttext << "\n";
      }
    } else {
      g_log.error("Funny... No child node.");
    }

    std::string innertext = pNode->innerText();

    if (pNode->childNodes()->length() == 1) {
      if (pNode->childNodes()->item(0)) {
        std::string childnodevalue =
            pNode->childNodes()->item(0)->getNodeValue();
        // g_log.notice() << "[DB] " << "Single child node value = " <<
        // childnodevalue << "\n";
      }
    }

    pNode = nodeIter.nextNode();

    if (nodename.compare(detlogname) == 0) {
      // it is a detector value string
      detstring = innertext;
    } else {
      // it is a log value string
      logstringmap.insert(std::make_pair(nodename, innertext));
    }
  }

  // Close file
  ifs.close();

  return;
}

MatrixWorkspace_sptr LoadSpiceXML2DDet::createMatrixWorkspace() {

  MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1));

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
