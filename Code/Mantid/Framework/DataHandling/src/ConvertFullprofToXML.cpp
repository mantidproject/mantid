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
    LoadFullprofResolution loader;
    loader.initialize();
    loader.setProperty("Filename",datafile);
    loader.setProperty("OutputWorkspace","ParamTable");

    loader.executeAsChildAlg();

    // Write into Parameter File
    // This code will later go into a child algorithm to enable it to be used by other algorithms
    // CODE YET TO BE WRITTEN

    // Remove table workspace after use
    AnalysisDataService::Instance().remove("ParamTable");

    return;
  }

} // namespace DataHandling
} // namespace Mantid




































