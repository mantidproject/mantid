/*WIKI* 

Load parameters from a GSAS instrument file into a table workspace

Later developments of this algorithm will enable these parameters to be put into the instrument of a wotrkspace
for either Ikeda-Carpender pseudo-Voigt translated into [[IkedaCarpenterPV]] or
back-to-back-exponential pseudo-Voigt translated into [[BackToBackExponential]].

*WIKI*/

#include "MantidDataHandling/LoadGSASInstrumentFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidDataHandling/LoadParameterFile.h"

#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/AutoPtr.h>

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
using namespace Poco::XML;

using Geometry::Instrument;
using Geometry::Instrument_sptr;
using Geometry::Instrument_const_sptr;
using Mantid::Geometry::InstrumentDefinitionParser;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(LoadGSASInstrumentFile)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadGSASInstrumentFile::LoadGSASInstrumentFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadGSASInstrumentFile::~LoadGSASInstrumentFile()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Sets documentation strings for this algorithm
    */
  void LoadGSASInstrumentFile::initDocs()
  {
    setWikiSummary("Load parameters from a GSAS Instrument file.");
    setOptionalMessage("Load parameters from a GSAS Instrument file.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
   */
  void LoadGSASInstrumentFile::init()
  {
    // Input file name
    vector<std::string> exts;
    exts.push_back(".prm");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an GSAS file to load.");

    // Output table workspace
    auto wsprop = new WorkspaceProperty<API::ITableWorkspace>("OutputTableWorkspace", "", Direction::Output, PropertyMode::Optional);
    declareProperty(wsprop, "Name of the output TableWorkspace containing instrument parameter information read from file. ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
    */
  void LoadGSASInstrumentFile::exec()
  {
    // Get input
    string datafile = getProperty("Filename");

    // Import data
    vector<string> lines;
    loadFile(datafile, lines);

    // Check Histogram type - only PNTR is currently supported
    std::string histType = getHistogramType( lines );
    if (histType != "PNTR") {
      if( histType.size()  == 4)
      {
        throw std::runtime_error("Histogram type "+histType+" not supported \n");
      }
      else
      {
        throw std::runtime_error("Error on checking histogram type: "+histType+"\n");
      }
    }

    size_t numBanks = getNumberOfBanks( lines );
    g_log.debug() << numBanks << "banks in file \n";

    // Examine bank information
    vector<size_t> bankStartIndex;
    scanBanks(lines, bankStartIndex );

    if( bankStartIndex.size() == 0)
    {
      throw std::runtime_error("No nanks found in file. \n");
    }

    if( numBanks != bankStartIndex.size()) 
    {  // The stated number of banks does not equal the number of banks found
      g_log.warning() << "The number of banks found" << bankStartIndex.size() << "is not equal to the number of banks stated" << numBanks << ".\n";
      g_log.warning() << "Number of banks found is used.";
      numBanks = bankStartIndex.size();
    }

   // Parse banks and export profile parameters
    //map<int, map<string, double> > bankparammap;
    for (size_t i = 0; i < numBanks; ++i)
    {
      size_t bankid = i+1;
      g_log.debug() << "Parse bank " << bankid << " of total " << numBanks << ".\n";
      //map<string, double> parammap;
      //parseResolutionStrings(parammap, lines, useBankIDsInFile , bankid, bankstartindexmap[bankid], bankendindexmap[bankid], nProf);
      //bankparammap.insert(make_pair(bankid, parammap));
      g_log.warning() << "Bank starts at line" << bankStartIndex[i] + 1 << "\n";
    }


    if( getPropertyValue("OutputTableWorkspace") != "")
    {
      // Output the output table workspace
      //setProperty("OutputTableWorkspace", outTabWs);
    }

    if( getPropertyValue("OutputTableWorkspace") == "")
    {
      // We don't know where to output
      throw std::runtime_error("Either the OutputTableWorkspace must be set.");
    }


    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Load file to a vector of strings.  Each string is a non-empty line.
    * @param filename :: string for name of the .prm file
    * @param lines :: vector of strings for each non-empty line in .prm file
    */
  void LoadGSASInstrumentFile::loadFile(string filename, vector<string>& lines)
  {
    string line;

    //the variable of type ifstream:
    ifstream myfile (filename.c_str());

    //check to see if the file is opened:
    if (myfile.is_open())
    {
      //while there are still lines in the
      //file, keep reading:
      while (! myfile.eof() )
      {
        //place the line from myfile into the
        //line variable:
        getline (myfile,line);

        //display the line we gathered:
        boost::algorithm::trim(line);
        if (line.size() > 0)
          lines.push_back(line);
      }

      //close the stream:
      myfile.close();
    }
    else
    {
      stringstream errmsg;
      errmsg << "Input .prm file " << filename << " cannot be open. ";
      g_log.error(errmsg.str());
      throw runtime_error(errmsg.str());
    }

    return;
  }

  /* Get the histogram type
  * @param lines :: vector of strings for each non-empty line in .irf file
  */
  std::string LoadGSASInstrumentFile::getHistogramType(vector<string>& lines)
  {
    // We assume there is just one HTYPE line, look for it from beginning and return its value.
    std::string lookFor = "INS   HTYPE";
    for (size_t i = 0; i <= lines.size(); ++i)
    {
      if(lines[i].substr(0,lookFor.size()) == lookFor)
      {
        if(lines[i].size() < lookFor.size()+7){
          // line too short
          return "HTYPE line too short";
        }
        return  lines[i].substr(lookFor.size()+3,4); // Found
      }
    }
    return "HTYPE line not found";
  }

 /* Get number of banks
  * @param lines :: vector of strings for each non-empty line in .irf file
  */
  size_t LoadGSASInstrumentFile::getNumberOfBanks(vector<string>& lines)
  {
    // We assume there is just one BANK line, look for it from beginning and return its value.
    std::string lookFor = "INS   BANK";
    for (size_t i = 0; i <= lines.size(); ++i)
    {
      if(lines[i].substr(0,lookFor.size()) == lookFor)
      {
        if(lines[i].size() < lookFor.size()+3){
          // line too short
          return 0;
        }
        return  boost::lexical_cast<size_t>(lines[i].substr(lookFor.size()+2,1)); // Found
      }
    }
    return 0;
  }

  /** Scan lines to determine which line each bank begins
  * @param lines :: vector of string of all non-empty lines in input file;
  * @param bankStartIndex :: [output] vector of start indices of banks in the order they occur in file
  */
  void LoadGSASInstrumentFile::scanBanks(const std::vector<std::string>& lines, std::vector<size_t>& bankStartIndex )
  {
    // We look for each line that contains 'BNKPAR' and take it to be the first line of a bank.
    // We currently ignore the bank number and assume they are numbered according to their order in the file.
    int startindex = -1;
    for (size_t i = 0; i < lines.size(); ++i)
    {
      string line = lines[i];
      if(line.substr(0,3) == "INS"){  // Ignore all lines that don't begin with INS
        if (line.find("BNKPAR") != string::npos)
        {  // We've found start of a new bank
          bankStartIndex.push_back(i);
        }
      }  // INS
    } // for(i)
  }


} // namespace DataHandling
} // namespace Mantid




































