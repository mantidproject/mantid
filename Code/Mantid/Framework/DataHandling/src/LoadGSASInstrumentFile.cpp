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

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Load file to a vector of strings.  Each string is a non-empty line.
    * @param filename :: string for name of the .irf file
    * @param lines :: vector of strings for each non-empty line in .irf file
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
      errmsg << "Input .irf file " << filename << " cannot be open. ";
      g_log.error(errmsg.str());
      throw runtime_error(errmsg.str());
    }

    return;
  }

} // namespace DataHandling
} // namespace Mantid




































