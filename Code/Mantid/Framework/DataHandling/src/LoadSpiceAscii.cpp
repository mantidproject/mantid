#include <fstream>
#include <boost/algorithm/string.hpp>

#include "MantidDataHandling/LoadSpiceAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"

using namespace boost::algorithm;

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

namespace Mantid
{
namespace DataHandling
{

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
        else if (line.find('Pt.'))
        {
          // Title line
          boost::split(titles, line, boost::is_any_of(" \t"));
        }
        else if (line.endswith("scan completed") < line.size())
        {
          std::string time = splitByString(line, "scan completed");
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


} // namespace DataHandling
} // namespace Mantid
