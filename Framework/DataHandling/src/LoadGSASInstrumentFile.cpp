// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadGSASInstrumentFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/LoadFullprofResolution.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidKernel/ArrayProperty.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>

#include <algorithm>
#include <fstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace std;
using namespace Poco::XML;

using Geometry::Instrument_const_sptr;
using Geometry::Instrument_sptr;

namespace Mantid::DataHandling {

DECLARE_ALGORITHM(LoadGSASInstrumentFile)

//----------------------------------------------------------------------------------------------
/** Implement abstract Algorithm methods
 */
void LoadGSASInstrumentFile::init() {
  // Input file name
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".prm"),
                  "Path to an GSAS file to load.");

  // Output table workspace
  auto wsprop = std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("OutputTableWorkspace", "", Direction::Output,
                                                                          PropertyMode::Optional);
  declareProperty(std::move(wsprop), "Name of the output TableWorkspace containing "
                                     "instrument parameter information read from file. ");

  // Use bank numbers as given in file
  declareProperty(std::make_unique<PropertyWithValue<bool>>("UseBankIDsInFile", true, Direction::Input),
                  "Use bank IDs as given in file rather than ordinal number of bank. "
                  "If the bank IDs in the file are not unique, it is advised to set this "
                  "to false.");

  // Bank to import
  declareProperty(std::make_unique<ArrayProperty<int>>("Banks"), "ID(s) of specified bank(s) to load, "
                                                                 "The IDs are as specified by UseBankIDsInFile. "
                                                                 "Default is all banks contained in input .prm file.");

  // Workspace to put parameters into. It must be a workspace group with one
  // worskpace per bank from the prm file
  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>("Workspace", "", Direction::InOut, PropertyMode::Optional),
      "A workspace group with the instrument to which we add the "
      "parameters from the GSAS instrument file, with one "
      "workspace for each bank of the .prm file");

  // Workspaces for each bank
  declareProperty(std::make_unique<ArrayProperty<int>>("WorkspacesForBanks"),
                  "For each bank,"
                  " the ID of the corresponding workspace in same order as the "
                  "banks are specified. "
                  "ID=1 refers to the first workspace in the workspace group, "
                  "ID=2 refers to the second workspace and so on. "
                  "Default is all workspaces in numerical order. "
                  "If default banks are specified, they too are taken to be in "
                  "numerical order");
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
    throw std::runtime_error("Error on checking histogram type: " + histType + "\n");
  }

  size_t numBanks = getNumberOfBanks(lines);
  g_log.debug() << numBanks << "banks in file \n";

  // Examine bank information
  vector<size_t> bankStartIndex;
  scanBanks(lines, bankStartIndex);

  if (bankStartIndex.empty()) {
    throw std::runtime_error("No nanks found in file. \n");
  }

  if (numBanks != bankStartIndex.size()) { // The stated number of banks does
                                           // not equal the number of banks
                                           // found
    g_log.warning() << "The number of banks found" << bankStartIndex.size()
                    << "is not equal to the number of banks stated" << numBanks << ".\n";
    g_log.warning() << "Number of banks found is used.";
    numBanks = bankStartIndex.size();
  }

  // Parse banks and export profile parameters
  map<size_t, map<string, double>> bankparammap;
  for (size_t i = 0; i < numBanks; ++i) {
    size_t bankid = i + 1;
    g_log.debug() << "Parse bank " << bankid << " of total " << numBanks << ".\n";
    map<string, double> parammap;
    parseBank(parammap, lines, bankid, bankStartIndex[bankid - 1]);
    bankparammap.emplace(bankid, parammap);
    g_log.debug() << "Bank starts at line" << bankStartIndex[i] + 1 << "\n";
  }

  // Get Workspace property
  WorkspaceGroup_sptr wsg = getProperty("Workspace");
  // Generate output table workspace
  API::ITableWorkspace_sptr outTabWs = genTableWorkspace(bankparammap);
  if (!getPropertyValue("OutputTableWorkspace").empty()) {
    // Output the output table workspace
    setProperty("OutputTableWorkspace", outTabWs);
  }
  if (wsg) {
    vector<int> bankIds = getProperty("Banks");
    vector<int> workspaceIds = getProperty("WorkspacesForBanks");
    map<int, size_t> workspaceOfBank;

    // Deal with bankIds
    if (!bankIds.empty()) {
      // If user provided a list of banks, check that they exist in the .prm
      // file
      const auto it = std::find_if(bankIds.cbegin(), bankIds.cend(),
                                   [&bankparammap](const auto &bankId) { return !bankparammap.count(bankId); });
      if (it != bankIds.cend()) {
        std::stringstream errorString;
        errorString << "Bank " << (*it) << " not found in .prm file";
        throw runtime_error(errorString.str());
      }
    } else {
      // Else, use all available banks
      bankIds.reserve(bankparammap.size());
      std::transform(bankparammap.cbegin(), bankparammap.cend(), std::back_inserter(bankIds),
                     [](const auto &bank) { return static_cast<int>(bank.first); });
    }

    // Generate workspaceOfBank
    LoadFullprofResolution::createBankToWorkspaceMap(bankIds, workspaceIds, workspaceOfBank);
    // Put parameters into workspace group
    LoadFullprofResolution::getTableRowNumbers(outTabWs, LoadFullprofResolution::m_rowNumbers);
    for (size_t i = 0; i < bankIds.size(); ++i) {
      int bankId = bankIds[i];
      size_t wsId = workspaceOfBank[bankId];
      Workspace_sptr wsi = wsg->getItem(wsId - 1);
      auto workspace = std::dynamic_pointer_cast<MatrixWorkspace>(wsi);
      // Get column from table workspace
      API::Column_const_sptr OutTabColumn = outTabWs->getColumn(i + 1);
      std::string parameterXMLString;
      LoadFullprofResolution::putParametersIntoWorkspace(
          OutTabColumn, workspace, static_cast<int>(bankparammap[i]["NPROF"]), parameterXMLString);
      // Load the string into the workspace
      Algorithm_sptr loadParamAlg = createChildAlgorithm("LoadParameterFile");
      loadParamAlg->setProperty("ParameterXML", parameterXMLString);
      loadParamAlg->setProperty("Workspace", workspace);
      loadParamAlg->execute();
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Load file to a vector of strings.  Each string is a non-empty line.
 * @param filename :: string for name of the .prm file
 * @param lines :: vector of strings for each non-empty line in .prm file
 */
void LoadGSASInstrumentFile::loadFile(const string &filename, vector<string> &lines) {
  ifstream myfile(filename.c_str());

  if (myfile.is_open()) {
    while (!myfile.eof()) {
      string line;
      getline(myfile, line);

      boost::algorithm::trim(line);
      if (!line.empty())
        lines.emplace_back(line);
    }

    myfile.close();
  } else {
    stringstream errmsg;
    errmsg << "Input .prm file " << filename << " cannot be open. ";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }
}

/* Get the histogram type
 * @param lines :: vector of strings for each non-empty line in .irf file
 * @return Histogram type code
 */
std::string LoadGSASInstrumentFile::getHistogramType(const vector<string> &lines) {
  // We assume there is just one HTYPE line, look for it from beginning and
  // return its value.
  std::string lookFor = "INS   HTYPE";
  for (size_t i = 0; i < lines.size(); ++i) {
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
  for (size_t i = 0; i < lines.size(); ++i) {
    if (lines[i].substr(0, lookFor.size()) == lookFor) {
      if (lines[i].size() < lookFor.size() + 3) {
        // line too short
        return 0;
      }
      return boost::lexical_cast<size_t>(lines[i].substr(lookFor.size() + 2, 1)); // Found
    }
  }
  return 0;
}

/** Scan lines to determine which line each bank begins
 * @param lines :: vector of string of all non-empty lines in input file;
 * @param bankStartIndex :: [output] vector of start indices of banks in the
 * order they occur in file
 */
void LoadGSASInstrumentFile::scanBanks(const std::vector<std::string> &lines, std::vector<size_t> &bankStartIndex) {
  // We look for each line that contains 'BNKPAR' and take it to be the first
  // line of a bank.
  // We currently ignore the bank number and assume they are numbered according
  // to their order in the file.
  for (size_t i = 0; i < lines.size(); ++i) {
    string line = lines[i];
    if (line.substr(0, 3) == "INS") {            // Ignore all lines that don't begin with INS
      if (line.find("BNKPAR") != string::npos) { // We've found start of a new bank
        bankStartIndex.emplace_back(i);
      }
    } // INS
  } // for(i)
}

//----------------------------------------------------------------------------------------------
/** Parse one bank in a .prm file to a map of parameter name and value
 * @param parammap :: [output] parameter name and value map
 * @param lines :: [input] vector of lines from .irf file;
 * @param bankid :: [input] ID of the bank to get parsed
 * @param startlineindex :: [input] index of the first line of the bank in
 * vector of lines
 */
void LoadGSASInstrumentFile::parseBank(std::map<std::string, double> &parammap, const std::vector<std::string> &lines,
                                       size_t bankid, size_t startlineindex) {
  double param1, param2, param3, param4;

  // We ignore the first lines of the bank
  // The first useful line starts with "INS  nPRCF", where n is the bank number
  // From this line we get the profile function and number of parameters
  size_t currentLineIndex = findINSPRCFLine(lines, startlineindex, param1, param2, param3, param4);

  parammap["NPROF"] = param2;

  // Ikeda-Carpenter PV
  // Then read 15 parameters from the next four INS lines.
  currentLineIndex = findINSPRCFLine(lines, currentLineIndex + 1, param1, param2, param3, param4);
  parammap["Alph0"] = param1;
  parammap["Alph1"] = param2;
  parammap["Beta0"] = param3;
  parammap["Beta1"] = param4; // Kappa

  currentLineIndex = findINSPRCFLine(lines, currentLineIndex + 1, param1, param2, param3, param4);
  parammap["Sig0"] = param1;
  parammap["Sig1"] = param2;
  parammap["Sig2"] = param3;
  parammap["Gam0"] = param4;

  findINSPRCFLine(lines, currentLineIndex + 1, param1, param2, param3, param4);
  parammap["Gam1"] = param1;
  parammap["Gam2"] = param2;
  if (param3 != 0.0) {
    g_log.warning() << "Bank" << bankid << "stec not 0, but " << param3;
  }
  if (param4 != 0.0) {
    g_log.warning() << "Bank" << bankid << "ptec not 0, but " << param4;
  }
}

//----------------------------------------------------------------------------------------------
/** Get next INS line of .prm file at or after given line index
 * @param lines :: [input] vector of lines from .irf file;
 * @param lineIndex :: [input] index of line to search from
 * @param param1 :: [output] first parameter in line
 * @param param2 :: [output] second parameter in line
 * @param param3 :: [output] third parameter in line
 * @param param4 :: [output] fourth parameter in line
 * @return line index for INS file
 * @throw end of file error
 */
size_t LoadGSASInstrumentFile::findINSPRCFLine(const std::vector<std::string> &lines, size_t lineIndex, double &param1,
                                               double &param2, double &param3, double &param4) {
  for (size_t i = lineIndex; i < lines.size(); ++i) {
    string line = lines[i];
    if ((line.substr(0, 3) == "INS") && (line.substr(6, 4) == "PRCF")) {
      std::istringstream paramLine;
      paramLine.str(lines[i].substr(15));
      paramLine >> param1 >> param2 >> param3 >> param4;
      return i;
    }
  }
  throw std::runtime_error("Unexpected end of file reached while searching for INS line. \n");
}

//----------------------------------------------------------------------------------------------
/** Generate output workspace
 *
 * TODO Resolve duplication of this code with
 * LoadFullprofResolution::genetableWorkspace(...)
 */
TableWorkspace_sptr LoadGSASInstrumentFile::genTableWorkspace(map<size_t, map<string, double>> bankparammap) {
  g_log.notice() << "Start to generate table workspace ...."
                 << ".\n";

  // Retrieve some information
  size_t numbanks = bankparammap.size();
  if (numbanks == 0)
    throw runtime_error("Unable to generate a table from an empty map!");

  auto bankmapiter = bankparammap.begin();
  size_t numparams = bankmapiter->second.size();

  // vector of all parameter name
  vector<string> vec_parname;
  vector<size_t> vec_bankids;

  map<string, double>::iterator parmapiter;
  for (parmapiter = bankmapiter->second.begin(); parmapiter != bankmapiter->second.end(); ++parmapiter) {
    string parname = parmapiter->first;
    vec_parname.emplace_back(parname);
  }

  for (bankmapiter = bankparammap.begin(); bankmapiter != bankparammap.end(); ++bankmapiter) {
    size_t bankid = bankmapiter->first;
    vec_bankids.emplace_back(bankid);
  }

  g_log.debug() << "[DBx240] Number of imported parameters is " << numparams
                << ", Number of banks = " << vec_bankids.size() << "."
                << "\n";

  // Create TableWorkspace
  auto tablews = std::make_shared<TableWorkspace>();

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
    newrow = tablews->appendRow();

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
      parmapiter = bpmapiter->second.find(parname);
      if (parmapiter == bpmapiter->second.end()) {
        throw runtime_error("Parameter cannot be found in a bank's map.");
      } else {
        double pvalue = parmapiter->second;
        newrow << pvalue;
      }

    } // END(j)
  } // END(i)

  return tablews;
}

} // namespace Mantid::DataHandling
