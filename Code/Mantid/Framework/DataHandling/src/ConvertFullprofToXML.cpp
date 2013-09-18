/*WIKI* 

Convert the initial fitting parameters in a Fullprof file to XML format in an [[InstrumentParameterFile]].

*WIKI*/
#include "MantidDataHandling/ConvertFullprofToXML.h"
#include "MantidDataHandling/LoadFullprofResolution.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"

#include <fstream>

// Needed for writing the XML file (will be moved to a child algorithm)
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>


namespace Mantid
{
namespace DataHandling
{
  using namespace API;
  using namespace Poco::XML;

  DECLARE_ALGORITHM(ConvertFullprofToXML)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
 ConvertFullprofToXML::ConvertFullprofToXML()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertFullprofToXML::~ConvertFullprofToXML()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Sets documentation strings for this algorithm
    */
  void ConvertFullprofToXML::initDocs()
  {
    setWikiSummary("Convert the initial fitting parameters in a Fullprof file to XML format in an Instrument Parameter File");
    setOptionalMessage("Convert the initial fitting parameters in a Fullprof file to XML format in an Instrument Parameter File");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
   */
  void ConvertFullprofToXML::init()
  {
    // Input file name
    std::vector<std::string> exts;
    exts.push_back(".irf");
    exts.push_back(".prf");
    declareProperty(new FileProperty("InputFilename", "", FileProperty::Load, exts),
        "Path to an Fullprof file to load.");

    // Output file
    std::vector<std::string> extso;
    extso.push_back(".xml");
    declareProperty(new FileProperty("OutputFilename", "", FileProperty::Save, extso),
      "The name to give to the parameter file.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
    */
  void ConvertFullprofToXML::exec()
  {
    // Get input
    std::string datafile = getProperty("InputFilename");
    // Get Output
    std::string paramfile = getProperty("OutputFilename");

    // Load with LoadFullprofResolution
    auto loader = createChildAlgorithm("LoadFullprofResolution");
    loader->setProperty("Filename",datafile);
    loader->executeAsChildAlg();

    // Write into Parameter File
    // This code will later go into a child algorithm to enable it to be used by other algorithms
    // CODE INCOMPLETE

    // Set up access to table workspace ParamTable
    API::ITableWorkspace_sptr paramTable = loader->getProperty("OutputWorkspace");
    // get the table workspace row numbers of the parameters and store them for later use
    getTableRowNumbers( paramTable, m_rowNumbers);

    // Set up access to Output file
    std::ofstream outFile(paramfile.c_str());
    if (!outFile)
    {
      throw Mantid::Kernel::Exception::FileError("Unable to open file:", paramfile);
    }

    // Set up writer to Paremeter file
    DOMWriter writer;
    writer.setNewLine("\n");
    writer.setOptions(XMLWriter::PRETTY_PRINT);

    // Get current time
    Kernel::DateAndTime date = Kernel::DateAndTime::getCurrentTime();
    std::string ISOdate = date.toISO8601String();
    std::string ISOdateShort = ISOdate.substr(0,19); // Remove fraction of seconds

    // Create document
    Poco::XML::Document* mDoc = new Document();
    Element* rootElem = mDoc->createElement("parameter-file");
    rootElem->setAttribute("date", ISOdateShort);
    mDoc->appendChild(rootElem);

    // Add instrument
    Element* instrumentElem = mDoc->createElement("component-link");
    instrumentElem->setAttribute("name","wholeInstrument");
    rootElem->appendChild(instrumentElem);
    addALFBEparameter( paramTable, mDoc, instrumentElem, "Alph0");

    // Add banks
    if(paramTable->columnCount() < 2){
      throw std::runtime_error("No banks found");
    }
    size_t num_banks = paramTable->columnCount()-1;

    for( size_t i=0; i<num_banks; ++i)
    {
      std::ostringstream bankName;
      bankName << "Bank" << (i+1);
      Element* bankElem = mDoc->createElement("component-link");
      bankElem->setAttribute("name",bankName.str());
      rootElem->appendChild(bankElem);
    }


    // Write document structure into file
    writer.writeNode(outFile, mDoc);

    return;
  }

  /* Add an ALFBE parameter to the XML document according to the table workspace
  *
  *  paramName is the name of the parameter as it appears in the table workspace
  */
  void ConvertFullprofToXML::addALFBEparameter(const API::ITableWorkspace_sptr & tablews, Poco::XML::Document* mDoc, Element* parent, const std::string paramName)
  {
     Element* parameterElem = mDoc->createElement("parameter");
     parameterElem->setAttribute("name", getXMLParameterName(paramName));
     parameterElem->setAttribute("type","fitting");

     Element *formulaElem = mDoc->createElement("formula");
     formulaElem->setAttribute("eq",getXMLEqValue(tablews, paramName, 1));
     if(paramName != "Beta1") formulaElem->setAttribute("result-unit","TOF");
     parameterElem->appendChild(formulaElem);

     Element* fixedElem = mDoc->createElement("fixed");
     parameterElem->appendChild(fixedElem);

     parent->appendChild(parameterElem);
  }


  /*
  *  Get the XML name of a parameter given its Table Workspace name
  */
  std::string ConvertFullprofToXML::getXMLParameterName( const std::string name )
  {
    std::string prefix = "IkedaCarpenterPV:";
    if(name == "Alph0") return prefix+"Alpha0";
    if(name == "Beta0") return prefix+"Beta0";
    if(name == "Alph1") return prefix+"Alpha1";
    if(name == "Beta1") return prefix+"Kappa";
    return "?"+name;
  }

  /*
  * Get the value string to put in the XML eq attribute of the formula element of the paramenter element
  * given the name of the parameter in the table workspace.
  */
  std::string ConvertFullprofToXML::getXMLEqValue( const API::ITableWorkspace_sptr & tablews, const std::string name, size_t bankNumber)
  {
    //API::Column_const_sptr column = tablews->getColumn( bankNumber );
    return "?"+name+std::to_string(bankNumber);
  }

  /* This function fills in a list of the row numbers starting 0 of the parameters
     in the table workspace, so one can find the position in a column of
     the value of the given parameter.
  */
  void ConvertFullprofToXML::getTableRowNumbers(const API::ITableWorkspace_sptr & tablews, std::map<std::string, size_t>& parammap)
  {
    parammap.clear();

    size_t numrows = tablews->rowCount();
    for (size_t i = 0; i < numrows; ++i)
    {
      TableRow row = tablews->getRow(i);
      std::string name;
      row >> name;
      parammap.insert(std::make_pair(name, i));
    }

    return;
  }

} // namespace DataHandling
} // namespace Mantid




































