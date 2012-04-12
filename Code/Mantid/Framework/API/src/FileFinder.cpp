//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IArchiveSearch.h"
#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Glob.h"

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <Poco/RegularExpression.h>
#include <boost/lexical_cast.hpp>

#include <cctype>
#include <algorithm>

#include <boost/algorithm/string.hpp>

namespace
{
  /**
   * Unary predicate for use with remove_if.  Checks for the existance of
   * a "*" wild card in the file extension string passed to it.
   *
   * @param ext :: the extension to check.
   *
   * @returns true if extension contains a "*", else false.
   */
  bool containsWildCard(const std::string & ext)
  {
    if (std::string::npos != ext.find("*"))
      return true;
    return false;
  }
}

namespace Mantid
{
  namespace API
  {
    using std::string;

    // this allowed string could be made into an array of allowed, currently used only by the ISIS SANS group
    const std::string FileFinderImpl::ALLOWED_SUFFIX = "-add";
    //----------------------------------------------------------------------
    // Public member functions
    //----------------------------------------------------------------------
    /**
     * Default constructor
     */
    FileFinderImpl::FileFinderImpl() : g_log(Mantid::Kernel::Logger::get("FileFinderImpl"))
    {
      // Make sure plugins are loaded
      std::string libpath = Kernel::ConfigService::Instance().getString("plugins.directory");
      if (!libpath.empty())
      {
        Kernel::LibraryManager::Instance().OpenAllLibraries(libpath);
      }

      // determine from Mantid property how sensitive Mantid should be
      std::string casesensitive = Mantid::Kernel::ConfigService::Instance().getString("filefinder.casesensitive");
      if ( boost::iequals("Off",casesensitive) )
        globOption = Poco::Glob::GLOB_CASELESS;
      else
        globOption = Poco::Glob::GLOB_DEFAULT;
    }


    /**
     * Option to set if file finder should be case sensitive
     * @param cs :: If true then set to case sensitive
     */
    void FileFinderImpl::setCaseSensitive(const bool cs) 
    {
      if ( cs )
        globOption = Poco::Glob::GLOB_DEFAULT;
      else
        globOption = Poco::Glob::GLOB_CASELESS;
    }

