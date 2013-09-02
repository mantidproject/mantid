/*WIKI* 

Convert the initial fitting parameters in a Fullprof file to XML format in an [[InstrumentParameterFile]].

*WIKI*/
#include "MantidDataHandling/ConvertFullprofToXml.h"
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

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace std;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(ConvertFullprofToXml)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
 ConvertFullprofToXml::ConvertFullprofToXml()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertFullprofToXml::~ConvertFullprofToXml()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Sets documentation strings for this algorithm
    */
  void ConvertFullprofToXml::initDocs()
  {
    setWikiSummary("Convert the initial fitting parameters in a Fullprof file to XML format in an Instrument Parameter File");
    setOptionalMessage("Convert the initial fitting parameters in a Fullprof file to XML format in an Instrument Parameter File");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
   */
  void ConvertFullprofToXml::init()
  {
    // Input file name
    vector<std::string> exts;
    exts.push_back(".prf");
    exts.push_back(".irf");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an Fullprof file to load.");

    // Output file
    vector<std::string> extso;
    extso.push_back(".xml");
    declareProperty(new FileProperty("Filename", "", FileProperty::Save, extso),
      "The name to give to the parameter file.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Implement abstract Algorithm methods
    */
  void ConvertFullprofToXml::exec()
  {
    // Get input
    string datafile = getProperty("Filename");
    vector<int> outputbankids = getProperty("Banks");

    // Import data
    vector<string> lines;
    loadFile(datafile, lines);

    // Examine bank information
    vector<int> vec_bankinprf;
    map<int, int> bankstartindexmap, bankendindexmap;
    scanBanks(lines, vec_bankinprf, bankstartindexmap, bankendindexmap);
    sort(vec_bankinprf.begin(), vec_bankinprf.end());

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Load file to a vector of strings.  Each string is a non-empty line.
    * @param filename :: string for name of the .irf file
    * @param lines :: vector of strings for each non-empty line in .irf file
    */
  void ConvertFullprofToXml::loadFile(string filename, vector<string>& lines)
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
  void ConvertFullprofToXml::scanBanks(const vector<string>& lines, vector<int>& banks,
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
  void ConvertFullprofToXml::parseResolutionStrings(map<string, double>& parammap, const vector<string>& lines,
                                                      int bankid, int startlineindex, int endlineindex)
  {
    string bankline = lines[startlineindex];
    double cwl;
    int tmpbankid;
    parseBankLine(bankline, cwl, tmpbankid);
    g_log.debug() << "Found CWL = " << cwl << ", Bank ID = " << tmpbankid << "\n";
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
  void ConvertFullprofToXml::parseBankLine(string line, double& cwl, int& bankid)
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

} // namespace DataHandling
} // namespace Mantid




































