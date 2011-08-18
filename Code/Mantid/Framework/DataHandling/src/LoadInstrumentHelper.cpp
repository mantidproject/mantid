//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/RegularExpression.h>
#include <Poco/DirectoryIterator.h>

#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/SAXParser.h>
#include <Poco/SAX/Attributes.h>


namespace Mantid
{
namespace DataHandling
{
  using Kernel::DateAndTime;
  using Kernel::ConfigService;
  using Kernel::Logger;
  using Geometry::Instrument;
  using Geometry::OneToOneSpectraDetectorMap;
  using API::MatrixWorkspace;
  using API::Run;
  using API::SpectraDetectorMap;
  using Poco::XML::XMLString;
  using Poco::XML::Attributes;
  using Poco::XML::SAXParser;
  using Poco::XML::XMLChar;
  using Poco::XML::Locator;

Logger& LoadInstrumentHelper::g_log = Logger::get("LoadInstrumentHelper");

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
    const XMLString &,
    const XMLString & localName,
    const XMLString &,
    const Attributes & attrList
    ) 
  { 
    if ( localName == "instrument" )
    {
      throw DummyException(static_cast<std::string>(attrList.getValue("","valid-from")), 
        static_cast<std::string>(attrList.getValue("","valid-to")));
    }
  }
  virtual void endElement(
    const XMLString &,
    const XMLString &,
    const XMLString & 
    ) {}
  virtual void startDocument() {}
  virtual void endDocument() {}
  virtual void characters(
    const XMLChar [],
    int ,
    int 
    ) {}
  virtual void endPrefixMapping(
    const XMLString &
    ) {}
  virtual void ignorableWhitespace(
    const XMLChar [],
    int ,
    int 
    ) {}
  virtual void processingInstruction(
    const XMLString & ,
    const XMLString & 
    ) {}
  virtual void setDocumentLocator(
    const Locator *
    ) {}
  virtual void skippedEntity(
    const XMLString &
    ) {}
  virtual void startPrefixMapping(
    const XMLString & ,
    const XMLString & 
    ) {}
};


/** Return from an IDF the values of the valid-from and valid-to attributes 
*
*  @param IDFfilename :: Full path of an IDF
*  @param[out] outValidFrom :: Used to return valid-from date
*  @param[out] outValidTo :: Used to return valid-to date
*/
void LoadInstrumentHelper::getValidFromTo(const std::string& IDFfilename, std::string& outValidFrom,
  std::string& outValidTo)
{
      SAXParser pParser;
      // Create on stack to ensure deletion. Relies on pParser also being local variable.
      myContentHandler  conHand;
      pParser.setContentHandler(&conHand);

      try
      {
        pParser.parse(IDFfilename);
      }
      catch(DummyException &e)
      {
        outValidFrom = e.m_validFrom;
        outValidTo = e.m_validTo;
      }
      catch(...)
      {
        // should throw some sensible here
      }
}


/** Return workspace start date as an ISO 8601 string. If this info not stored in workspace the 
*   method returns current date.
*
*  @param workspace :: workspace to get information from
*  @return workspace start date as a string
*/
std::string LoadInstrumentHelper::getWorkspaceStartDate(const boost::shared_ptr<API::MatrixWorkspace>& workspace)
{
    const API::Run& runDetails = workspace->run();
    std::string date;
    if ( runDetails.hasProperty("run_start") )
    {
      date = runDetails.getProperty("run_start")->value();
    }
    else
    {
      g_log.information("run_start not stored in workspace. Default to current date.");

      date = Kernel::DateAndTime::get_current_time().to_ISO8601_string();
    }

    return date;
}

/** A given instrument may have multiple IDFs associated with it. This method return an 
*  identifier which identify a given IDF for a given instrument. An IDF filename is 
*  required to be of the form IDFname + _Definition + Identifier + .xml, the identifier
*  then is the part of a filename that identifies the IDF valid at a given date.
*
*  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
*  @param date :: ISO 8601 date
*  @return full path of IDF
*/
std::string LoadInstrumentHelper::getInstrumentFilename(const std::string& instrumentName, const std::string& date)
{
  g_log.debug() << "Looking for instrment XML file for " << instrumentName << " that is valid on '" << date << "'\n";
  // Lookup the instrument (long) name
  std::string instrument(Kernel::ConfigService::Instance().getInstrument(instrumentName).name());

  // Get the search directory for XML instrument definition files (IDFs)
  std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();

  Poco::RegularExpression regex(instrument+"_Definition.*\\.xml", Poco::RegularExpression::RE_CASELESS );
  Poco::DirectoryIterator end_iter;
  DateAndTime d(date);
  std::string mostRecentIDF; // store most recent IDF which is returned if no match for the date found
  DateAndTime refDate("1900-01-31 23:59:59"); // used to help determine the most recent IDF
  for ( Poco::DirectoryIterator dir_itr(directoryName); dir_itr != end_iter; ++dir_itr )
  {
    if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

    std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();
    if ( regex.match(l_filenamePart) )
    {
      g_log.debug() << "Found file: '" << dir_itr->path() << "'\n";
      std::string validFrom, validTo;
      LoadInstrumentHelper::getValidFromTo(dir_itr->path(), validFrom, validTo);
      g_log.debug() << "File '" << dir_itr->path() << " valid dates: from '" << validFrom << "' to '" << validTo << "'\n";
      DateAndTime from(validFrom);
      // Use a default valid-to date if none was found.
      DateAndTime to;
      if (validTo.length() > 0)
        to.set_from_ISO8601_string(validTo);
      else
        to.set_from_ISO8601_string("2100-01-01");

      if ( from <= d && d <= to )
      {
        return dir_itr->path();
      }
      if ( to > refDate )
      {
        refDate = to;
        mostRecentIDF = dir_itr->path();
      }
    }
  }

  // No date match found return most recent   
  return mostRecentIDF;
}

} // namespace DataHandling
} // namespace Mantid
