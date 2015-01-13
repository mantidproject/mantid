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

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadGSASInstrumentFile)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadGSASInstrumentFile::LoadGSASInstrumentFile() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadGSASInstrumentFile::~LoadGSASInstrumentFile() {}

//----------------------------------------------------------------------------------------------
/** Implement abstract Algorithm methods
 */
void LoadGSASInstrumentFile::init() {
  // Input file name
  vector<std::string> exts;
  exts.push_back(".prm");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "Path to an GSAS file to load.");

  // Output table workspace
  auto wsprop = new WorkspaceProperty<API::ITableWorkspace>(
      "OutputTableWorkspace", "", Direction::Output, PropertyMode::Optional);
  declareProperty(wsprop, "Name of the output TableWorkspace containing "
                          "instrument parameter information read from file. ");

  return;
}

//----------------------------------------------------------------------------------------------
/** Implement abstract Algorithm methods
  */
void LoadGSASInstrumentFile::exec() {
  // Get input
  string datafile = getProperty("Filename");

  // Import data
  vector<string> lines;
  loadFile(datafile, lines);

  // Check Histogram type - only PNTR is currently supported
  std::string histType = getHistogramType(lines);
  if (histType != "PNTR") {
    if (histType.size() == 4) {
      throw std::runtime_error("Histogram type " + histType +
                               " not supported \n");
    } else {
      throw std::runtime_error("Error on checking histogram type: " + histType +
                               "\n");
    }
  }

  // Get function type
  // Initially we assume Ikeda Carpenter PV
  int nProf = 9;

  size_t numBanks = getNumberOfBanks(lines);
  g_log.debug() << numBanks << "banks in file \n";

  // Examine bank information
  vector<size_t> bankStartIndex;
  scanBanks(lines, bankStartIndex);

  if (bankStartIndex.size() == 0) {
    throw std::runtime_error("No nanks found in file. \n");
  }

  if (numBanks != bankStartIndex.size()) { // The stated number of banks does
                                           // not equal the number of banks
                                           // found
    g_log.warning() << "The number of banks found" << bankStartIndex.size()
                    << "is not equal to the number of banks stated" << numBanks
                    << ".\n";
    g_log.warning() << "Number of banks found is used.";
    numBanks = bankStartIndex.size();
  }

  // Parse banks and export profile parameters
  map<size_t, map<string, double>> bankparammap;
  for (size_t i = 0; i < numBanks; ++i) {
    size_t bankid = i + 1;
    g_log.debug() << "Parse bank " << bankid << " of total " << numBanks
                  << ".\n";
    map<string, double> parammap;
    parseBank(parammap, lines, bankid, bankStartIndex[bankid - 1], nProf);
    bankparammap.insert(make_pair(bankid, parammap));
    g_log.debug() << "Bank starts at line" << bankStartIndex[i] + 1 << "\n";
  }

  // Generate output table workspace
  API::ITableWorkspace_sptr outTabWs = genTableWorkspace(bankparammap);

  if (getPropertyValue("OutputTableWorkspace") != "") {
    // Output the output table workspace
    setProperty("OutputTableWorkspace", outTabWs);
  }

  if (getPropertyValue("OutputTableWorkspace") == "") {
    // We don't know where to output
    throw std::runtime_error("OutputTableWorkspace must be set.");
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Load file to a vector of strings.  Each string is a non-empty line.
  * @param filename :: string for name of the .prm file
  * @param lines :: vector of strings for each non-empty line in .prm file
  */
void LoadGSASInstrumentFile::loadFile(string filename, vector<string> &lines) {
  string line;

  // the variable of type ifstream:
  ifstream myfile(filename.c_str());

  // check to see if the file is opened:
  if (myfile.is_open()) {
    // while there are still lines in the
    // file, keep reading:
    while (!myfile.eof()) {
      // place the line from myfile into the
      // line variable:
      getline(myfile, line);

      // display the line we gathered:
      boost::algorithm::trim(line);
      if (line.size() > 0)
        lines.push_back(line);
    }

    // close the stream:
    myfile.close();
  } else {
    stringstream errmsg;
    errmsg << "Input .prm file " << filename << " cannot be open. ";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }

  return;
}

/* Get the histogram type
* @param lines :: vector of strings for each non-empty line in .irf file
* @return Histogram type code
*/
std::string
LoadGSASInstrumentFile::getHistogramType(const vector<string> &lines) {
  // We assume there is just one HTYPE line, look for it from beginning and
  // return its value.
  std::string lookFor = "INS   HTYPE";
  for (size_t i = 0; i <= lines.size(); ++i) {
    if (lines[i].substr(0, lookFor.size()) == lookFor) {
      if (lines[i].size() < lookFor.size() + 7) {
        // line too short
        return "HTYPE line too short";
      }
      return lines[i].substr(lookFor.size() + 3, 4); // Found
    }
  }
  return "HTYPE line not found";
}

/* Get number of banks
 * @param lines :: vector of strings for each non-empty line in .irf file
 * @return number of banks
 */
size_t LoadGSASInstrumentFile::getNumberOfBanks(const vector<string> &lines) {
  // We assume there is just one BANK line, look for it from beginning and
  // return its value.
  std::string lookFor = "INS   BANK";
  for (size_t i = 0; i <= lines.size(); ++i) {
    if (lines[i].substr(0, lookFor.size()) == lookFor) {
      if (lines[i].size() < lookFor.size() + 3) {
        // line too short
        return 0;
      }
      return boost::lexical_cast<size_t>(
          lines[i].substr(lookFor.size() + 2, 1)); // Found
    }
  }
  return 0;
}

/** Scan lines to determine which line each bank begins
* @param lines :: vector of string of all non-empty lines in input file;
* @param bankStartIndex :: [output] vector of start indices of banks in the
* order they occur in file
*/
void LoadGSASInstrumentFile::scanBanks(const std::vector<std::string> &lines,
                                       std::vector<size_t> &bankStartIndex) {
  // We look for each line that contains 'BNKPAR' and take it to be the first
  // line of a bank.
  // We currently ignore the bank number and assume they are numbered according
  // to their order in the file.
  for (size_t i = 0; i < lines.size(); ++i) {
    string line = lines[i];
    if (line.substr(0, 3) ==
        "INS") { // Ignore all lines that don't begin with INS
      if (line.find("BNKPAR") !=
          string::npos) { // We've found start of a new bank
        bankStartIndex.push_back(i);
      }
    } // INS
  }   // for(i)
}

//----------------------------------------------------------------------------------------------
/** Parse one bank in a .prm file to a map of parameter name and value
* @param parammap :: [output] parameter name and value map
* @param lines :: [input] vector of lines from .irf file;
* @param bankid :: [input] ID of the bank to get parsed
* @param startlineindex :: [input] index of the first line of the bank in vector
* of lines
* @param profNumber :: [input] index of the profile number
*/
void LoadGSASInstrumentFile::parseBank(std::map<std::string, double> &parammap,
                                       const std::vector<std::string> &lines,
                                       size_t bankid, size_t startlineindex,
                                       int profNumber) {
  parammap["NPROF"] = profNumber; // Add numerical code for function as
                                  // LoadFullprofResolution does.

  // We ignore the first three INS lines of the bank, then read 15 parameters
  // from the next four INS lines.
  size_t currentLineIndex = findINSLine(lines, startlineindex);
  size_t start = 15;
  // ignore 1st line
  currentLineIndex = findINSLine(lines, currentLineIndex + 1);
  // ignore 2nd line
  currentLineIndex = findINSLine(lines, currentLineIndex + 1);
  // ignore 3rd line
  currentLineIndex = findINSLine(lines, currentLineIndex + 1);
  // process 4th line
  std::istringstream paramLine4;
  double param1, param2, param3, param4;
  paramLine4.str(lines[currentLineIndex].substr(start));
  paramLine4 >> param1 >> param2 >> param3 >> param4;
  parammap["Alph0"] = param1;
  parammap["Alph1"] = param2;
  parammap["Beta0"] = param3;
  parammap["Beta1"] = param4; // Kappa

  currentLineIndex = findINSLine(lines, currentLineIndex + 1);
  // process 5th line
  std::istringstream paramLine5;
  double param5, param6, param7, param8;
  paramLine5.str(lines[currentLineIndex].substr(start));
  paramLine5 >> param5 >> param6 >> param7 >> param8;
  parammap["Sig0"] = param5;
  parammap["Sig1"] = param6;
  parammap["Sig2"] = param7;
  parammap["Gam0"] = param8;

  currentLineIndex = findINSLine(lines, currentLineIndex + 1);
  // process 6th line
  std::istringstream paramLine6;
  double param9, param10, param11, param12;
  paramLine6.str(lines[currentLineIndex].substr(start));
  paramLine6 >> param9 >> param10 >> param11 >> param12;
  parammap["Gam1"] = param9;
  parammap["Gam2"] = param10;
  if (param11 != 0.0) {
    g_log.warning() << "Bank" << bankid << "stec not 0, but " << param3;
  }
  if (param12 != 0.0) {
    g_log.warning() << "Bank" << bankid << "ptec not 0, but " << param4;
  }

  // ignore 7th line
}

//----------------------------------------------------------------------------------------------
/** Get next INS line of .prm file at or after given line index
* @param lines :: [input] vector of lines from .irf file;
* @param lineIndex :: [input] index of line to search from
* @return line index for INS file
* @throw end of file error
*/
size_t
LoadGSASInstrumentFile::findINSLine(const std::vector<std::string> &lines,
                                    size_t lineIndex) {
  for (size_t i = lineIndex; i < lines.size(); ++i) {
    string line = lines[i];
    if (line.substr(0, 3) == "INS")
      return i;
  }
  throw std::runtime_error(
      "Unexpected end of file reached while searching for INS line. \n");
}

//----------------------------------------------------------------------------------------------
/** Generate output workspace
*
* TODO Resolve duplication of this code with
* LoadFullprofResolution::genetableWorkspace(...)
*/
TableWorkspace_sptr LoadGSASInstrumentFile::genTableWorkspace(
    map<size_t, map<string, double>> bankparammap) {
  g_log.notice() << "Start to generate table workspace ...."
                 << ".\n";

  // Retrieve some information
  size_t numbanks = bankparammap.size();
  if (numbanks == 0)
    throw runtime_error("Unable to generate a table from an empty map!");

  map<size_t, map<string, double>>::iterator bankmapiter = bankparammap.begin();
  size_t numparams = bankmapiter->second.size();

  // vector of all parameter name
  vector<string> vec_parname;
  vector<size_t> vec_bankids;

  map<string, double>::iterator parmapiter;
  for (parmapiter = bankmapiter->second.begin();
       parmapiter != bankmapiter->second.end(); ++parmapiter) {
    string parname = parmapiter->first;
    vec_parname.push_back(parname);
  }

  for (bankmapiter = bankparammap.begin(); bankmapiter != bankparammap.end();
       ++bankmapiter) {
    size_t bankid = bankmapiter->first;
    vec_bankids.push_back(bankid);
  }

  g_log.debug() << "[DBx240] Number of imported parameters is " << numparams
                << ", Number of banks = " << vec_bankids.size() << "."
                << "\n";

  // Create TableWorkspace
  TableWorkspace_sptr tablews(new TableWorkspace());

  // set columns :
  // Any 2 columns cannot have the same name.
  tablews->addColumn("str", "Name");
  for (size_t i = 0; i < numbanks; ++i) {
    stringstream colnamess;
    size_t bankid = vec_bankids[i];
    colnamess << "Value_" << bankid;
    tablews->addColumn("double", colnamess.str());
  }

  g_log.debug() << "Number of column = " << tablews->columnCount() << ".\n";

  // add BANK ID row
  TableRow newrow = tablews->appendRow();
  newrow << "BANK";
  for (size_t i = 0; i < numbanks; ++i)
    newrow << static_cast<double>(vec_bankids[i]);

  g_log.debug() << "Number of row now = " << tablews->rowCount() << ".\n";

  // add profile parameter rows
  for (size_t i = 0; i < numparams; ++i) {
    TableRow newrow = tablews->appendRow();

    string parname = vec_parname[i];
    newrow << parname;

    for (size_t j = 0; j < numbanks; ++j) {
      size_t bankid = vec_bankids[j];

      // Locate map of bank 'bankid'
      map<size_t, map<string, double>>::iterator bpmapiter;
      bpmapiter = bankparammap.find(bankid);
      if (bpmapiter == bankparammap.end()) {
        throw runtime_error("Bank cannot be found in map.");
      }

      // Locate parameter
      map<string, double>::iterator parmapiter;
      parmapiter = bpmapiter->second.find(parname);
      if (parmapiter == bpmapiter->second.end()) {
        throw runtime_error("Parameter cannot be found in a bank's map.");
      } else {
        double pvalue = parmapiter->second;
        newrow << pvalue;
      }

    } // END(j)
  }   // END(i)

  return tablews;
}

} // namespace DataHandling
} // namespace Mantid
