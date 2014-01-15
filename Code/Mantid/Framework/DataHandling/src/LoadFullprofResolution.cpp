/*WIKI* 

Load Fullprof resolution (.irf) file to TableWorkspace(s) and optionally into the instrument of a matrix workspace.

*WIKI*/
#include "MantidDataHandling/LoadFullprofResolution.h"
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

  DECLARE_ALGORITHM(LoadFullprofResolution)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadFullprofResolution::LoadFullprofResolution()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadFullprofResolution::~LoadFullprofResolution()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Sets documentation strings for this algorithm
    */
  void LoadFullprofResolution::initDocs()
  {
    setWikiSummary("Load Fullprof's resolution (.irf) file to one or multiple TableWorkspace(s).");
    setOptionalMessage("Load Fullprof's resolution (.irf) file to one or multiple TableWorkspace(s).");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
   */
  void LoadFullprofResolution::init()
  {
    // Input file name
    vector<std::string> exts;
    exts.push_back(".irf");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an Fullprof .irf file to load.");

    // Output table workspace
    auto wsprop = new WorkspaceProperty<API::ITableWorkspace>("OutputTableWorkspace", "", Direction::Output, PropertyMode::Optional);
    declareProperty(wsprop, "Name of the output TableWorkspace containing profile parameters or bank information. ");

    // Bank to import
    declareProperty(new ArrayProperty<int>("Banks"), "ID(s) of specified bank(s) to load. "
                    "Default is all banks contained in input .irf file.");

    // declareProperty("Bank", EMPTY_INT(), "ID of a specific bank to load. Default is all banks in .irf file.");

    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut, PropertyMode::Optional),
        "Optional: A matrix workspace with the instrument to which we add the parameters from the Fullprof .irf file.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
    */
  void LoadFullprofResolution::exec()
  {
    // Get input
    string datafile = getProperty("Filename");
    vector<int> outputbankids = getProperty("Banks");
    MatrixWorkspace_sptr workspace = getProperty("Workspace");

    // Import data
    vector<string> lines;
    loadFile(datafile, lines);

    // Get Prof number
    int nProf = getProfNumber(lines);

    // Examine bank information
    vector<int> vec_bankinirf;
    map<int, int> bankstartindexmap, bankendindexmap;
    scanBanks(lines, vec_bankinirf, bankstartindexmap, bankendindexmap);
    sort(vec_bankinirf.begin(), vec_bankinirf.end());

    for (size_t i = 0; i < vec_bankinirf.size(); ++i)
      g_log.debug() << "Irf containing bank " << vec_bankinirf[i] << ".\n";

    vector<int> vec_bankids; // bank IDs to output to table workspace

    if (vec_bankinirf.empty())
    {
      throw runtime_error("No Bank is found in input file.");
    }
    else if (outputbankids.size() == 0)
    {
      vec_bankids = vec_bankinirf;
    }
    else
    {
      sort(outputbankids.begin(), outputbankids.end());
      for (size_t i = 0; i < outputbankids.size(); ++i)
      {
        int outputbankid = outputbankids[i];
        if (outputbankid < 0)
        {
          g_log.warning() << "Input bank ID (" << outputbankid << ") is negative.  It is not allowed and ignored. " << ".\n";
        }
        else
        {
          vector<int>::iterator fiter = lower_bound(vec_bankinirf.begin(), vec_bankinirf.end(), outputbankid);
          if (fiter == vec_bankinirf.end() || *fiter != outputbankid)
          {
            // Specified bank ID does not exist.
            stringstream errmsg;
            errmsg << "Specified output bank (ID = " << outputbankid << ") cannot be found in input "
                   << datafile << ", which includes bank : ";
            for (size_t i = 0; i < vec_bankinirf.size(); ++i)
            {
              errmsg << vec_bankinirf[i];
              if (i != vec_bankinirf.size()-1)
                errmsg << ",";
            }
            g_log.error(errmsg.str());
            throw runtime_error(errmsg.str());
          }
          else
          {
            vec_bankids.push_back(outputbankid);
          }
        }
      }
      if (vec_bankids.size() == 0)
      {
        g_log.error("There is no valid specified bank IDs for output.");
        throw runtime_error("There is no valid specified bank IDs for output.");
      }
    }

    // Parse .irf and export profile parameters
    map<int, map<string, double> > bankparammap;
    for (size_t i = 0; i < vec_bankids.size(); ++i)
    {
      int bankid = vec_bankids[i];
      g_log.debug() << "Parse bank " << bankid << " of total " << vec_bankids.size() << ".\n";
      map<string, double> parammap;
      parseResolutionStrings(parammap, lines, bankid, bankstartindexmap[bankid], bankendindexmap[bankid], nProf);
      bankparammap.insert(make_pair(bankid, parammap));
    }

    // Generate output table workspace
    API::ITableWorkspace_sptr outTabWs = genTableWorkspace(bankparammap);

    if( getPropertyValue("OutputTableWorkspace") != "")
    {
      // Output the output table workspace
      setProperty("OutputTableWorkspace", outTabWs);
    }


    // If workspace, put parameters there
    if(workspace)
    {
      putParametersIntoWorkspace( outTabWs, workspace );
    } 
    else if( getPropertyValue("OutputTableWorkspace") == "")
    {
      // We don't know where to output
      throw std::runtime_error("Either the OutputTableWorkspace or Workspace property must be set.");
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Load file to a vector of strings.  Each string is a non-empty line.
    * @param filename :: string for name of the .irf file
    * @param lines :: vector of strings for each non-empty line in .irf file
    */
  void LoadFullprofResolution::loadFile(string filename, vector<string>& lines)
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

  //----------------------------------------------------------------------------------------------
  /** Get the NPROF number
    * @param lines :: vector of string of all non-empty lines in input file;
    */
  int LoadFullprofResolution::getProfNumber( const vector<string>& lines)
  {
    // Assume the NPROF number is on the second line
    if (lines[1].find("NPROF") != string::npos)
    {
       // Split line to get the NPROF number
      size_t nStart = lines[1].find("NPROF");
      size_t nNumber = lines[1].find("=", nStart) + 1;
      size_t nEnd = lines[1].find(" ",nStart); // Assume the NRPOF number is followed by space
      if(nNumber == string::npos + 1 || nEnd == string::npos ) return (-1);
      return( boost::lexical_cast<int> (lines[1].substr(nNumber,nEnd-nNumber)) );
    }

    return(0);
  }

  //----------------------------------------------------------------------------------------------
  /** Scan lines for bank IDs
    * @param lines :: vector of string of all non-empty lines in input file;
    * @param banks :: [output] vector of integers for existing banks in .irf file;
    * @param bankstartindexmap :: [output] map to indicate the first line of each bank in vector lines.
    * @param bankendindexmap :: [output] map to indicate the last line of each bank in vector lines
    */
  void LoadFullprofResolution::scanBanks(const vector<string>& lines, vector<int>& banks,
                                         map<int, int>& bankstartindexmap, map<int, int>& bankendindexmap)
  {
    int startindex = -1;
    int endindex = -1;
    for (size_t i = 0; i < lines.size(); ++i)
    {
      string line = lines[i];
      if (line.find("Bank") != string::npos)
      {
        // A new line found
        if (startindex >= 0)
        {
          // Previous line is in a bank range.  Then finish the previous bank range
          endindex = static_cast<int>(i) - 1;
          bankstartindexmap.insert(make_pair(banks.back(), startindex));
          bankendindexmap.insert(make_pair(banks.back(), endindex));
        }

        // Start the new pair
        startindex = static_cast<int>(i);
        endindex = -1;

        // Split Bank
        vector<string> level1s;
        boost::split(level1s, line, boost::is_any_of("Bank"));
        vector<string> level2s;
        string bankterm = level1s.back();
        boost::algorithm::trim(bankterm);
        boost::split(level2s, bankterm, boost::is_any_of(" "));
        int bankid = atoi(level2s[0].c_str());
        banks.push_back(bankid);
      }
    }
    if (startindex >= 0)
    {
      endindex = static_cast<int>(lines.size())-1;
      bankstartindexmap.insert(make_pair(banks.back(), startindex));
      bankendindexmap.insert(make_pair(banks.back(), endindex));
    }

    g_log.debug() << "[DB1112] Number of bank IDs = " << banks.size() << ", "
         << "Number of ranges = " << bankstartindexmap.size() << endl;
    for (size_t i = 0; i < banks.size(); ++i)
    {
      g_log.debug() << "Bank " << banks[i] << " From line " << bankstartindexmap[banks[i]] << " to "
           << bankendindexmap[banks[i]] << endl;
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse one bank in a .irf file to a map of parameter name and value
    * @param parammap :: [output] parameter name and value map
    * @param lines :: [input] vector of lines from .irf file;
    * @param bankid :: [input] ID of the bank to get parsed
    * @param startlineindex :: [input] index of the first line of the bank in vector of lines
    * @param endlineindex :: [input] index of the last line of the bank in vector of lines
    * @param profNumber :: [input] index of the profile number
    */
  void LoadFullprofResolution::parseResolutionStrings(map<string, double>& parammap, const vector<string>& lines,
                                                      int bankid, int startlineindex, int endlineindex, int profNumber)
  {
    string bankline = lines[startlineindex];
    double cwl;
    int tmpbankid;
    parseBankLine(bankline, cwl, tmpbankid);
    if(tmpbankid != -1) 
    {
      g_log.debug() << "Found CWL = " << cwl << ", Bank ID = " << tmpbankid << "\n";
    }
    else
    {
      g_log.warning() << "No CWL found for bank " << bankid;
      tmpbankid = bankid;
    }
    if (bankid != tmpbankid)
    {
      stringstream errss;
      errss << "Input bank ID (" << bankid << ") is not same as the bank ID (" << tmpbankid
        << ") found in the specified region from input. ";
      throw runtime_error(errss.str());
    }
    parammap["NPROF"] = profNumber;
    parammap["CWL"] = cwl;

    double tempdb;
    for (int i = startlineindex+1; i <= endlineindex; ++i)
    {
      string line = lines[i];

      // Skip information line
      if (line[0] == '!')
        continue;

      // skip NPROF line, which is processed by getProfNumber
      if ( line.find("NPROF") != string::npos )
         continue;

      // Parse
      g_log.debug() << "Parse Line " << i << "\t\t" << line << "\n";

      if (boost::starts_with(line, "TOFRG"))
      {
        // TOFRG tof-min step tof-max
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 4)
        {
          stringstream errmsg;
          errmsg << "Line TOFRG has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["tof-min"] = tempdb;
          tempdb = atof(terms[2].c_str());
          parammap["step"] = tempdb;
          tempdb = atof(terms[3].c_str());
          parammap["tof-max"] = tempdb;
        }
      }
      else if (boost::starts_with(line, "D2TOF"))
      {
        // D2TOF Dtt1 Dtt2 Zero
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 2 && terms.size() != 4)
        {
          stringstream errmsg;
          errmsg << "Line D2TOF has " << terms.size() << " terms.  Different from 2/4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["Dtt1"] = tempdb;
          if (terms.size() == 4)
          {
            tempdb = atof(terms[2].c_str());
            parammap["Dtt2"] = tempdb;
            tempdb = atof(terms[3].c_str());
            parammap["Zero"] = tempdb;
          }
          else
          {
            parammap["Dtt2"] = 0.0;
            parammap["Zero"] = 0.0;
          }
        }
      } // "D2TOF"
      else if (boost::starts_with(line, "ZD2TOF"))
      {
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 3)
        {
          stringstream errmsg;
          errmsg << "Line ZD2TOF has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["Zero"] = tempdb;
          tempdb = atof(terms[2].c_str());
          parammap["Dtt1"] = tempdb;
          parammap["Dtt2"] = 0;
        }
      } // "ZD2TOF"
      else if (boost::starts_with(line, "D2TOT"))
      {
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 6)
        {
          stringstream errmsg;
          errmsg << "Line TOFRG has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["Dtt1t"] = tempdb;
          tempdb = atof(terms[2].c_str());
          parammap["Dtt2t"] = tempdb;
          tempdb = atof(terms[3].c_str());
          parammap["Tcross"] = tempdb;
          tempdb = atof(terms[4].c_str());
          parammap["Width"] = tempdb;
          tempdb = atof(terms[5].c_str());
          parammap["Zerot"] = tempdb;
        }
      } // "D2TOT"
      else if (boost::starts_with(line, "ZD2TOT"))
      {
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 6)
        {
          stringstream errmsg;
          errmsg << "Line TOFRG has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["Zerot"] = tempdb;
          tempdb = atof(terms[2].c_str());
          parammap["Dtt1t"] = tempdb;
          tempdb = atof(terms[3].c_str());
          parammap["Dtt2t"] = tempdb;
          tempdb = atof(terms[4].c_str());
          parammap["Tcross"] = tempdb;
          tempdb = atof(terms[5].c_str());
          parammap["Width"] = tempdb;
        }
      } // "ZD2TOT"
      else if (boost::starts_with(line, "TWOTH"))
      {
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 2)
        {
          stringstream errmsg;
          errmsg << "Line TOFRG has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["twotheta"] = tempdb;
        }
      } // "TWOTH"
      else if (boost::starts_with(line, "SIGMA"))
      {
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 4)
        {
          stringstream errmsg;
          errmsg << "Line TOFRG has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["Sig2"] = sqrt(tempdb);
          tempdb = atof(terms[2].c_str());
          parammap["Sig1"] = sqrt(tempdb);
          tempdb = atof(terms[3].c_str());
          parammap["Sig0"] = sqrt(tempdb);
        }
      } // "SIGMA"
      else if (boost::starts_with(line, "GAMMA"))
      {
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 4)
        {
          stringstream errmsg;
          errmsg << "Line TOFRG has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["Gam2"] = tempdb;
          tempdb = atof(terms[2].c_str());
          parammap["Gam1"] = tempdb;
          tempdb = atof(terms[3].c_str());
          parammap["Gam0"] = tempdb;
        }
      } // "GAMMA"
      else if (boost::starts_with(line, "ALFBE"))
      {
        // ALFBE alph0 beta0 alph1 beta1
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 5)
        {
          stringstream errmsg;
          errmsg << "Line ALFBE has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["Alph0"] = tempdb;
          tempdb = atof(terms[2].c_str());
          parammap["Beta0"] = tempdb;
          tempdb = atof(terms[3].c_str());
          parammap["Alph1"] = tempdb;
          tempdb = atof(terms[4].c_str());
          parammap["Beta1"] = tempdb;
        }
      } // "ALFBE"
      else if (boost::starts_with(line, "ALFBT"))
      {
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 5)
        {
          stringstream errmsg;
          errmsg << "Line ALFBT has " << terms.size() << " terms.  Different from 4 terms in definition.";
          g_log.error(errmsg.str());
          throw runtime_error(errmsg.str());
        }
        else
        {
          tempdb = atof(terms[1].c_str());
          parammap["Alph0t"] = tempdb;
          tempdb = atof(terms[2].c_str());
          parammap["Beta0t"] = tempdb;
          tempdb = atof(terms[3].c_str());
          parammap["Alph1t"] = tempdb;
          tempdb = atof(terms[4].c_str());
          parammap["Beta1t"] = tempdb;
        }
      }  // "ALFBT"
      else if (boost::starts_with(line,"END"))
      {
        // Ignore END line
        g_log.debug("END line of bank." );
      }
      else
      {
        g_log.warning() << "Line '" << line << "' is not processed.\n";
      } // END -IF StartWith

    } // For-all-line

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse a line containig bank information
    */
  void LoadFullprofResolution::parseBankLine(string line, double& cwl, int& bankid)
  {
    // 1. Split along 'Bank'
    std::vector<std::string> v;
    iter_split(v, line, boost::algorithm::first_finder("Bank"));

    // 2. Split the rest around CWL if there is
    string infostr = v[1];
    boost::algorithm::trim(infostr);

    cwl = -1.0;
    bankid = -1;

    if (infostr.find("CWL") != string::npos)
    {
      // There is CWL
      v.clear();
      iter_split(v, infostr, boost::algorithm::first_finder("CWL"));

      // Bank ID
      bankid = atoi(v[0].c_str());

      // CWL
      infostr = v[1];
      v.clear();
      boost::split(v, infostr, boost::is_any_of("=A"));
      for (size_t i = 0; i < v.size(); ++i)
      {
        g_log.debug() << "Last CWL splitted.  Term " << i << ": \t\t" << "'" << v[i] << "'\n";
        string candidate = v[i];
        boost::algorithm::trim(candidate);
        if (candidate.size() > 0)
        {
          cwl = atof(candidate.c_str());
          break;
        }
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate output workspace
    */
  TableWorkspace_sptr LoadFullprofResolution::genTableWorkspace(map<int, map<string, double> > bankparammap)
  {
    g_log.notice() << "Start to generate table workspace ...." << ".\n";

    // Retrieve some information
    size_t numbanks = bankparammap.size();
    if (numbanks == 0)
      throw runtime_error("Unable to generate a table from an empty map!");

    map<int, map<string, double> >::iterator bankmapiter = bankparammap.begin();
    size_t numparams = bankmapiter->second.size();

    // vector of all parameter name
    vector<string> vec_parname;
    vector<int> vec_bankids;

    map<string, double>::iterator parmapiter;
    for (parmapiter = bankmapiter->second.begin(); parmapiter != bankmapiter->second.end(); ++parmapiter)
    {
      string parname = parmapiter->first;
      vec_parname.push_back(parname);
    }

    for (bankmapiter = bankparammap.begin(); bankmapiter != bankparammap.end(); ++bankmapiter)
    {
      int bankid = bankmapiter->first;
      vec_bankids.push_back(bankid);
    }

    g_log.debug() << "[DBx240] Number of imported parameters is " << numparams
                  << ", Number of banks = " << vec_bankids.size() << "." << "\n";

    // Create TableWorkspace
    TableWorkspace_sptr tablews(new TableWorkspace());

    // set columns :
    // Any 2 columns cannot have the same name.
    tablews->addColumn("str", "Name");
    for (size_t i = 0; i < numbanks; ++i)
    {
      stringstream colnamess;
      int bankid = vec_bankids[i];
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
    for (size_t i = 0; i < numparams; ++i)
    {
      TableRow newrow = tablews->appendRow();

      string parname = vec_parname[i];
      newrow << parname;

      for (size_t j = 0; j < numbanks; ++j)
      {
        int bankid = vec_bankids[j];

        // Locate map of bank 'bankid'
        map<int, map<string, double> >::iterator bpmapiter;
        bpmapiter = bankparammap.find(bankid);
        if (bpmapiter == bankparammap.end())
        {
          throw runtime_error("Bank cannot be found in map.");
        }

        // Locate parameter
        map<string, double>::iterator parmapiter;
        parmapiter = bpmapiter->second.find(parname);
        if (parmapiter == bpmapiter->second.end())
        {
          throw runtime_error("Parameter cannot be found in a bank's map.");
        }
        else
        {
          double pvalue = parmapiter->second;
          newrow << pvalue;
        }

      } // END(j)
    } // END(i)

    return tablews;
  }

  void LoadFullprofResolution::putParametersIntoWorkspace( const API::ITableWorkspace_sptr &tws, API::MatrixWorkspace_sptr ws)
  {  

    // Get instrument name from matrix workspace
    std::string instrumentName = ws->getInstrument()->getName();

    // Convert table workspace into DOM XML document
    //   Set up writer to Paremeter file
    DOMWriter writer;
    writer.setNewLine("\n");
    writer.setOptions(XMLWriter::PRETTY_PRINT);

    //   Get current time
    Kernel::DateAndTime date = Kernel::DateAndTime::getCurrentTime();
    std::string ISOdate = date.toISO8601String();
    std::string ISOdateShort = ISOdate.substr(0,19); // Remove fraction of seconds

    //   Create document
    AutoPtr<Document> mDoc = new Document();
    AutoPtr<Element> rootElem = mDoc->createElement("parameter-file");
    rootElem->setAttribute("date", ISOdateShort);
    mDoc->appendChild(rootElem);

    //   Add instrument
    getTableRowNumbers( tws, m_rowNumbers);
    AutoPtr<Element> instrumentElem = mDoc->createElement("component-link");
    instrumentElem->setAttribute("name",instrumentName);
    rootElem->appendChild(instrumentElem);
    API::Column_const_sptr column1 = tws->getColumn( 1 );
    addALFBEParameter( column1, mDoc, instrumentElem, "Alph0");
    addALFBEParameter( column1, mDoc, instrumentElem, "Beta0");
    addALFBEParameter( column1, mDoc, instrumentElem, "Alph1");
    addALFBEParameter( column1, mDoc, instrumentElem, "Beta1");

    //   Add banks
    if(tws->columnCount() < 2){
      throw std::runtime_error("No banks found");
    }
    size_t num_banks = tws->columnCount()-1;

    for( size_t i=0; i<num_banks; ++i)
    {
      API::Column_const_sptr column = tws->getColumn( i+1 );
      const double bankNumber = column->cell<double>(0);
      std::ostringstream bankName;
      bankName << "bank" << bankNumber;
      AutoPtr<Element> bankElem = mDoc->createElement("component-link");
      bankElem->setAttribute("name",bankName.str());
      addSigmaParameters( column, mDoc, bankElem );
      addGammaParameters( column, mDoc, bankElem );
      rootElem->appendChild(bankElem);
    }

    // Convert DOM XML document into string
    std::ostringstream outFile;
    writer.writeNode(outFile, mDoc);  
    std::string parameterXMLString = outFile.str();

    //std::ofstream outfileDebug("C:/Temp/test2_fullprof.xml");
    //outfileDebug << parameterXMLString;
    //outfileDebug.close();


    // Load the string into the workspace
    LoadParameterFile::execManually(true, "", parameterXMLString, ws);

  }

  /* Add an ALFBE parameter to the XML document according to the table workspace
  *
  *  paramName is the name of the parameter as it appears in the table workspace
  */
  void LoadFullprofResolution::addALFBEParameter(const API::Column_const_sptr column, Poco::XML::Document* mDoc, Element* parent, const std::string& paramName)
  {
    AutoPtr<Element> parameterElem = mDoc->createElement("parameter");
    parameterElem->setAttribute("name", getXMLParameterName(paramName));
    parameterElem->setAttribute("type","fitting");

    AutoPtr<Element> formulaElem = mDoc->createElement("formula");
    formulaElem->setAttribute("eq",getXMLEqValue(column, paramName));
    if(paramName != "Beta1") formulaElem->setAttribute("result-unit","TOF");
    parameterElem->appendChild(formulaElem);

    AutoPtr<Element> fixedElem = mDoc->createElement("fixed");
    parameterElem->appendChild(fixedElem);

    parent->appendChild(parameterElem);
  }

    /* Add a set of SIGMA paraters to the XML document according to the table workspace
   * for the bank at the given column of the table workspace
   */
  void LoadFullprofResolution::addSigmaParameters(const API::Column_const_sptr column, Poco::XML::Document* mDoc, Poco::XML::Element* parent )
  {
     AutoPtr<Element> parameterElem = mDoc->createElement("parameter");
     parameterElem->setAttribute("name", "IkedaCarpenterPV:SigmaSquared");
     parameterElem->setAttribute("type","fitting");

     AutoPtr<Element> formulaElem = mDoc->createElement("formula");
     std::string eqValue = getXMLEqValue(column, "Sig1")+"*centre^2+"+getXMLEqValue(column, "Sig0");
     formulaElem->setAttribute("eq", eqValue);
     formulaElem->setAttribute("unit","dSpacing");
     formulaElem->setAttribute("result-unit","TOF^2");
     parameterElem->appendChild(formulaElem);

     parent->appendChild(parameterElem);
  }

   /* Add a set of GAMMA paraters to the XML document according to the table workspace
   * for the bank at the given column of the table workspace
   */
  void LoadFullprofResolution::addGammaParameters(const API::Column_const_sptr column, Poco::XML::Document* mDoc, Poco::XML::Element* parent )
  {
     AutoPtr<Element> parameterElem = mDoc->createElement("parameter");
     parameterElem->setAttribute("name", "IkedaCarpenterPV:Gamma");
     parameterElem->setAttribute("type","fitting");

     AutoPtr<Element> formulaElem = mDoc->createElement("formula");
     std::string eqValue = getXMLEqValue(column, "Gam1" )+"*centre";
     formulaElem->setAttribute("eq", eqValue);
     formulaElem->setAttribute("unit","dSpacing");
     formulaElem->setAttribute("result-unit","TOF");
     parameterElem->appendChild(formulaElem);

     parent->appendChild(parameterElem);
  }



  /*
  *  Get the XML name of a parameter given its Table Workspace name
  */
  std::string LoadFullprofResolution::getXMLParameterName( const std::string& name )
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
  std::string LoadFullprofResolution::getXMLEqValue( const API::Column_const_sptr column, const std::string& name )
  {
    size_t paramNumber = m_rowNumbers[name];
    //API::Column_const_sptr column = tablews->getColumn( columnIndex );
    double eqValue = column->cell<double>(paramNumber);
    if(name.substr(0,3) == "Sig") eqValue = eqValue*eqValue; // Square the sigma values
    return boost::lexical_cast<std::string>(eqValue);
  }

  /* This function fills in a list of the row numbers starting 0 of the parameters
     in the table workspace, so one can find the position in a column of
     the value of the given parameter.
  */
  void LoadFullprofResolution::getTableRowNumbers(const API::ITableWorkspace_sptr & tablews, std::map<std::string, size_t>& parammap)
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




































