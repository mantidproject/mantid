#include "MantidDataHandling/LoadSpiceXML2DDet.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "Poco/SAX/InputSource.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/AutoPtr.h"

#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"

#include <fstream>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::API;
using namespace Mantid::Kernel;

DECLARE_ALGORITHM(LoadSpiceXML2DDet)

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
}

//----------------------------------------------------------------------------------------------
/**
 * @brief LoadSpiceXML2DDet::exec
 */
void LoadSpiceXML2DDet::exec() {
  // Load input
  const std::string xmlfilename = getProperty("Filename");

  // Parse
  parseSpiceXML(xmlfilename);

  // Create output workspace
  MatrixWorkspace_sptr outws = createMatrixWorkspace();

  setProperty("OutputWorkspace", outws);
}

void LoadSpiceXML2DDet::parseSpiceXML(const std::string &xmlfilename) {
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
    const Poco::XML::XMLString nodevalue = pNode->nodeValue();
    g_log.notice() << "[DB] Node name = " << nodename << "\n";
    pNode = nodeIter.nextNode();
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

} // namespace DataHandling
} // namespace Mantid
