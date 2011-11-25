//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/MultiFileNameParser.h"

#include <algorithm>
#include <numeric>
#include <exception>
#include <sstream>

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <Poco/Path.h>

namespace Mantid
{
  namespace Kernel
  {
    ////////////////////////////////////////////////////////////////////////////////////////
    // STATIC CONSTANTS - regexs used to parse multi runs, built up and nested step by step.
    ////////////////////////////////////////////////////////////////////////////////////////

    const std::string MultiFileNameParser::INST = "([A-Za-z]+|PG3|pg3)";

    const std::string MultiFileNameParser::UNDERSCORE = "(_{0,1})";
    const std::string MultiFileNameParser::SPACE      = "(\\s*)";

    const std::string MultiFileNameParser::COMMA = "(" + SPACE + "," + SPACE + ")";
    const std::string MultiFileNameParser::PLUS  = "(" + SPACE + "\\+" + SPACE + ")";
    const std::string MultiFileNameParser::MINUS = "(" + SPACE + "\\-" + SPACE + ")";
    const std::string MultiFileNameParser::COLON = "(" + SPACE + ":" + SPACE + ")";

    const std::string MultiFileNameParser::SINGLE         = "([0-9]+)";
    const std::string MultiFileNameParser::RANGE          = "(" + SINGLE + COLON + SINGLE + ")";
    const std::string MultiFileNameParser::STEP_RANGE     = "(" + SINGLE + COLON + SINGLE + COLON + SINGLE + ")";
    const std::string MultiFileNameParser::ADD_LIST       = "(" + SINGLE + "(" + PLUS + SINGLE + ")+" + ")";
    const std::string MultiFileNameParser::ADD_RANGE      = "(" + SINGLE + MINUS + SINGLE + ")";
    const std::string MultiFileNameParser::ADD_STEP_RANGE = "(" + SINGLE + MINUS + SINGLE + COLON + SINGLE + ")";

    const std::string MultiFileNameParser::ANY  = "(" + ADD_STEP_RANGE + "|" + ADD_RANGE + "|" + ADD_LIST + "|" + STEP_RANGE + "|" + RANGE + "|" + SINGLE + ")";
    const std::string MultiFileNameParser::LIST = "(" + ANY + "(" + COMMA + ANY + ")*" + ")";

    namespace
    {
      /**
       * Functor to convert a vector of run numbers, which only contains
       * a single run, into a string containing a cast of that run.
       *
       * @param run - a single run contained in a vector of 
       */
      struct convertUIntVect2String
      {
        std::string operator()(std::vector<unsigned int> run)
        {
          //if(run.size() != 1)
            //throw std::exception::exception("An unexpected run number was found during parsing.");

          return boost::lexical_cast<std::string>(run.at(0));
        }
      };

      /**
       * Functor that takes in a map of run information parsed so far and a runString, parses
       * parses the runString then appends the info to the map, returning it to the accumulate
       * STL algo that called it.
       *
       * @param parsedRuns - the map of info relating to the runs parsed so far
       * @param runString - the string containing new runs to be added to the map
       * @returns the updated map, appended with the data from the runString
       */
      struct parseRunRange
      {
        std::map<std::vector<unsigned int>, std::string> & operator()(
          std::map<std::vector<unsigned int>, 
          std::string> & parsedRuns, 
          const std::string & runString)
        {
          // Regex to separate non-added runs from the added runs.
          boost::regex regex("(" + 
            MultiFileNameParser::STEP_RANGE + "|" + 
            MultiFileNameParser::RANGE + "|" + 
            MultiFileNameParser::SINGLE + ")");

          // Deal with the case where we have an added run range or list to append to the map.
          if(!boost::regex_match(runString,regex))
          {
            // Use Sofia's run parser here, but it's only ever going to produce a vector with a 
            // single vector inside, since we only ever feed in one added bunch of runs.
            std::vector<unsigned int> runUInts = UserStringParser().parse(runString).at(0);

            // Add the newly parsed runs and the ws name to the map, and return it.
            parsedRuns.insert(
              parsedRuns.end(),
              std::pair<std::vector<unsigned int>, std::string>
                (runUInts, runString));

            return parsedRuns;
          }
          // We are left with the case where we have a NON-added run or run range to append to the map.
          else
          {
            // In this case the run parser will spit out a vector of vectors, each of the inner vectors 
            // containing only one run number.
            std::vector<std::vector<unsigned int> > runUInts = UserStringParser().parse(runString);

            // Convert the unsigned ints into strings.
            std::vector<std::string> runStrings;
            runStrings.resize(runUInts.size());
            std::transform(
              runUInts.begin(), runUInts.end(),
              runStrings.begin(),
              convertUIntVect2String());

            // @TODO:
            // Here would be the best place to insert code to make sure the run strings mantain any 
            // of the original trailing zeros entered by the user.

            // Add the newly parsed runs and the ws name to the map, and return it.
            std::vector<std::string>::const_iterator s_it = runStrings.cbegin();
            std::vector<std::vector<unsigned int> >::const_iterator vUI_it = runUInts.cbegin();
            for( ; s_it != runStrings.cend(); ++s_it, ++vUI_it)
            {
              parsedRuns.insert(
                parsedRuns.end(), 
                std::pair<std::vector<unsigned int>, std::string>
                  (*vUI_it, *s_it));
            }

            return parsedRuns;
          }
        }
      };
    } // Anonymous namespace

