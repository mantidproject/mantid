/*WIKI*



This algorithm saves a SpecialWorkspace2D/MaskWorkspace to an XML file.


*WIKI*/

#include "MantidDataHandling/SaveDetectorMasks.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ISpectrum.h"

#include "fstream"
#include "sstream"
#include "algorithm"

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

    // 1. Get input
    DataObjects::SpecialWorkspace2D_const_sptr inpWS = this->getProperty("InputWorkspace");
    std::string outxmlfilename = this->getPropertyValue("OutputFile");

    // 2. Convert Workspace to ...
    std::vector<detid_t> detid0s;
    for (size_t i = 0; i < inpWS->getNumberHistograms(); i ++){
      if (inpWS->dataY(i)[0] > 0.1){
        // It is way from 0 but smaller than 1
        // a) workspace index -> spectrum -> detector ID
        const API::ISpectrum *spec = inpWS->getSpectrum(i);
        if (!spec){
          g_log.error() << "No spectrum corresponds to workspace index " << i << std::endl;
          throw std::invalid_argument("Cannot find spectrum");
        }

        const std::set<detid_t> detids = spec->getDetectorIDs();
        if (detids.size() != 1){
          g_log.error() << "Impossible Situation! Workspace " << i << " corresponds to #(Det) = " << detids.size() << std::endl;
          throw std::invalid_argument("Impossible number of detectors");
        }

        // b) get detector id
        detid_t detid = 0;
        std::set<detid_t>::const_iterator it;
        for (it=detids.begin(); it!=detids.end(); ++it){
          detid = *it;
        }

        // c) store
        detid0s.push_back(detid);

      } // if
    } // for

    // d) sort
    g_log.debug() << "Number of detetors to be masked = " << detid0s.size() << std::endl;
    std::sort(detid0s.begin(), detid0s.end());

    // 3. Count workspace to count 1 and 0
    std::vector<detid_t> idx0sts;  // starting point of the pair
    std::vector<detid_t> idx0eds;  // ending point of pair

    detid_t i0st = detid0s[0];
    detid_t i0ed = detid0s[0];

    for (size_t i = 1; i < detid0s.size(); i ++){

      if (detid0s[i] == detid0s[i-1]+1){
        // If it is continuous: record the current one
        i0ed = detid0s[i];
      } else {
        // If skip: restart everything
        // i) record previous result
        idx0sts.push_back(i0st);
        idx0eds.push_back(i0ed);
        // ii) reset the register
        i0st = detid0s[i];
        i0ed = detid0s[i];
      }

    } // for

    // Complete the registration
    idx0sts.push_back(i0st);
    idx0eds.push_back(i0ed);

    for (size_t i = 0; i < idx0sts.size(); i++){
      g_log.information() << "Section " << i << " : " << idx0sts[i] << "  ,  " << idx0eds[i] << " to be masked and recorded."<< std::endl;
    }

    // 4. Write out to XML nodes
    // a) Create document and root node
    AutoPtr<Document> pDoc = new Document;
    AutoPtr<Element> pRoot = pDoc->createElement("detector-masking");
    pDoc->appendChild(pRoot);
    // pRoot->setAttribute("default", "use");

    // b) Append Group
    AutoPtr<Element> pChildGroup = pDoc->createElement("group");
    // pChildGroup->setAttribute("type", "notuse");
    pRoot->appendChild(pChildGroup);

    // c) Append detid
    // c1. Generate text value
    std::stringstream ss;
    for (size_t i = 0; i < idx0sts.size(); i ++){
      size_t ist = idx0sts[i];
      size_t ied = idx0eds[i];

      // a-b or a
      bool writedata = true;
      if (ist < ied){
        ss << ist << "-" << ied;
      } else if (ist == ied){
        ss << ist;
      } else {
        writedata = false;
      }
      // add ","
      if (writedata && i < idx0sts.size()-1){
        ss << ",";
      }

    } // for
    std::string textvalue = ss.str();

    // c2. Create element
    AutoPtr<Element> pDetid = pDoc->createElement("detids");
    AutoPtr<Text> pText1 = pDoc->createTextNode(textvalue);
    pDetid->appendChild(pText1);
    pChildGroup->appendChild(pDetid);

    // 4. Write
    DOMWriter writer;
    writer.setNewLine("\n");
    writer.setOptions(XMLWriter::PRETTY_PRINT);

    std::ofstream ofs;
    ofs.open(outxmlfilename.c_str(), std::fstream::out);

    ofs << "<?xml version=\"1.0\"?>\n"
        << "<?xml-stylesheet type=\"text/xsl\" href=\"cansasxml-html.xsl\" ?>\n";

    writer.writeNode(std::cout, pDoc);
    writer.writeNode(ofs, pDoc);
    ofs.close();

    return;
  }


} // namespace DataHandling
} // namespace Mantid
