#include <fstream>
#include <boost/algorithm/string.hpp>

#include "MantidDataHandling/LoadSpiceAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidKernel/ArrayProperty.h"

#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>

using namespace boost::algorithm;

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

namespace Mantid {
namespace DataHandling {

// DECLARE_FILELOADER_ALGORITHM(LoadSpiceAscii)
DECLARE_ALGORITHM(LoadSpiceAscii)

static bool endswith(const std::string &s, const std::string &subs) {
  // s is not long enough
  if (s.size() < subs.size())
    return false;

  // get a substring
  std::string tail = s.substr(s.size() - subs.size());

  if (tail.compare(subs) != 0)
    return false;

  return true;
}

static bool checkIntersection(std::vector<std::string> v1,
                              std::vector<std::string> v2) {
  // Sort
  std::sort(v1.begin(), v1.end());
  std::sort(v2.begin(), v2.end());

  // Check intersectiom
  std::vector<std::string> intersectvec(v1.size() + v2.size());
  std::vector<std::string>::iterator outiter = std::set_intersection(
      v1.begin(), v1.end(), v2.begin(), v2.end(), intersectvec.begin());
  if (static_cast<int>(outiter - intersectvec.begin()) == 0)
    return false;

  return true;
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadSpiceAscii::LoadSpiceAscii() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadSpiceAscii::~LoadSpiceAscii() {}

//----------------------------------------------------------------------------------------------
/** Name
 * @brief LoadSpiceAscii::name
 * @return
 */
const std::string LoadSpiceAscii::name() const { return "LoadSpiceAscii"; }

//----------------------------------------------------------------------------------------------
/** Version
 * @brief LoadSpiceAscii::version
 * @return
 */
int LoadSpiceAscii::version() const { return 1; }

//----------------------------------------------------------------------------------------------
/** Category
 */
const std::string LoadSpiceAscii::category() const { return "DataHandling"; }

//----------------------------------------------------------------------------------------------
/** Summary
 */
const std::string LoadSpiceAscii::summary() const {
  return "Load Spice data to workspaces in general.";
}

//----------------------------------------------------------------------------------------------
/** Declaration of properties
 */
void LoadSpiceAscii::init() {
  // Input files
  std::vector<std::string> exts;
  exts.push_back(".dat");
  declareProperty(
      new FileProperty("Filename", "", API::FileProperty::Load, exts),
      "Name of SPICE data file.");

  // Logs to be float type sample log
  auto floatspckeyprop =
      new ArrayProperty<std::string>("FloatSampleLogNames", Direction::Input);
  declareProperty(floatspckeyprop,
                  "List of log names that will be imported as float property.");

  // Logs to be integer type sample log
  auto intspckeyprop =
      new ArrayProperty<std::string>("IntegerSampleLogNames", Direction::Input);
  declareProperty(
      intspckeyprop,
      "List of log names that will be imported as integer property.");

  // Logs to be string type sample log
  auto strspckeyprop =
      new ArrayProperty<std::string>("StringSampleLogNames", Direction::Input);
  declareProperty(
      strspckeyprop,
      "List of log names that will be imported as string property.");

  declareProperty("IgnoreUnlistedLogs", false,
                  "If it is true, all log names are not listed in any of above "
                  "3 input lists will be ignored. "
                  "Otherwise, any log name is not listed will be treated as "
                  "string property.");

  declareProperty(new ArrayProperty<std::string>("DateAndTimeLog"),
                  "Name and format for date and time");

  // Output
  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Name of TableWorkspace containing experimental data.");

  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("RunInfoWorkspace", "",
                                             Direction::Output),
      "Name of TableWorkspace containing experimental information.");

  return;
}

//----------------------------------------------------------------------------------------------
/** Exec
  */
void LoadSpiceAscii::exec() {
  // Input properties and validate
  std::string filename = getPropertyValue("Filename");
  std::vector<std::string> strlognames = getProperty("StringSampleLogNames");
  std::vector<std::string> intlognames = getProperty("IntegerSampleLogNames");
  std::vector<std::string> floatlognames = getProperty("FloatSampleLogNames");
  bool ignoreunlisted = getProperty("IgnoreUnlistedLogs");
  std::vector<std::string> datetimeprop = getProperty("DateAndTimeLog");

  bool valid = validateLogNamesType(floatlognames, intlognames, strlognames);
  if (!valid)
    throw std::runtime_error(
        "At one log name appears in multiple log type lists");

  // Parse
  std::vector<std::vector<std::string> > datalist;
  std::vector<std::string> titles;
  std::map<std::string, std::string> runinfodict;
  parseSPICEAscii(filename, datalist, titles, runinfodict);

  // Build output workspaces
  API::ITableWorkspace_sptr outws = createDataWS(datalist, titles);

  // Build run information workspace
  API::MatrixWorkspace_sptr runinfows = createRunInfoWS(
      runinfodict, floatlognames, intlognames, strlognames, ignoreunlisted);

  // Process date and time for run start explicitly
  setupRunStartTime(runinfows, datetimeprop);

  // Set properties
  setProperty("OutputWorkspace", outws);
  setProperty("RunInfoWorkspace", runinfows);
}

//----------------------------------------------------------------------------------------------
/** Check whether 3 sets of values have intersection
 * @brief LoadSpiceAscii::validateLogNamesType
 * @param floatlognames
 * @param intlognames
 * @param strlognames
 * @return
 */
bool LoadSpiceAscii::validateLogNamesType(
    const std::vector<std::string> &floatlognames,
    const std::vector<std::string> &intlognames,
    const std::vector<std::string> &strlognames) {
  std::vector<std::vector<std::string> > vec_lognamelist;
  vec_lognamelist.push_back(floatlognames);
  vec_lognamelist.push_back(intlognames);
  vec_lognamelist.push_back(strlognames);

  // Check whther there is any intersction among 3 sets
  bool hascommon = false;
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = i + 1; j < 3; ++j) {
      hascommon = checkIntersection(vec_lognamelist[i], vec_lognamelist[j]);
      if (hascommon) {
        std::stringstream ess;
        ess << "logsets[" << i << "] and log sets[" << j
            << "] has intersection.";
        g_log.error(ess.str());
        break;
      }
    }

