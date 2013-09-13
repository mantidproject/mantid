/*WIKI* 

Load Fullprof resolution (.irf) file to TableWorkspace(s)

*WIKI*/
#include "MantidDataHandling/LoadFullprofResolution.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <fstream>

/**
  CHANGE:
  1. Remove the 2nd output for bank information
  2. Make the output to be a n(bank) + 1 column workspace

  **/

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace std;

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

    // Output workspace
    auto wsprop = new WorkspaceProperty<API::ITableWorkspace>("OutputWorkspace", "", Direction::Output);
    declareProperty(wsprop, "Name of the output TableWorkspace containing profile parameters or bank information. ");

    // Bank to import
    declareProperty(new ArrayProperty<int>("Banks"), "ID(s) of specified bank(s) to load. "
                    "Default is all banks contained in input .irf file.");

    // declareProperty("Bank", EMPTY_INT(), "ID of a specific bank to load. Default is all banks in .irf file.");

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

    // Import data
    vector<string> lines;
    loadFile(datafile, lines);

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
      parseResolutionStrings(parammap, lines, bankid, bankstartindexmap[bankid], bankendindexmap[bankid]);
      bankparammap.insert(make_pair(bankid, parammap));
    }

    // Generate output table workspace
    API::ITableWorkspace_sptr outws = genTableWorkspace(bankparammap);

    // 6. Output
    setProperty("OutputWorkspace", outws);

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
  /** Scan lines for bank IDs
    * @param lines :: vector of string of all non-empty lines in input file;
    * @param banks :: [output] vector of integers for existing banks in .irf file;
    * @param bankstartindexmap :: [output] map to indicate the first line of each bank in vector lines.
    * @param bankendindexmap :: [output] map to indicate the last lie of each bank in vector lines
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
    */
  void LoadFullprofResolution::parseResolutionStrings(map<string, double>& parammap, const vector<string>& lines,
                                                      int bankid, int startlineindex, int endlineindex)
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

    double tempdb;
    for (int i = startlineindex+1; i <= endlineindex; ++i)
    {
      string line = lines[i];

      // Skip information line
      if (line[0] == '!')
        continue;

      // Parse
      g_log.debug() << "Parse Line " << i << "\t\t" << line << "\n";

      if (boost::starts_with(line, "TOFRG"))
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
          parammap["tof-min"] = tempdb;
          tempdb = atof(terms[2].c_str());
          parammap["step"] = tempdb;
          tempdb = atof(terms[3].c_str());
          parammap["tof-max"] = tempdb;
        }
      }
      else if (boost::starts_with(line, "D2TOF"))
      {
        vector<string> terms;
        boost::split(terms, line, boost::is_any_of(" "), boost::token_compress_on);
        if (terms.size() != 2 && terms.size() != 4)
        {
          stringstream errmsg;
          errmsg << "Line TOFRG has " << terms.size() << " terms.  Different from 2/4 terms in definition.";
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
          errmsg << "Line TOFRG has " << terms.size() << " terms.  Different from 4 terms in definition.";
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


} // namespace DataHandling
} // namespace Mantid




































