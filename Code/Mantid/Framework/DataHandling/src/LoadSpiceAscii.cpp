#include <fstream>
#include <boost/algorithm/string.hpp>

#include "MantidDataHandling/LoadSpiceAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"

#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>

using namespace boost::algorithm;

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

namespace Mantid
{
namespace DataHandling
{

  static bool endswith(const std::string &s, const std::string &subs)
  {
    // s is not long enough
    if (s.size() < subs.size())
      return false;

    // get a substring
    std::string tail = s.substr(s.size()-subs.size());

    if (tail.compare(subs) != 0)
      return false;

    return true;
  }

  static bool checkIntersection(std::vector<std::string> v1, std::vector<std::string> v2)
  {
    throw std::runtime_error("Implement ASAP");
    return true;
  }

  static bool checkIntersection(std::set<std::string> v1, std::set<std::string> v2)
  {
    throw std::runtime_error("Implement ASAP");
    return true;
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadSpiceAscii::LoadSpiceAscii()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadSpiceAscii::~LoadSpiceAscii()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Declaration of properties
   */
  void LoadSpiceAscii::init()
  {
    // Input files
    std::vector<std::string> exts;
    exts.push_back(".dat");
    declareProperty(new FileProperty("Filename", "", API::FileProperty::Load, exts),
                    "Name of SPICE data file.");

    // Logs to be string type sample log
    auto strspckeyprop = new ArrayProperty<std::string>("StringSampleLogNames", Direction::Input);
    declareProperty(strspckeyprop, "List of log names that will be imported as string property.");

    // Logs to be float type sample log
    auto floatspckeyprop = new ArrayProperty<std::string>("FloatSampleLogNames", Direction::Input);
    declareProperty(floatspckeyprop, "List of log names that will be imported as float property.");

    declareProperty("IgnoreUnlistedLogs", false,
                    "If it is true, all log names are not listed in any of above 3 input lists will be ignored. "
                    "Otherwise, any log name is not listed will be treated as string property.");

    // Output
    declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "", Direction::Output),
                    "Name of TableWorkspace containing experimental data.");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("RunInfoWorkspace", "", Direction::Output),
                    "Name of TableWorkspace containing experimental information.");

