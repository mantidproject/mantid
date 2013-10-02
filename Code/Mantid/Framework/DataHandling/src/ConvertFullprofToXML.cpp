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

#include <boost/lexical_cast.hpp>


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

    // Instrument name
    declareProperty("InstrumentName", "", "Name of instrument for the input file" );

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

    // Get instrument
    std::string instrumentName = getProperty("InstrumentName");

    // Get Output
    std::string paramfile = getProperty("OutputFilename");


    // We need the instrument name because it is not extracted by LoadFullprofResolution and is
    // needed by fitting despite also being available in the IDF.
    if ( instrumentName.empty() )
      throw std::runtime_error("The InstrumentName property must be set.");

    // Load with LoadFullprofResolution
    auto loader = createChildAlgorithm("LoadFullprofResolution");
    loader->setProperty("Filename",datafile);
    loader->executeAsChildAlg();

    // Write into Parameter File
    // This code will later go into a child algorithm to enable it to be used by other algorithms

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
    instrumentElem->setAttribute("name",instrumentName);
    rootElem->appendChild(instrumentElem);
    addALFBEParameter( paramTable, mDoc, instrumentElem, "Alph0");
    addALFBEParameter( paramTable, mDoc, instrumentElem, "Beta0");
    addALFBEParameter( paramTable, mDoc, instrumentElem, "Alph1");
    addALFBEParameter( paramTable, mDoc, instrumentElem, "Beta1");

    // Add banks
    if(paramTable->columnCount() < 2){
      throw std::runtime_error("No banks found");
    }
    size_t num_banks = paramTable->columnCount()-1;

    for( size_t i=0; i<num_banks; ++i)
    {
      API::Column_const_sptr column = paramTable->getColumn( i+1 );
      const double bankNumber = column->cell<double>(0);
      std::ostringstream bankName;
      bankName << "bank" << bankNumber;
      Element* bankElem = mDoc->createElement("component-link");
      bankElem->setAttribute("name",bankName.str());
      addSigmaParameters( paramTable, mDoc, bankElem, i+1);
      addGammaParameters( paramTable, mDoc, bankElem, i+1);
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
  void ConvertFullprofToXML::addALFBEParameter(const API::ITableWorkspace_sptr & tablews, Poco::XML::Document* mDoc, Element* parent, const std::string& paramName)
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

  /* Add a set of SIGMA paraters to the XML document according to the table workspace
   * for the bank at the given column of the table workspace
   */
  void ConvertFullprofToXML::addSigmaParameters(const API::ITableWorkspace_sptr & tablews, Poco::XML::Document* mDoc, Poco::XML::Element* parent, size_t columnIndex)
  {
     Element* parameterElem = mDoc->createElement("parameter");
     parameterElem->setAttribute("name", "IkedaCarpenterPV:SigmaSquared");
     parameterElem->setAttribute("type","fitting");

     Element *formulaElem = mDoc->createElement("formula");
     std::string eqValue = getXMLEqValue(tablews, "Sig1", columnIndex)+"*centre^2+"+getXMLEqValue(tablews, "Sig0", columnIndex);
     formulaElem->setAttribute("eq", eqValue);
     formulaElem->setAttribute("unit","dSpacing");
     formulaElem->setAttribute("result-unit","TOF^2");
     parameterElem->appendChild(formulaElem);

     parent->appendChild(parameterElem);
  }

   /* Add a set of GAMMA paraters to the XML document according to the table workspace
   * for the bank at the given column of the table workspace
   */
  void ConvertFullprofToXML::addGammaParameters(const API::ITableWorkspace_sptr & tablews, Poco::XML::Document* mDoc, Poco::XML::Element* parent, size_t columnIndex)
  {
     Element* parameterElem = mDoc->createElement("parameter");
     parameterElem->setAttribute("name", "IkedaCarpenterPV:Gamma");
     parameterElem->setAttribute("type","fitting");

     Element *formulaElem = mDoc->createElement("formula");
     std::string eqValue = getXMLEqValue(tablews, "Gam1", columnIndex)+"*centre";
     formulaElem->setAttribute("eq", eqValue);
     formulaElem->setAttribute("unit","dSpacing");
     formulaElem->setAttribute("result-unit","TOF");
     parameterElem->appendChild(formulaElem);

     parent->appendChild(parameterElem);
  }



  /*
  *  Get the XML name of a parameter given its Table Workspace name
  */
  std::string ConvertFullprofToXML::getXMLParameterName( const std::string& name )
  {
    // Only used for ALFBE parameters
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
  std::string ConvertFullprofToXML::getXMLEqValue( const API::ITableWorkspace_sptr & tablews, const std::string& name, size_t columnIndex)
  {
    size_t paramNumber = m_rowNumbers[name];
    API::Column_const_sptr column = tablews->getColumn( columnIndex );
    double eqValue = column->cell<double>(paramNumber);
    if(name.substr(0,3) == "Sig") eqValue = eqValue*eqValue; // Square the sigma values
    return boost::lexical_cast<std::string>(eqValue);
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




