    /** 
     * Constructor
     */
    MultiFileNameParser::MultiFileNameParser() : 
      m_multiFileName(),
      m_dir(), 
      m_inst(),
      m_runs(),
      m_ext(),
      m_fileNamesToWsNameMap(),
      m_parser()
    {
    }
    
    /** 
     * Destructor
     */
    MultiFileNameParser::~MultiFileNameParser()
    {
    }

    /**
     * Main entry point to the class.  Parses the given multiFileName string
     * by calling all the helper private functions, and the functors.
     *
     * @param multiFileName - the string to parse into seperate file names
     * @returns a string containing any errors, "" if successful
     */
    std::string MultiFileNameParser::parse(const std::string & multiFileName)
    {
      try
      {
        m_multiFileName = multiFileName;
        split();

        // Split the run string into tokens, tokenised by commas.
        std::vector<std::string> tokens;
        boost::split(tokens, m_runs, boost::is_any_of(","));

        // Do some further tokenising of the tokens where necessary, and parse into
        // a UIntVect2StringMap which maps vectors of unsigned int run numbers to 
        // the eventual workspace name of that vector of runs.
        std::map<std::vector<unsigned int>, std::string> runUIntsToWsNameMap;
        runUIntsToWsNameMap = std::accumulate(
          tokens.begin(), tokens.end(),
          runUIntsToWsNameMap,
          parseRunRange());

        // Finally, convert the unsigned int run numbers back into full file names
        // to get our finished map, by passing each pair in runUIntsToWsNameMap
        // to populateMap.
        std::for_each(
          runUIntsToWsNameMap.begin(), runUIntsToWsNameMap.end(),
          boost::bind(&MultiFileNameParser::populateMap, this, _1));

        return "";
      }
      catch(const std::exception &e)
      {
        std::stringstream error;
        error << "Could not parse \"" << multiFileName << "\": " << e.what() << ".";
        clear();
        return error.str();
      }
    }

    /**
     * Getter for the m_fileNamesToWsNameMap.
     *
     * @returns the map
     */
    std::map<std::vector<std::string>, std::string> MultiFileNameParser::getFileNamesToWsNameMap() const
    {
      return m_fileNamesToWsNameMap;
    }

    /**
     * Clears all member variables, to be called when parsing exception caught.
     */
    void MultiFileNameParser::clear()
    {
      m_multiFileName.clear();
      m_dir.clear();
      m_inst.clear();
      m_runs.clear();
      m_ext.clear();
      m_fileNamesToWsNameMap.clear();
    }

    /**
     * Splits the string found at m_multiFileName into it's basic 
     * dir/inst/run/ext components.
     */
    void MultiFileNameParser::split()
    {
      //if(m_multiFileName.empty())
      //  throw std::exception("No file name to parse.");
      
      Poco::Path fullPath(m_multiFileName);
      
      // The extension is easy to parse, let's do it first.
      m_ext = fullPath.getExtension();
      if(!m_ext.empty()) m_ext = "." + m_ext;

      // Parse instrument name and runs from the base name using regexs.  Throw if 
      // not found since these are required.
      std::string baseName = fullPath.getBaseName();
      m_inst = getMatchingString("^" + INST + UNDERSCORE, baseName);
      //if(m_inst.empty()) throw std::exception("Cannot parse instrument name.");
      m_runs = getMatchingString(LIST + "$", baseName);
      //if(m_runs.empty()) throw std::exception("Cannot parse run numbers.");
      
      // Lop off what we've found and we're left with the directory.
      m_dir = m_multiFileName.substr(0, m_multiFileName.size() - (baseName.size() + m_ext.size()));
    }

    /**
     * Takes in a pair consisting of a vector of run numbers and a ws name,
     * converts the run numbers to full file names, and appends a new pair
     * to the filename to wsName map.
     *
     * @param pair - a std::pair consisting of a vector of run numbers and a ws name
     */
    void MultiFileNameParser::populateMap(
      const std::pair<std::vector<unsigned int>, std::string> & pair)
    {
      // Convert vector of run numbers to vector of filenames
      std::vector<unsigned int> runs = pair.first;
      std::vector<std::string> fileNames;
      fileNames.resize(runs.size());
      std::transform(
        runs.begin(), runs.end(),
        fileNames.begin(),
        boost::bind(&MultiFileNameParser::createFileName, this, _1));

      // Append to map
      m_fileNamesToWsNameMap.insert(std::make_pair(fileNames, pair.second));
    }

    /**
     * Uses the run parameter and the other parts of the file name that have
     * already been parsed to construct a new file name for this specific run.
     * 
     * @param run - the run number of the file
     * @returns the full file name of the file with given run number
     */
    std::string MultiFileNameParser::createFileName(unsigned int run)
    {
      std::stringstream fileName;
      fileName << m_dir << m_inst << boost::lexical_cast<std::string>(run) << m_ext;
      return fileName.str();
    }

    /**
     * Finds a given regex in a given string, and returns the part of the string
     * that matches the regex.  Returns "" if there is no occurance of the regex.
     * 
     * @param regex - the regular expression to find within the string
     * @param toParse - the string within to find the regex
     * @returns the part (if any) of the given string that matches the given regex
     */
    std::string MultiFileNameParser::getMatchingString(const std::string & regex, const std::string & toParse)
    {
      return boost::sregex_iterator(
        toParse.begin(), toParse.end(), 
        boost::regex(regex)
      )->str();
    }
  }
}