    return;
  }

  //--
  /**
    */
  void LoadSpiceAscii::exec()
  {
    // Input properties and validate
    std::string filename = getPropertyValue("Filename");
    std::vector<std::string> strlognames = getProperty("StringSampleLogNames");
    std::vector<std::string> intlognames = getProperty("IntSampleLogNames");
    std::vector<std::string> floatlognames = getProperty("FloatSampleLogNames");
    bool ignoreunlisted = getProperty("IgnoreUnlistedLogs");

    bool valid = validateLogNamesType(floatlognames, intlognames, strlognames);
    if (!valid)
      throw std::runtime_error("At one log name appears in multiple log type lists");

    // Parse
    std::vector<std::vector<std::string> > datalist;
    std::vector<std::string> titles;
    std::map<std::string, std::string> runinfodict;
    parseSPICEAscii(filename, datalist, titles, runinfodict);

    // Build output workspaces
    API::ITableWorkspace_sptr outws = createDataWS(datalist, titles);

    // Build run information workspace
    API::MatrixWorkspace_sptr runinfows = createRunInfoWS(runinfodict, floatlognames, intlognames, strlognames, ignoreunlisted);

    // Set properties
    setProperty("OutputWorkspace", outws);
    setProperty("RunInfoWorkspace", runinfows);
  }

  //---
  /** Check whether 3 sets of values have intersection
    */
  bool LoadSpiceAscii::validateLogNamesType(const std::vector<std::string> &floatlognames,
                                            const std::vector<std::string> &intlognames,
                                            const std::vector<std::string>& strlognames)
  {
    std::vector<std::set<std::string> > vec_logsets;

    std::set<std::string> fset;
    for (size_t i = 0; i < floatlognames.size(); ++i)
      fset.insert(floatlognames[i]);
    vec_logsets.push_back(fset);

    std::set<std::string> iset;
    for (size_t i = 0; i < intlognames.size(); ++i)
      iset.insert(intlognames[i]);
    vec_logsets.push_back(iset);

    std::set<std::string> sset;
    for (size_t i = 0; i < strlognames.size(); ++i)
      sset.insert(strlognames[i]);
    vec_logsets.push_back(sset);

    // Check whther there is any intersction among 3 sets
    bool hascommon = false;
    for (size_t i = 0; i < 3; ++i)
    {
      for (size_t j = 0; j < i; ++j)
      {
        hascommon = checkIntersection(vec_logsets[i], vec_logsets[j]);
        if (hascommon)
        {
          std::stringstream ess;
          ess << "logsets[" << i << "] and log sets[" << j << "] has intersection.";
          g_log.error(ess.str());
          break;
        }
      }
    }

    return (!hascommon);
  }

  //------------------------
  /** Parse SPICE Ascii file to dictionary
   * @brief LoadSpiceAscii::parseSPICEAscii
   * @param filename
   * @param a
   */
  void LoadSpiceAscii::parseSPICEAscii(const std::string &filename,
                                       std::vector<std::vector<std::string> > &datalist,
                                       std::vector<std::string>& titles,
                                       std::map<std::string, std::string>& runinfodict)
  {
    // Import file
    std::ifstream spicefile(filename);
    if (!spicefile.is_open())
    {
      std::stringstream ess;
      ess << "File  " << filename << " cannot be opened.";
      throw std::runtime_error(ess.str());
    }

    std::string line;
    while(std::getline(spicefile, line))
    {
      // Parse one line

      // Strip
      boost::trim(line);
      // skip for empyt line
      if (line.size() == 0)
        continue;

      // Comment line for run information
      if (line[0] == '#')
      {
        // remove comment flag # and trim space
        line.erase(0, 1);
        boost::trim(line);

        if (line.find('='))
        {
          // run information line
          std::vector<std::string> terms;
          boost::split(terms, line, boost::is_any_of("="));
          boost::trim(terms[0]);
          std::string infovalue("");
          if (terms.size() == 2)
            boost::trim(terms[1]);
          else
            g_log.warning("Something is not right.");
          runinfodict.insert(std::make_pair(terms[0], infovalue));
        }
        else if (line.find("Pt."))
        {
          // Title line
          boost::split(titles, line, boost::is_any_of(" \t"));
        }
        else if (endswith(line, "scan completed"))
        {
          std::vector<std::string> terms;
          boost::iter_split(terms, line, boost::algorithm::first_finder("scan completed"));
          std::string time = terms.back();
          boost::trim(time);
          runinfodict.insert(std::make_pair("runend", time));
        }
        else
        {
          // Not supported
          std::stringstream wss;
          wss << "Line " << line << " cannot be parsed. It is ignored then.";
          g_log.warning(wss.str());
        }
      } // If for run info
      else
      {
        // data line
        std::vector<std::string> terms;
        boost::split(terms, line, boost::is_any_of(" \t\n"));
        datalist.push_back(terms);
      }
    }

    return;
  }


  //-----
  /** Create the table workspace containing experimental data
  Each row is a data point measured in experiment
   * @brief LoadSpiceAscii::createDataWS
   * @param datalist
   * @param titles
   * @return
   */
  API::ITableWorkspace_sptr LoadSpiceAscii::createDataWS(const std::vector<std::vector<std::string> >& datalist,
                                                         const std::vector<std::string> &titles)
  {
    // Create a table workspace with columns defined
    DataObjects::TableWorkspace_sptr outws = boost::make_shared<DataObjects::TableWorkspace>();
    size_t ipt = -1;
    for (size_t i = 0; i < titles.size(); ++i)
    {
      if (titles[i].compare("Pt.") == 0)
      {
        outws->addColumn("int", titles[i]);
        ipt = i;
      }
      else
      {
        outws->addColumn("double", titles[i]);
      }
    }

    // Add rows
    size_t numrows = datalist.size();
    size_t numcols = outws->columnCount();
    for (size_t irow = 0; irow < numrows; ++irow)
    {
      TableRow newrow = outws->appendRow();
      for (size_t icol = 0; icol < numcols; ++icol)
      {
        std::string item = datalist[irow][icol];
        if (icol == ipt)
          newrow << atoi(item.c_str());
        else
          newrow << atof(item.c_str());
      }
    }

    ITableWorkspace_sptr tablews = boost::dynamic_pointer_cast<ITableWorkspace>(outws);
    return tablews;
  }


  //-----------------------------------------
  /** Create run information workspace
   * @brief LoadSpiceAscii::createRunInfoWS
   * @param runinfodict
   * @param floatlognamelist
   * @param intlognamelist
   * @param strlognamelist
   * @param ignoreunlisted
   * @return
   */
  API::MatrixWorkspace_sptr LoadSpiceAscii::createRunInfoWS(std::map<std::string, std::string> runinfodict,
                                                            std::vector<std::string> floatlognamelist,
                                                            std::vector<std::string> intlognamelist,
                                                            std::vector<std::string> strlognamelist,
                                                            bool ignoreunlisted)
  {
    // Create an empty workspace
    API::MatrixWorkspace_sptr infows
        = WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);

    throw std::runtime_error("Set up VerticalAxisUnit='SpectraNumber'");

    // Sort
    std::sort(floatlognamelist.begin(), floatlognamelist.end());
    std::sort(intlognamelist.begin(), intlognamelist.end());
    std::sort(strlognamelist.begin(), strlognamelist.end());

    // Create sample log properties
    std::map<std::string, std::string>::iterator miter;
    for (miter = runinfodict.begin(); miter != runinfodict.end(); ++miter)
    {
      const std::string title = miter->first;
      const std::string strvalue = miter->second;

      if (std::find(floatlognamelist.begin(), floatlognamelist.end(), title) != floatlognamelist.end())
      {
        // Case as a double property
        bool adderrorvalue = false;
        double value, error;

        // Convert to float value and error (if exists)
        if (strvalue.find("+/-") != std::string::npos)
        {
          adderrorvalue = true;

          std::vector<std::string> terms;
          boost::iter_split(terms, strvalue, boost::algorithm::first_finder("+/-"));
          value = atof(terms[0].c_str());
          error = atof(terms[1].c_str());
        }
        else
        {
          value = atof(strvalue.c_str());
          error = 0;
        }

        // Add properties
        addFloatProperty(infows, title, value);
        if (adderrorvalue)
        {
          std::stringstream tss;
          tss << title << ".error";
          addFloatProperty(infows, tss.str(), error);
        }
      }
      else if (std::find(intlognamelist.begin(), intlognamelist.end(), title) != intlognamelist.end())
      {
        // It is an integer log
        addIntegerProperty(infows, title, atoi(strvalue.c_str()));
      }
      else if (!ignoreunlisted || std::find(strlognamelist.begin(), strlognamelist.end(), title) != strlognamelist.end())
      {
        // It is a string log or it is not defined but not ignored either
        addStringProperty(infows, title, strvalue);
      }

    }

    return infows;
  }

  void LoadSpiceAscii::addFloatProperty(API::MatrixWorkspace_sptr ws, const std::string &pname, float pvalue)
  {

  }

  void LoadSpiceAscii::addIntegerProperty(API::MatrixWorkspace_sptr ws, const std::string &pname, int ivalue)
  {

  }

  void LoadSpiceAscii::addStringProperty(API::MatrixWorkspace_sptr ws, const std::string &pname, const std::string& svalue)
  {

  }


} // namespace DataHandling
} // namespace Mantid