  return (!hascommon);
}

//----------------------------------------------------------------------------------------------
/** Parse SPICE Ascii file to dictionary
 * @brief LoadSpiceAscii::parseSPICEAscii
 * @param filename
 * @param datalist
 * @param titles
 * @param runinfodict
 */
void LoadSpiceAscii::parseSPICEAscii(
    const std::string &filename,
    std::vector<std::vector<std::string> > &datalist,
    std::vector<std::string> &titles,
    std::map<std::string, std::string> &runinfodict) {
  // Import file
  std::ifstream spicefile(filename.c_str());
  if (!spicefile.is_open()) {
    std::stringstream ess;
    ess << "File  " << filename << " cannot be opened.";
    throw std::runtime_error(ess.str());
  }

  std::string line;
  while (std::getline(spicefile, line)) {
    // Parse one line

    // Strip
    boost::trim(line);
    // skip for empyt line
    if (line.size() == 0)
      continue;

    // Comment line for run information
    if (line[0] == '#') {
      // remove comment flag # and trim space
      line.erase(0, 1);
      boost::trim(line);

      if (line.find('=') != std::string::npos) {
        // run information line
        std::vector<std::string> terms;
        boost::split(terms, line, boost::is_any_of("="));
        boost::trim(terms[0]);
        g_log.debug() << "Title = " << terms[0]
                      << ", number of splitted terms = " << terms.size()
                      << "\n";
        std::string infovalue("");
        if (terms.size() == 2) {
          infovalue = terms[1];
          boost::trim(infovalue);
        } else if (terms.size() > 2) {
          // Content contains '='
          for (size_t j = 1; j < terms.size(); ++j) {
            if (j > 1)
              infovalue += "=";
            infovalue += terms[j];
          }
        } else {
          std::stringstream wss;
          wss << "Line '" << line
              << "' is hard to parse.  It has more than 1 '='.";
          g_log.warning(wss.str());
        }
        runinfodict.insert(std::make_pair(terms[0], infovalue));
      } else if (line.find("Pt.") != std::string::npos) {
        // Title line
        boost::split(titles, line, boost::is_any_of("\t\n "),
                     boost::token_compress_on);
      } else if (endswith(line, "scan completed.")) {
        std::vector<std::string> terms;
        boost::iter_split(terms, line,
                          boost::algorithm::first_finder("scan completed."));
        std::string time = terms.front();
        boost::trim(time);
        runinfodict.insert(std::make_pair("runend", time));
      } else {
        // Not supported
        std::stringstream wss;
        wss << "Line " << line << " cannot be parsed. It is ignored then.";
        g_log.warning(wss.str());
      }
    } // If for run info
    else {
      // data line
      std::vector<std::string> terms;
      boost::split(terms, line, boost::is_any_of(" \t\n"),
                   boost::token_compress_on);
      datalist.push_back(terms);
    }
  }

  g_log.debug() << "Run info dictionary has " << runinfodict.size()
                << " entries."
                << "\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Create the table workspace containing experimental data
Each row is a data point measured in experiment
 * @brief LoadSpiceAscii::createDataWS
 * @param datalist
 * @param titles
 * @return
 */
API::ITableWorkspace_sptr LoadSpiceAscii::createDataWS(
    const std::vector<std::vector<std::string> > &datalist,
    const std::vector<std::string> &titles) {
  // Create a table workspace with columns defined
  DataObjects::TableWorkspace_sptr outws =
      boost::make_shared<DataObjects::TableWorkspace>();
  size_t ipt = -1;
  for (size_t i = 0; i < titles.size(); ++i) {
    if (titles[i].compare("Pt.") == 0) {
      outws->addColumn("int", titles[i]);
      ipt = i;
    } else {
      outws->addColumn("double", titles[i]);
    }
  }

  // Add rows
  size_t numrows = datalist.size();
  size_t numcols = outws->columnCount();
  for (size_t irow = 0; irow < numrows; ++irow) {
    TableRow newrow = outws->appendRow();
    for (size_t icol = 0; icol < numcols; ++icol) {
      std::string item = datalist[irow][icol];
      if (icol == ipt)
        newrow << atoi(item.c_str());
      else
        newrow << atof(item.c_str());
    }
  }

  ITableWorkspace_sptr tablews =
      boost::dynamic_pointer_cast<ITableWorkspace>(outws);
  return tablews;
}

//----------------------------------------------------------------------------------------------
/** Create run information workspace
 * @brief LoadSpiceAscii::createRunInfoWS
 * @param runinfodict
 * @param floatlognamelist
 * @param intlognamelist
 * @param strlognamelist
 * @param ignoreunlisted
 * @return
 */
API::MatrixWorkspace_sptr
LoadSpiceAscii::createRunInfoWS(std::map<std::string, std::string> runinfodict,
                                std::vector<std::string> &floatlognamelist,
                                std::vector<std::string> &intlognamelist,
                                std::vector<std::string> &strlognamelist,
                                bool ignoreunlisted) {
  // Create an empty workspace
  API::MatrixWorkspace_sptr infows =
      WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);

  // Sort
  std::sort(floatlognamelist.begin(), floatlognamelist.end());
  std::sort(intlognamelist.begin(), intlognamelist.end());
  std::sort(strlognamelist.begin(), strlognamelist.end());

  // Create sample log properties
  std::map<std::string, std::string>::iterator miter;
  for (miter = runinfodict.begin(); miter != runinfodict.end(); ++miter) {
    const std::string title = miter->first;
    const std::string strvalue = miter->second;

    g_log.debug() << "Trying to add property " << title << " with value "
                  << strvalue << "\n";

    if (std::find(floatlognamelist.begin(), floatlognamelist.end(), title) !=
        floatlognamelist.end()) {
      // Case as a double property
      bool adderrorvalue = false;
      double value, error;

      // Convert to float value and error (if exists)
      if (strvalue.find("+/-") != std::string::npos) {
        adderrorvalue = true;

        std::vector<std::string> terms;
        boost::iter_split(terms, strvalue,
                          boost::algorithm::first_finder("+/-"));
        value = atof(terms[0].c_str());
        error = atof(terms[1].c_str());
      } else {
        value = atof(strvalue.c_str());
        error = 0;
      }

      // Add properties
      addProperty<double>(infows, title, value);
      if (adderrorvalue) {
        std::stringstream tss;
        tss << title << ".error";
        addProperty<double>(infows, tss.str(), error);
      }
    } else if (std::find(intlognamelist.begin(), intlognamelist.end(), title) !=
               intlognamelist.end()) {
      // It is an integer log
      addProperty<int>(infows, title, atoi(strvalue.c_str()));
    } else if (!ignoreunlisted ||
               std::find(strlognamelist.begin(), strlognamelist.end(), title) !=
                   strlognamelist.end()) {
      // It is a string log or it is not defined but not ignored either
      addProperty<std::string>(infows, title, strvalue);
    }
  }

  return infows;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief LoadSpiceAscii::setupRunStartTime
 * @param runinfows
 * @param datetimeprop
 */
void LoadSpiceAscii::setupRunStartTime(
    API::MatrixWorkspace_sptr runinfows,
    const std::vector<std::string> &datetimeprop) {
  // Check if no need to process run start time
  if (datetimeprop.size() == 0) {
    g_log.information("User chooses not to set up run start date and time.");
    return;
  }

  // Parse property vector
  if (datetimeprop.size() != 4) {
    g_log.warning() << "Run start date and time property must contain 4 "
                       "strings.  User only specifies " << datetimeprop.size()
                    << ".  Set up failed."
                    << "\n";
    return;
  }

  // Parse
  std::string datelogname = datetimeprop[0];
  std::string timelogname = datetimeprop[2];
  if (!(runinfows->run().hasProperty(datelogname) &&
        runinfows->run().hasProperty(timelogname))) {
    std::stringstream errss;
    errss << "Unable to locate user specified date and time sample logs "
          << datelogname << " and " << timelogname << ".";
    throw std::runtime_error(errss.str());
  }

  const std::string &rawdatestring =
      runinfows->run().getProperty(datelogname)->value();
  const std::string &dateformat = datetimeprop[1];
  std::string mtddatestring = processDateString(rawdatestring, dateformat);

  const std::string &rawtimestring =
      runinfows->run().getProperty(timelogname)->value();
  const std::string &timeformat = datetimeprop[3];
  std::string mtdtimestring = processTimeString(rawtimestring, timeformat);

  std::string mtddatetimestr = mtddatestring + "T" + mtdtimestring;

  // Set up property
  DateAndTime runstart(mtddatetimestr);
  addProperty<std::string>(runinfows, "run_start", runstart.toISO8601String());
}

//----------------------------------------------------------------------------------------------
/**
 * @brief LoadSpiceAscii::processDateString
 * @param rawdate
 * @param dateformat
 * @return
 */
std::string LoadSpiceAscii::processDateString(const std::string &rawdate,
                                              const std::string &dateformat) {
  // Identify splitter
  std::string splitter("");
  if (dateformat.find('/') != std::string::npos)
    splitter += "/";
  else if (dateformat.find('-') != std::string::npos)
    splitter += "-";
  else if (dateformat.find('.') != std::string::npos)
    splitter += ".";
  else
    throw std::runtime_error("Input date format does not contain any of / - "
                             "or '.'.  Format unsupported.");

  // Split
  std::vector<std::string> dateterms;
  std::vector<std::string> formatterms;
  boost::split(dateterms, rawdate, boost::is_any_of(splitter));
  boost::split(formatterms, dateformat, boost::is_any_of(splitter));

  if (dateterms.size() != formatterms.size() || dateterms.size() != 3)
    throw std::runtime_error("Unsupported date string and format");
  std::string year("");
  std::string month("");
  std::string day("");
  for (size_t i = 0; i < 3; ++i) {
    if (formatterms[i].find("Y") != std::string::npos)
      year = dateterms[i];
    else if (formatterms[i].find("M") != std::string::npos) {
      month = dateterms[i];
      if (month.size() == 1)
        month = "0" + month;
    } else {
      day = dateterms[i];
      if (day.size() == 1)
        day = "0" + day;
    }
  }

  std::string formatdate = year + "-" + month + "-" + day;

  return formatdate;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief LoadSpiceAscii::processTimeString
 * @param rawtime
 * @param timeformat
 * @return
 */
std::string LoadSpiceAscii::processTimeString(const std::string &rawtime,
                                              const std::string &timeformat) {
  // Process time format to find out it is 12 hour or 24 hour format
  std::string timeformatcpy(timeformat);
  boost::trim(timeformatcpy);

  int format;
  if (timeformatcpy.find(' ') == std::string::npos)
    format = 24;
  else
    format = 12;

  // Process
  std::string mtdtime;
  if (format == 24)
    mtdtime = rawtime;
  else {
    std::vector<std::string> terms;
    boost::split(terms, rawtime, boost::is_any_of(" "));
    bool pm = false;
    if (terms[1].compare("PM") == 0)
      pm = true;

    std::vector<std::string> terms2;
    boost::split(terms2, terms[0], boost::is_any_of(":"));
    int hour = atoi(terms[0].c_str());
    if (hour < 12 && pm)
      hour += 12;

    std::stringstream hourss;
    hourss << hour;
    std::string hourstr = hourss.str();
    if (hourstr.size() == 1)
      hourstr = "0" + hourstr;
    std::string minstr = terms2[1];
    if (minstr.size() == 1)
      minstr = "0" + minstr;
    std::string secstr = terms2[2];
    if (secstr.size() == 1)
      secstr = "0" + secstr;

    mtdtime = hourstr + ":" + minstr + ":" + secstr;
  }

  return mtdtime;
}

//----------------------------------------------------------------------------------------------
/** Add property to workspace
 * @brief LoadSpiceAscii::addProperty
 * @param ws
 * @param pname
 * @param pvalue
 */
template <typename T>
void LoadSpiceAscii::addProperty(API::MatrixWorkspace_sptr ws,
                                 const std::string &pname, T pvalue) {
  ws->mutableRun().addLogData(new PropertyWithValue<T>(pname, pvalue));

  return;
}

} // namespace DataHandling
} // namespace Mantid
