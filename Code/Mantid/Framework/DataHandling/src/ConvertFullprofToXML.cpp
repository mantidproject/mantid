/*WIKI* 

Convert the initial fitting parameters in a Fullprof file to XML format in an [[InstrumentParameterFile]].

*WIKI*/
#include "MantidDataHandling/ConvertFullprofToXML.h"
#include "MantidDataHandling/LoadFullprofResolution.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <fstream>

// Needed for writing the XML file (will be moved to a child algorithm)
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>

using namespace Poco::XML;

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace std;

namespace Mantid
{
namespace DataHandling
{

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
    vector<std::string> exts;
    exts.push_back(".irf");
    exts.push_back(".prf");
    declareProperty(new FileProperty("InputFilename", "", FileProperty::Load, exts),
        "Path to an Fullprof file to load.");

    // Output file
    vector<std::string> extso;
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
    string datafile = getProperty("InputFilename");
    // Get Output
    string paramfile = getProperty("OutputFilename");

    //vector<int> outputbankids = getProperty("Banks");

    // Load with LoadFullprofResolution
    //LoadFullprofResolution loader;
    //loader.initialize();
    auto loader = createChildAlgorithm("LoadFullprofResolution");
    loader->setProperty("Filename",datafile);
    loader->executeAsChildAlg();

    // Write into Parameter File
    // This code will later go into a child algorithm to enable it to be used by other algorithms
    // CODE INCOMPLETE

    // Set up access to table workspace ParamTable
    API::ITableWorkspace_sptr paramTable = loader->getProperty("OutputWorkspace");

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

    if(paramTable->columnCount() < 2){
      throw std::runtime_error("No banks found");
    }
    size_t num_banks = paramTable->columnCount()-1;

    for( size_t i=0; i<num_banks; ++i)
    {
      std::ostringstream bankName;
      bankName << "Bank" << (i+1);
      Element* bankElem = mDoc->createElement(bankName.str());
      rootElem->appendChild(bankElem);
    }


    // Write document structure into file
    writer.writeNode(outFile, mDoc);

    return;
  }

} // namespace DataHandling
} // namespace Mantid




































