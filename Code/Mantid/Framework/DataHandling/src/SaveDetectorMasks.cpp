/*WIKI*



This algorithm saves a SpecialWorkspace2D/MaskWorkspace to an XML file.


*WIKI*/

#include "MantidDataHandling/SaveDetectorMasks.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidAPI/FileProperty.h"


#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/Text.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMWriter.h"
#include "Poco/XML/XMLWriter.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace Poco::XML;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(SaveDetectorMasks)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveDetectorMasks::SaveDetectorMasks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveDetectorMasks::~SaveDetectorMasks()
  {
  }
  
  /// Sets documentation strings for this algorithm
  void SaveDetectorMasks::initDocs(){
    this->setWikiSummary("Save a MaskWorkspace/SpecialWorkspace2D to an XML file.");
    this->setOptionalMessage("Save a MaskWorkspace/SpecialWorkspace2D to an XML file.");
  }

  /// Define input parameters
  void SaveDetectorMasks::init(){

    declareProperty(new API::WorkspaceProperty<DataObjects::SpecialWorkspace2D>("InputWorkspace", "", Direction::Input),
        "MaskingWorkspace to output to XML file (SpecialWorkspace2D)");
    declareProperty(new FileProperty("OutputFile", "", FileProperty::Save, ".xml"),
        "File to save the detectors mask in XML format");

  }

  /// Main body to execute algorithm
  void SaveDetectorMasks::exec(){
    // TODO This is a dummy prototype

    // 1. Create document and root node
    AutoPtr<Document> pDoc = new Document;
    AutoPtr<Element> pRoot = pDoc->createElement("root");
    pDoc->appendChild(pRoot);

    // 2. Append child
    AutoPtr<Element> pChild1 = pDoc->createElement("child1");
    AutoPtr<Text> pText1 = pDoc->createTextNode("text1");
    pChild1->appendChild(pText1);
    pRoot->appendChild(pChild1);

    // 3. Write
    DOMWriter writer;
    writer.setNewLine("\n");
    writer.setOptions(XMLWriter::PRETTY_PRINT);
    writer.writeNode(std::cout, pDoc);

  }


} // namespace DataHandling
} // namespace Mantid