    /**
     * Return the full path to the file given its name
     * @param fName :: A full file name (without path) including extension
     * @return The full path if the file exists and can be found in one of the search locations
     *  or an empty string otherwise.
     */
    std::string FileFinderImpl::getFullPath(const std::string& fName) const
    {
      g_log.debug() << "getFullPath(" << fName << ")\n";
      // If this is already a full path, nothing to do
      if (Poco::Path(fName).isAbsolute())
        return fName;

      // First try the path relative to the current directory. Can throw in some circumstances with extensions that have wild cards
      try
      {
        Poco::File fullPath(Poco::Path().resolve(fName));
        if (fullPath.exists())
          return fullPath.path();
      }
      catch (std::exception&)
      {
      }

      const std::vector<std::string>& searchPaths =
          Kernel::ConfigService::Instance().getDataSearchDirs();
      std::vector<std::string>::const_iterator it = searchPaths.begin();
      for (; it != searchPaths.end(); ++it)
      {
// On windows globbing is note working properly with network drives 
// for example a network drive containing a $ 
// For this reason, and since windows is case insensitive anyway
// a special case is made for windows
#ifdef _WIN32
          if (fName.find("*") != std::string::npos)
          {
#endif
          Poco::Path path(*it, fName);
          Poco::Path pathPattern(path);
          std::set < std::string > files;
          Kernel::Glob::glob(pathPattern, files, globOption);
          if (!files.empty())
          {
            return *files.begin();
          }
#ifdef _WIN32
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
#endif
      }
      return "";
    }

    /** Run numbers can be followed by an allowed string. Check if there is
     *  one, remove it from the name and return the string, else return empty
     *  @param userString run number that may have a suffix
     *  @return the suffix, if there was one
     */
    std::string FileFinderImpl::extractAllowedSuffix(std::string & userString) const
    {
      if (userString.find(ALLOWED_SUFFIX) == std::string::npos)
      {
        //short cut processing as normally there is no suffix
        return "";
      }

      // ignore any file extension in checking if a suffix is present
      Poco::Path entry(userString);
      std::string noExt(entry.getBaseName());
      const size_t repNumChars = ALLOWED_SUFFIX.size();
      if (noExt.find(ALLOWED_SUFFIX) == noExt.size() - repNumChars)
      {
        userString.replace(userString.size() - repNumChars, repNumChars, "");
        return ALLOWED_SUFFIX;
      }
      return "";
    }

    /**
     * Return the name of the facility as determined from the hint.
     *
     * @param hint :: The name hint.
     * @return This will return the default facility if it cannot be determined.
     */
    const Kernel::FacilityInfo FileFinderImpl::getFacility(const string& hint) const
    {
      if ((!hint.empty()) && (!isdigit(hint[0])))
      {
        string instrName(hint);
        Poco::Path path(instrName);
        instrName = path.getFileName();
        if ((instrName.find("PG3") == 0) || (instrName.find("pg3") == 0))
        {
          instrName = "PG3";
        }
        else
        {
          // go forwards looking for the run number to start
          {
            string::const_iterator it = std::find_if(instrName.begin(), instrName.end(), std::ptr_fun(isdigit));
            std::string::size_type nChars = std::distance( static_cast<string::const_iterator>(instrName.begin()), it);
            instrName = instrName.substr(0, nChars);
          }

          // go backwards looking for the instrument name to end - gets around delimiters
          if (!instrName.empty())
          {
            string::const_reverse_iterator it = std::find_if(instrName.rbegin(), instrName.rend(),
                                                             std::ptr_fun(isalpha));
            string::size_type nChars = std::distance(it,
                                        static_cast<string::const_reverse_iterator>(instrName.rend()));
            instrName = instrName.substr(0, nChars);
          }
        }
        try {
          const Kernel::InstrumentInfo instrument = Kernel::ConfigService::Instance().getInstrument(instrName);
          return instrument.facility();
        } catch (Kernel::Exception::NotFoundError &e) {
          g_log.debug() << e.what() << "\n";
        }
      }
      return Kernel::ConfigService::Instance().getFacility();
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
        instrPart = Kernel::ConfigService::Instance().getInstrument().shortName();
        runPart = hint;
      }
      else
      {
        /// Find the last non-digit as the instrument name can contain numbers
        std::string::const_reverse_iterator it = std::find_if(hint.rbegin(), hint.rend(),
            std::not1(std::ptr_fun(isdigit)));
        // No non-digit or all non-digits
        if (it == hint.rend() || it == hint.rbegin())
        {
          throw std::invalid_argument("Malformed hint to FileFinderImpl::makeFileName: " + hint);
        }
        std::string::size_type nChars = std::distance(it, hint.rend());

        // PG3 is a special case (name ends in a number)- don't trust them
        if ((hint.find("PG3") == 0) || (hint.find("pg3") == 0)) {
          instrPart = "PG3";
          if (nChars < 3)
            nChars++;
        }
        else {
          instrPart = hint.substr(0, nChars);
        }
        runPart = hint.substr(nChars);
      }

      Kernel::InstrumentInfo instr = Kernel::ConfigService::Instance().getInstrument(instrPart);
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
     * @param facility :: The current facility object
     * @return The file name
     * @throw NotFoundError if a required default is not set
     * @throw std::invalid_argument if the argument is malformed or run number is too long
     */
    std::string FileFinderImpl::makeFileName(const std::string& hint, const Kernel::FacilityInfo& facility) const
    {
      if (hint.empty())
        return "";

      std::string filename(hint);
      const std::string suffix = extractAllowedSuffix(filename);

      std::pair < std::string, std::string > p = toInstrumentAndNumber(filename);
      std::string delimiter = facility.delimiter();

      filename = p.first;
      if (!delimiter.empty())
      {
        filename += delimiter;
      }
      filename += p.second;

      if (!suffix.empty())
      {
        filename += suffix;
      }

      return filename;
    }

    /**
     * Find the file given a hint. If the name contains a dot(.) then it is assumed that it is already a file stem
     * otherwise calls makeFileName internally.
     * @param hint :: The name hint, format: [INSTR]1234[.ext]
     * @param exts :: Optional list of allowed extensions. Only those extensions found in both
     *  facilities extension list and exts will be used in the search. If an extension is given in hint 
     *  this argument is ignored.
     * @return The full path to the file or empty string if not found
     */
    std::string FileFinderImpl::findRun(const std::string& hint, const std::set<std::string> *exts) const
    {
      g_log.debug() << "set findRun(\'" << hint << "\', exts[" << exts->size() << "])\n";
      if (hint.empty())
        return "";
      std::vector<std::string> exts_v;
      if (exts != NULL && exts->size() > 0)
        exts_v.assign(exts->begin(), exts->end());

      return this->findRun(hint, exts_v);
    }

    std::string FileFinderImpl::findRun(const std::string& hint,const std::vector<std::string> &exts)const
    {
      g_log.debug() << "vector findRun(\'" << hint << "\', exts[" << exts.size() << "])\n";
      if (hint.empty())
        return "";

      // if it looks like a full filename just do a quick search for it
      Poco::Path hintPath(hint);
      if (!hintPath.getExtension().empty())
      {
        // check in normal search locations
        std::string path = getFullPath(hint);
        g_log.debug() << "path returned from getFullPath = " << path << '\n';
        try
        {
          if (!path.empty() && Poco::File(path).exists())
          {
            g_log.debug() << "path is not empty and exists" << '\n';
            return path;
          }
        }
        catch(std::exception& e)
        {
          g_log.error() << "Cannot open file " << path << ": " << e.what() << '\n';
          return "";
        }
      }

      // so many things depend on the facility just get it now
      const Kernel::FacilityInfo facility = this->getFacility(hint);
      // initialize the archive searcher
      std::vector<IArchiveSearch_sptr> archs;
      { // hide in a local namespace so things fall out of scope
        std::string archiveOpt = Kernel::ConfigService::Instance().getString("datasearch.searcharchive");
        std::transform(archiveOpt.begin(), archiveOpt.end(), archiveOpt.begin(), tolower);
        if (!archiveOpt.empty() && archiveOpt != "off" && !facility.archiveSearch().empty())
        {
          g_log.debug() << "archive search count..." << facility.archiveSearch().size() << "\n";
          std::vector<std::string>::const_iterator it = facility.archiveSearch().begin();
          for (; it != facility.archiveSearch().end(); it++)
          {
            g_log.debug() << "Archives to be searched..." << *it << "\n";
            archs.push_back(ArchiveSearchFactory::Instance().create(*it));
          }
        }
      }


      // ask the archive search for help
      if (!hintPath.getExtension().empty())
      {
        g_log.debug() << "Starting hintPath.getExtension()..." << hintPath.getExtension() << "\n";
        if (archs.size() != 0 )
        {
          try
          {
            std::string path = getArchivePath(archs, hint);
            if (!path.empty())
            {
              Poco::File file(path);
              if (file.exists())
              {
                return file.path();
              }
            }
          }
          catch(...)
          {
            g_log.error() << "Archive search could not find '" << hint << "'\n";
          }
        }
      }

      // Do we need to try and form a filename from our preset rules
      std::string filename(hint);
      std::string extension;
      if (hintPath.depth() == 0)
      {
        std::size_t i = filename.find_last_of('.');
        if (i != std::string::npos)
        {
          extension = filename.substr(i);
          filename.erase(i);
        }
        try
        {
          filename = makeFileName(filename, facility);
        }
        catch(std::invalid_argument&)
        {
          g_log.error() << "Could not find file '" << filename << "'\n";
        }
      }

      // work through the extensions
      const std::vector<std::string> facility_extensions = facility.extensions();
      // select allowed extensions
      std::vector < std::string > extensions;
      if (!extension.empty())
      {
        extensions.push_back(extension);
      }
      else if (!exts.empty())
      {
        extensions.insert(extensions.end(), exts.begin(), exts.end());
        // find intersection of facility_extensions and exts, preserving the order of facility_extensions
        std::vector<std::string>::const_iterator it = facility_extensions.begin();
        for (; it != facility_extensions.end(); ++it)
        {
          if (std::find(exts.begin(), exts.end(), *it) == exts.end())
          {
            extensions.push_back(*it);
          }
        }
      }
      else
      {
        extensions.assign(facility_extensions.begin(), facility_extensions.end());
      }

      // Look first at the original filename then for case variations. This is important
      // on platforms where file names ARE case sensitive.
      std::vector<std::string> filenames(3,filename);
      std::transform(filename.begin(),filename.end(),filenames[1].begin(),toupper);
      std::transform(filename.begin(),filename.end(),filenames[2].begin(),tolower);

      // Remove wild cards.
      extensions.erase(std::remove_if( // "Erase-remove" idiom.
          extensions.begin(), extensions.end(), 
          containsWildCard),
        extensions.end());

      std::vector<std::string>::const_iterator ext = extensions.begin();
      for (; ext != extensions.end(); ++ext)
      {
        for(size_t i = 0; i < filenames.size(); ++i)
        {
          std::string path = getFullPath(filenames[i] + *ext);
          if (!path.empty())
          {
            //Check file actually exists.
            try
            {
              if (Poco::File(path).exists() )
                return path;
              else
                return("");
            }
            catch(...)
            {
              return("");
            }
          }
        }
      }

      // Search the archive of the default facility
      if (archs.size() != 0 )
      {
        g_log.debug() << "Search the archive of the default facility" << "\n";
        std::string path;
        std::vector<std::string>::const_iterator ext = extensions.begin();
        for (; ext != extensions.end(); ++ext)
        {
          g_log.debug() << " filenames.size()=" <<  filenames.size() << "\n";
          for(size_t i = 0; i < filenames.size(); ++i)
          {
            try
            {
              path = getArchivePath(archs, filenames[i] + *ext);
            }
            catch(...)
            {
              return "";
            }
            if( path.empty() ) return "";

            Poco::Path pathPattern(path);
            if (ext->find("*") != std::string::npos)
            {
              continue;
              // FIXME: Does this need to be before continue?
              //std::set < std::string > files;
              //Kernel::Glob::glob(pathPattern, files, globOption);
            }
            else
            {
              Poco::File file(pathPattern);
              if (file.exists())
              {
                return file.path();
              }
            }
          } // i
        }  // ext
      } // archs

      return "";
    }

    std::string FileFinderImpl::findFullPath(const std::string& hint, const std::set<std::string> *exts) const
    {
      g_log.debug() << "set findFullPath(\'" << hint << "\', exts[" << exts->size() << "])\n";
      if (hint.empty())
        return "";
      std::vector<std::string> exts_v;
      if (exts != NULL && exts->size() > 0)
        exts_v.assign(exts->begin(), exts->end());

      return this->findFullPath(hint, exts_v);
    }

    std::string FileFinderImpl::findFullPath(const std::string& hint,const std::vector<std::string> &exts)const
    {
      g_log.debug() << "vector findFullPath(\'" << hint << "\', exts[" << exts.size() << "])\n";

      //if partial filename or run number is not supplied, return here
      if (hint.empty())
        return "";

      // if it looks like a full filename just do a quick search for it
      Poco::Path hintPath(hint);
      if (!hintPath.getExtension().empty())
      {
        // check in normal search locations
        g_log.debug() << "hintPath is not empty, check in normal search locations" << "\n";
        std::string path = getFullPath(hint);
        if (!path.empty())
        {
          g_log.information() << "found path = " << path << '\n';
          return path;
        } else {
          g_log.notice() << "Unable to find files via direcotry search with the filename that looks like a full filename" << "\n";
        }
      }

      // get facility from the FacilityInfo
      const Kernel::FacilityInfo facility = this->getFacility(hint);

      // initialize the archive searcher
      std::vector<IArchiveSearch_sptr> archs;
      { // hide in a local namespace so things fall out of scope
        std::string archiveOpt = Kernel::ConfigService::Instance().getString("datasearch.searcharchive");
        std::transform(archiveOpt.begin(), archiveOpt.end(), archiveOpt.begin(), tolower);
        if (!archiveOpt.empty() && archiveOpt != "off" && !facility.archiveSearch().empty())
        {
          std::vector<std::string>::const_iterator it = facility.archiveSearch().begin();
          for (; it != facility.archiveSearch().end(); it++)
          {
            g_log.debug() << "get archive search for the facility..." << *it << "\n";
            archs.push_back(ArchiveSearchFactory::Instance().create(*it));
          }
        }
      }


      // ask the archive search for help
      if (!hintPath.getExtension().empty())
      {
        g_log.debug() << "hintPath is not empty, try archive search" << "\n";
        if (archs.size() != 0 )
        {
          std::string path = getArchivePath(archs, hint);
          if (!path.empty())
          {
            g_log.information() << "found path = " << path << '\n';
            return path;
          } else {
            g_log.notice() << "Unable to find files via archive search with the filename that looks like a full filename" << "\n";
          }
          return path;
        }
      }


      // Do we need to try and form a filename from our preset rules
      std::string filename(hint);
      std::string extension;
      if (hintPath.depth() == 0)
      {
        std::size_t i = filename.find_last_of('.');
        if (i != std::string::npos)
        {
          extension = filename.substr(i);
          filename.erase(i);
        }
        try
        {
          filename = makeFileName(filename, facility);
        }
        catch(std::invalid_argument&)
        {
          g_log.error() << "Could not find file '" << filename << "'\n";
        }
      }

      // Look first at the original filename then for case variations. This is important
      // on platforms where file names ARE case sensitive.
      std::set<std::string> filenames;
      filenames.insert(filename);
      std::transform(filename.begin(),filename.end(),filename.begin(),toupper);
      filenames.insert(filename);
      std::transform(filename.begin(),filename.end(),filename.begin(),tolower);
      filenames.insert(filename);

      // work through the extensions
      // try the extension that comes with the filename
      if (!extension.empty())
      {
        g_log.debug() << "Attempt to find files with the extension that comes with the filename" <<  extension << "\n";
        std::string path = getPath(archs, filenames, std::vector<std::string>(1, extension));
        if (!path.empty())
        {
          g_log.information() << "found path = " << path << '\n';
          return path;
        } else {
          g_log.notice() << "Unable to find files with extensions that comes with the filename" << "\n";
        }
      }

      // try the extension that are supplied by user
      if (!exts.empty())
      {
        g_log.debug() << "Attempt to find files with extensions that are supplied by users or algorithms, first extension = " <<  *(exts.begin()) << "\n";
        std::string path = getPath(archs, filenames, exts);
        if (!path.empty())
        {
          g_log.information() << "found path = " << path << '\n';
          return path;
        } else {
          g_log.notice() << "Unable to find files with extensions that are supplied by users or algorithms" << "\n";
        }
      }

      // work through the extensions
      const std::vector<std::string> facility_extensions = facility.extensions();
      // select allowed extensions
      std::vector < std::string > extensions;

      g_log.debug() << "Add facility extensions defined in the Facility.xml file" << "\n";
      extensions.assign(facility_extensions.begin(), facility_extensions.end());
      std::string path = getPath(archs, filenames, extensions);
      if (!path.empty())
      {
        g_log.information() << "found path = " << path << '\n';
        return path;
      } else {
        g_log.notice() << "Unable to find files with extensions that are defined in the Facility.xml file" << "\n";
      }

      g_log.notice() << "Unable to find files" << "\n";

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
      g_log.debug() << "findRuns hint = " << hint << "\n";
      std::vector < std::string > res;
      Poco::StringTokenizer hints(hint, ",",
          Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
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
        if ((*h).find(ALLOWED_SUFFIX) != std::string::npos)
        {
          fileSuspected = true;
        }

        Poco::StringTokenizer range(*h, "-",
            Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
        if ((range.count() > 2) && (!fileSuspected))
        {
          throw std::invalid_argument("Malformed range of runs: " + *h);
        }
        else if ((range.count() == 2) && (!fileSuspected))
        {
          std::pair < std::string, std::string > p1 = toInstrumentAndNumber(range[0]);
          std::string run = p1.second;
          size_t nZero = run.size(); // zero padding
          if (range[1].size() > nZero)
          {
            throw std::invalid_argument("Malformed range of runs: " + *h
                + ". The end of string value is longer than the instrument's zero padding");
          }
          int runNumber = boost::lexical_cast<int>(run);
          std::string runEnd = run;
          // Adds zero padding to end of range.
          runEnd.replace(runEnd.end() - range[1].size(), runEnd.end(), range[1]);

          // Throw if runEnd contains something else other than a digit.
          Poco::RegularExpression digits("[0-9]+");
          if (!digits.match(runEnd))
            throw std::invalid_argument("Malformed range of runs: Part of the run has a non-digit character in it.");

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
            std::string path = findFullPath(p1.first + run);
            //std::string path = findRun(p1.first + run);
            if (!path.empty())
            {
              res.push_back(path);
            }
          }

        }
        else
        {
          std::string path = findFullPath(*h);
          //std::string path = findRun(*h);
          if (!path.empty())
          {
            res.push_back(path);
          }
        }
      }

      return res;
    }

    /**
     * Return the path to the file found in archive
     * @param archs :: A full file name (without path) including extension
     * @param fName :: A full file name (without path) including extension
     * @return The full path if the file exists and can be found in one of the search locations
     *  or an empty string otherwise.
     */
    std::string FileFinderImpl::getArchivePath(const std::vector<IArchiveSearch_sptr>& archs, const std::string& fName) const
    {
      g_log.debug() << "getArchivePath(" << fName << ")\n";
      std::string path = "";
      std::vector<IArchiveSearch_sptr>::const_iterator it = archs.begin();
      for (; it != archs.end(); it++)
      {
        path = (*it)->getPath(fName);
        if (!path.empty())
        {
          return path;
        }
      }
      return path;
    }

    /**
     * Return the full path to the file given its name
     * @param fName :: A vector of full file name (without path) and a vector of extensions
     * @return The full path if the file exists and can be found in one of the search locations
     *  or an empty string otherwise.
     */
    std::string FileFinderImpl::getPath(const std::vector<IArchiveSearch_sptr>& archs, const std::set<std::string>& filenames, const std::vector<std::string>& exts) const
    {
      std::string path;

      std::vector<std::string> extensions;
      extensions.assign(exts.begin(),exts.end());

      // Remove wild cards.
      extensions.erase(std::remove_if( // "Erase-remove" idiom.
          extensions.begin(), extensions.end(),
          containsWildCard),
        extensions.end());

      std::vector<std::string>::const_iterator ext = extensions.begin();

      for (; ext != extensions.end(); ++ext)
      {
        std::set<std::string>::const_iterator it = filenames.begin();
        for(; it!=filenames.end(); it++)
        {
          path = getFullPath(*it + *ext);
          try
          {
            if (!path.empty() && Poco::File(path).exists())
            {
              g_log.debug() << "path returned from getFullPath() = " << path << '\n';
              return path;
            }
          }
          catch(std::exception& e)
          {
            g_log.error() << "Cannot open file " << path << ": " << e.what() << '\n';
            return "";
          }
        }
      }

      // Search the archive
      if (archs.size() != 0 )
      {
        g_log.debug() << "Search the archive of the default facility" << "\n";
        std::string path = "";
        std::vector<std::string>::const_iterator ext = extensions.begin();
        for (; ext != extensions.end(); ++ext)
        {
          std::set<std::string>::const_iterator it = filenames.begin();
          for(; it!=filenames.end(); it++)
          {
            path = getArchivePath(archs, *it + *ext);
            try
            {
              if (!path.empty() && Poco::File(path).exists())
              {
                return path;
              }
            }
            catch(std::exception& e)
            {
              g_log.error() << "Cannot open file " << path << ": " << e.what() << '\n';
              return "";
            }
          } // it
        }  // ext
      } // archs

      return "";
    }

  }// API
}// Mantid

