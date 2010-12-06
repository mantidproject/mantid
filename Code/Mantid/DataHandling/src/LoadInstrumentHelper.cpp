//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/UnitFactory.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include "MantidKernel/ArrayProperty.h"
#include <sstream>

#include "Poco/DirectoryIterator.h"
#include "Poco/RegularExpression.h"

#include "Poco/SAX/SAXParser.h"
#include "Poco/SAX/ContentHandler.h"
#include "Poco/SAX/Attributes.h"

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;

/// For debugging...
static bool VERBOSE = false;

namespace Mantid
{
namespace DataHandling
{
using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace Poco::XML;

// used to terminate SAX process
class DummyException {
public:
  std::string m_validFrom;
  std::string m_validTo;
  DummyException(std::string validFrom, std::string validTo) 
    : m_validFrom(validFrom), m_validTo(validTo) {}


};

// SAX content handler for grapping stuff quickly from IDF
class myContentHandler : public Poco::XML::ContentHandler
{
  virtual void startElement(
    const XMLString & uri,
    const XMLString & localName,
    const XMLString & qname,
    const Attributes & attrList
    ) 
  { 
    //int a = 2;
    //std::cout << localName << "\n";
    //for (int i = 0; i < attrList.getLength(); i++)
    //  std::cout << attrList.getValue(i) << "\n";

    if ( localName == "instrument" )
    {
      throw DummyException(static_cast<std::string>(attrList.getValue("","valid-from")), 
        static_cast<std::string>(attrList.getValue("","valid-to")));
    }
  }
  virtual void endElement(
    const XMLString & uri,
    const XMLString & localName,
    const XMLString & qname
    ) {}
  virtual void startDocument() {}
  virtual void endDocument() {}
  virtual void characters(
    const XMLChar ch[],
    int start,
    int length
    ) {}
  virtual void endPrefixMapping(
    const XMLString & prefix
    ) {}
  virtual void ignorableWhitespace(
    const XMLChar ch[],
    int start,
    int length
    ) {}
  virtual void processingInstruction(
    const XMLString & target,
    const XMLString & data
    ) {}
  virtual void setDocumentLocator(
    const Locator * loc
    ) {}
  virtual void skippedEntity(
    const XMLString & name
    ) {}
  virtual void startPrefixMapping(
    const XMLString & prefix,
    const XMLString & uri
    ) {}
};


/** Return from an IDF the values of the valid-from and valid-to attributes 
*
*  @param IDFname Full path of an IDF
*  @param outValidFrom Used to return valid-from date
*  @param outValidto Used to return valid-to date
*/
void LoadInstrumentHelper::getValidFromTo(const std::string& IDFfilename, std::string& outValidFrom,
  std::string& outValidTo)
{
      SAXParser pParser;
      myContentHandler*  conHand = new myContentHandler();
      pParser.setContentHandler(conHand);

      try
      {
        pParser.parse(IDFfilename);
      }
      catch(DummyException e)
      {
        outValidFrom = e.m_validFrom;
        outValidTo = e.m_validTo;
      }
      catch(...)
      {
        // should return some sensible here
      }
}


struct fromToEntry
{
  std::string identifier; 
  DateAndTime from;
  DateAndTime to;
};

/// Given an instrument name and a date return filename of appropriate IDF
/** A given instrument may have multiple IDFs associated with it. This method return an 
*  identifier which identify a given IDF for a given instrument. An IDF filename is 
*  required to be of the form IDFname + _Definition + Identifier + .xml, the identifier
*  then is the part of a filename that identifies the IDF valid at a given date.
*
*  @param IDFname Instrument name ID e.g. GEM, TOPAS or BIOSANS
*  @param date Date. E.g. date when raw files where collected
*/
std::string LoadInstrumentHelper::getIDF_identifier(const std::string& idfName, const std::string& date)
{
  // Get the search directory for XML instrument definition files (IDFs)
  std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();
  // adjust IDFname to be consistent with names used in instrument search directory
  std::string IDFname = Kernel::ConfigService::Instance().adjustIDFname(idfName);


  Poco::RegularExpression regex(IDFname+"_Definition.*\\.xml", Poco::RegularExpression::RE_CASELESS );
  Poco::DirectoryIterator end_iter;
  std::vector<fromToEntry> idfInstances;
  DateAndTime d(date);
  for ( Poco::DirectoryIterator dir_itr(directoryName); dir_itr != end_iter; ++dir_itr )
  {
    if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

    std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

    if ( regex.match(l_filenamePart) )
    {
      std::string validFrom, validTo;
      getValidFromTo(dir_itr->path(), validFrom, validTo);

      DateAndTime from(validFrom);
      DateAndTime to(validTo);
      if ( DateAndTime(validFrom) <= d && d <= DateAndTime(validTo) )
      {
        size_t foundDefinition;
        foundDefinition = l_filenamePart.find("_Definition");
        size_t foundXML;
        foundXML = l_filenamePart.find(".xml");
        return l_filenamePart.substr(foundDefinition+11,foundXML-foundDefinition-11);
      }
    }
  }

  // should throw warning here: no IDF found with date:  
  return "";
}

} // namespace DataHandling
} // namespace Mantid
