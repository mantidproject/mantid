#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataHandling/SaveParameterFile.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include <boost/lexical_cast.hpp>

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/XML/XMLWriter.h>

#include <fstream>

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveParameterFile)

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;

  using namespace Poco;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveParameterFile::SaveParameterFile()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveParameterFile::~SaveParameterFile()
  { }

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SaveParameterFile::name() const { return "SaveParameterFile";};

  /// Algorithm's version for identification. @see Algorithm::version
  int SaveParameterFile::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SaveParameterFile::category() const { return "DataHandling\\Instrument";}

  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveParameterFile::init()
  {
    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::Input,boost::make_shared<InstrumentValidator>()),
    "Workspace to save the instrument parameters from.");

    std::vector<std::string> exts;
    exts.push_back(".xml");

    declareProperty(new API::FileProperty("Filename","", API::FileProperty::Save, exts),
    "The name of the file into which the instrument parameters will be saved.");

    declareProperty("LocationParameters", false, "Save the location parameters used to calibrate the instrument.", Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveParameterFile::exec()
  {
    const MatrixWorkspace_const_sptr ws = getProperty("Workspace");
    const bool saveLocationParams = getProperty("LocationParameters");
    const std::string filename = getProperty("Filename");

    const Instrument_const_sptr instrument = ws->getInstrument();
    const ParameterMap_sptr params = instrument->getParameterMap();

    //Map of component ids to their respective XML Elements
    std::map<ComponentID,XML::AutoPtr<XML::Element> > compMap;

    //Set up the XML document
    XML::AutoPtr<XML::Document> xmlDoc = new XML::Document;
    XML::AutoPtr<XML::Element> rootElem = xmlDoc->createElement("parameter-file");
    rootElem->setAttribute("instrument", instrument->getName());
    rootElem->setAttribute("valid-from", instrument->getValidFromDate().toISO8601String());
    xmlDoc->appendChild(rootElem);

    //Iterate through all the parameters set for the instrument and build an XML
    //document out of it.
    for(auto paramsIt = params->begin(); paramsIt != params->end(); ++paramsIt)
    {
      //Component data
      const ComponentID cID = (*paramsIt).first;
      const std::string cFullName = cID->getFullName();
      const IDetector* cDet = dynamic_cast<IDetector*>(cID);
      const detid_t cDetID = (cDet) ? cDet->getID() : 0;
      const std::string cDetIDStr = boost::lexical_cast<std::string>(cDetID);

      //Parameter data
      const std::string pName = (*paramsIt).second->name();
      const std::string pType = (*paramsIt).second->type();
      const std::string pValue = (*paramsIt).second->asString();

      //Skip rot and pos according to: http://www.mantidproject.org/IDF#Using_.3Cparameter.3E
      if(pName == "pos" || pName == "rot")
        continue;

      //If save location parameters is not enabled, skip any location parameters
      if(!saveLocationParams)
      {
        if( pName == "x"          || pName == "y"          || pName == "z" ||
            pName == "r-position" || pName == "t-position" || pName == "p-position" ||
            pName == "rotx"       || pName == "roty"       || pName == "rotz")
        {
          continue;
        }
      }

      //A component-link element
      XML::AutoPtr<XML::Element> compElem = 0;

      /* If an element already exists for a component with this name, re-use it.
       *
       * Why are we using an std::map and not simply traversing the DOM? Because
       * the interface for doing that is painful and horrible to use, and this is
       * probably faster (but negligably so in this case).
       *
       * And lastly, because Poco::XML::NodeList::length() segfaults.
       */
      auto compIt = compMap.find(cID);
      if(compIt != compMap.end())
      {
        compElem = (*compIt).second;
      }

      //One doesn't already exist? Make a new one.
      if(!compElem)
      {
        compElem = xmlDoc->createElement("component-link");
        rootElem->appendChild(compElem);
        compMap[cID] = compElem;
      }

      //Create the parameter and value elements
      XML::AutoPtr<XML::Element> paramElem = xmlDoc->createElement("parameter");
      XML::AutoPtr<XML::Element> valueElem = xmlDoc->createElement("value");

      //Set the attributes
      compElem->setAttribute("name", cFullName);

      //If there is a valid detector id, include it
      if(cDetID > 0)
      {
        compElem->setAttribute("id", cDetIDStr);
      }

      paramElem->setAttribute("name", pName);

      //For strings, we specify their type.
      if(pType == "string")
      {
        paramElem->setAttribute("type", "string");
      }

      valueElem->setAttribute("val", pValue);

      //Insert the elements into the document
      compElem->appendChild(paramElem);
      paramElem->appendChild(valueElem);
    }

    //Output the XMl document to the given file.
    XML::DOMWriter writer;
    writer.setOptions(XML::XMLWriter::PRETTY_PRINT | XML::XMLWriter::WRITE_XML_DECLARATION);
    std::ofstream file(filename.c_str(), std::ofstream::trunc);
    try
    {
      writer.writeNode(file, xmlDoc);
    }
    catch(Poco::Exception &e)
    {
      g_log.error() << "Error serializing XML for SaveParameterFile: " << e.displayText() << std::endl;
    }
    file.flush();
    file.close();

  }

} // namespace Algorithms
} // namespace Mantid
