//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IArchiveSearch.h"
#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Glob.h"

#include "Poco/Path.h"
#include "Poco/File.h"
#include "Poco/StringTokenizer.h"
#include <boost/lexical_cast.hpp>

#include <cctype>
#include <algorithm>

namespace Mantid
{
namespace API
{
//----------------------------------------------------------------------
// Public member functions
//----------------------------------------------------------------------
/**
 * Default constructor
 */
FileFinderImpl::FileFinderImpl()
{
  // Make sure plugins are loaded
  std::string libpath = Kernel::ConfigService::Instance().getString("plugins.directory");
  if (!libpath.empty())
  {
    Kernel::LibraryManager::Instance().OpenAllLibraries(libpath);
  }
}

/**
 * Return the full path to the file given its name
 * @param fName :: A full file name (without path) including extension
 * @return The full path if the file exists and can be found in one of the search locations
 *  or an empty string otherwise.
 */
std::string FileFinderImpl::getFullPath(const std::string& fName) const
{
  // If this is already a full path, nothing to do
  if (Poco::Path(fName).isAbsolute())
    return fName;

  const std::vector<std::string>& searchPaths = Kernel::ConfigService::Instance().getDataSearchDirs();
  std::vector<std::string>::const_iterator it = searchPaths.begin();
  for (; it != searchPaths.end(); ++it)
  {
    if (fName.find("*") != std::string::npos)
    {
      Poco::Path path(*it, fName);
      Poco::Path pathPattern(path);
      std::set<std::string> files;
      Kernel::Glob::glob(pathPattern, files);
      if (!files.empty())
      {
        return *files.begin();
      }
    }
    else
    {
      Poco::Path path(*it, fName);
      Poco::File file(path);
      if (file.exists())
      {
        return path.toString();
      }
    }
  }
  return "";
}

/**
 * Extracts the instrument name and run number from a hint
 * @param hint :: The name hint
 * @return A pair of instrument name and run number
 */
std::pair<std::string, std::string> FileFinderImpl::toInstrumentAndNumber(const std::string& hint) const
{
  std::string instrPart;
  std::string runPart;

  if (isdigit(hint[0]))
  {
    instrPart = Kernel::ConfigService::Instance().Facility().Instrument().shortName();
    runPart = hint;
  }
  else
  {
    /// Find the last non-digit as the instrument name can contain numbers
    std::string::const_reverse_iterator it = std::find_if(hint.rbegin(), hint.rend(), std::not1(
        std::ptr_fun(isdigit)));
    // No non-digit or all non-digits
    if (it == hint.rend() || it == hint.rbegin())
    {
      throw std::invalid_argument("Malformed hint to FileFinderImpl::makeFileName: " + hint);
    }
    std::string::size_type nChars = std::distance(it, hint.rend());
    instrPart = hint.substr(0, nChars);
    runPart = hint.substr(nChars);
  }

  Kernel::InstrumentInfo instr = Kernel::ConfigService::Instance().Facility().Instrument(instrPart);
  size_t nZero = instr.zeroPadding();
  // remove any leading zeros in case there are too many of them
  std::string::size_type i = runPart.find_first_not_of('0');
  runPart.erase(0, i);
  while (runPart.size() < nZero)
    runPart.insert(0, "0");
  if (runPart.size() > nZero && nZero != 0)
  {
    throw std::invalid_argument("Run number does not match instrument's zero padding");
  }

  instrPart = instr.shortName();

  return std::make_pair(instrPart, runPart);

}

/**
 * Make a data file name (without extension) from a hint. The hint can be either a run number or
 * a run number prefixed with an instrument name/short name. If the instrument
 * name is absent the default one is used.
 * @param hint :: The name hint
 * @return The file name
 * @throw NotFoundError if a required default is not set
 * @throw std::invalid_argument if the argument is malformed or run number is too long
 */
std::string FileFinderImpl::makeFileName(const std::string& hint) const
{
  if (hint.empty())
    return "";

  std::pair<std::string, std::string> p = toInstrumentAndNumber(hint);

  Kernel::InstrumentInfo instr = Kernel::ConfigService::Instance().Facility().Instrument(p.first);
  std::string delimiter = instr.delimiter();

  if (delimiter.empty())
  {
    return p.first + p.second;
  }
  else
  {
    return p.first + delimiter + p.second;
  }

}

/**
 * Find the file given a hint. If the name contains a dot(.) then it is assumed that it is already a file stem
 * otherwise calls makeFileName internally.
 * @param hint :: The name hint
 * @param exts :: Optional list of allowed extensions. Only those extensions found in both
 *  facilities extension list and exts will be used in the search
 * @return The full path to the file or empty string if not found
 */
std::string FileFinderImpl::findRun(const std::string& hint, const std::set<std::string> *exts) const
{
  if (hint.find(".") != std::string::npos)
  {
    return getFullPath(hint);
  }
  std::string fName = makeFileName(hint);
  const std::vector<std::string> facility_extensions =
      Kernel::ConfigService::Instance().Facility().extensions();
  // select allowed extensions
  std::vector<std::string> extensions;
  if (exts != NULL)
  {
    // find intersection of facility_extensions and exts, preserving the order of facility_extensions
    std::vector<std::string>::const_iterator it = facility_extensions.begin();
    for (; it != facility_extensions.end(); ++it)
    {
      if (exts->find(*it) != exts->end())
      {
        extensions.push_back(*it);
      }
    }
  }
  else
  {
    extensions.assign(facility_extensions.begin(), facility_extensions.end());
  }
  std::vector<std::string>::const_iterator ext = extensions.begin();
  for (; ext != extensions.end(); ++ext)
  {
    std::string path = getFullPath(fName + *ext);
    if (!path.empty())
      return path;
  }

  // Search the archive of the default facility
  std::string archiveOpt = Kernel::ConfigService::Instance().getString("datasearch.searcharchive");
  std::transform(archiveOpt.begin(), archiveOpt.end(), archiveOpt.begin(), tolower);
  if (!archiveOpt.empty() && archiveOpt != "off"
      && !Kernel::ConfigService::Instance().Facility().archiveSearch().empty())
  {
    IArchiveSearch_sptr arch = ArchiveSearchFactory::Instance().create(
        *Kernel::ConfigService::Instance().Facility().archiveSearch().begin());
    if (arch)
    {
      std::string path = arch->getPath(fName);
      if (!path.empty())
      {
        std::vector<std::string>::const_iterator ext = extensions.begin();
        for (; ext != extensions.end(); ++ext)
        {
          Poco::Path pathPattern(path + *ext);
          if (ext->find("*") != std::string::npos)
          {
            continue;
            std::set<std::string> files;
            Kernel::Glob::glob(pathPattern, files);
            std::cerr << "Searching for:" << pathPattern.toString() << '\n';
            std::cerr << "Found:" << files.size() << '\n';
          }
          else
          {
            Poco::File file(pathPattern);
            if (file.exists())
            {
              return file.path();
            }
          }
        }
      }
    }
  }
  return "";
}

/**
 * Find a list of files file given a hint. Calls findRun internally.
 * @param hint :: Comma separated list of hints to findRun method.
 *  Can also include ranges of runs, e.g. 123-135 or equivalently 123-35.
 *  Only the beginning of a range can contain an instrument name.
 * @return A vector of full paths or empty vector
 * @throw std::invalid_argument if the argument is malformed
 */
std::vector<std::string> FileFinderImpl::findRuns(const std::string& hint) const
{
  std::vector<std::string> res;
  Poco::StringTokenizer hints(hint, ",", Poco::StringTokenizer::TOK_TRIM
      | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
  Poco::StringTokenizer::Iterator h = hints.begin();

  for (; h != hints.end(); ++h)
  {
    // Quick check for a filename
    bool fileSuspected = false;
    // Assume if the hint contains either a "/" or "\" it is a filename..
    if ((*h).find("\\") != std::string::npos)
    {
      fileSuspected = true;
    }
    if ((*h).find("/") != std::string::npos)
    {
      fileSuspected = true;
    }

    Poco::StringTokenizer range(*h, "-", Poco::StringTokenizer::TOK_TRIM
        | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
    if ((range.count() > 2) && (!fileSuspected))
    {
      throw std::invalid_argument("Malformed range of runs: " + *h);
    }
    else if ((range.count() == 2) && (!fileSuspected))
    {
      std::pair<std::string, std::string> p1 = toInstrumentAndNumber(range[0]);
      std::string run = p1.second;
      size_t nZero = run.size(); // zero padding
      if (range[1].size() > nZero)
      {
        ("Malformed range of runs: " + *h
            + ". The end of string value is longer than the instrument's zero padding");
      }
      int runNumber = boost::lexical_cast<int>(run);
      std::string runEnd = run;
      runEnd.replace(runEnd.end() - range[1].size(), runEnd.end(), range[1]);
      int runEndNumber = boost::lexical_cast<int>(runEnd);
      if (runEndNumber < runNumber)
      {
        throw std::invalid_argument("Malformed range of runs: " + *h);
      }
      for (int irun = runNumber; irun <= runEndNumber; ++irun)
      {
        run = boost::lexical_cast<std::string>(irun);
        while (run.size() < nZero)
          run.insert(0, "0");
        std::string path = findRun(p1.first + run);
        if (!path.empty())
        {
          res.push_back(path);
        }
      }

    }
    else
    {
      std::string path = findRun(*h);
      if (!path.empty())
      {
        res.push_back(path);
      }
    }
  }

  return res;
}

}// API

}// Mantid

