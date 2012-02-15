//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/MultiFileNameParser.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Exception.h"

#include <algorithm>
#include <numeric>
#include <iterator>
#include <cassert>

#include <ctype.h>

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/bind.hpp>

#include <Poco/Path.h>

namespace Mantid
{
namespace Kernel
{
  namespace MultiFileNameParsing
  {
    /////////////////////////////////////////////////////////////////////////////
    // Static constants.
    /////////////////////////////////////////////////////////////////////////////
    
    namespace Regexs
    {
      const std::string INST = "([A-Za-z]+|PG3|pg3)";

      const std::string UNDERSCORE = "(_{0,1})";
      const std::string SPACE      = "(\\s*)";

      const std::string COMMA = "(" + SPACE + "," + SPACE + ")";
      const std::string PLUS  = "(" + SPACE + "\\+" + SPACE + ")";
      const std::string MINUS = "(" + SPACE + "\\-" + SPACE + ")";
      const std::string COLON = "(" + SPACE + ":" + SPACE + ")";

      const std::string SINGLE         = "([0-9]+)";
      const std::string RANGE          = "(" + SINGLE + COLON + SINGLE + ")";
      const std::string STEP_RANGE     = "(" + SINGLE + COLON + SINGLE + COLON + SINGLE + ")";
      const std::string ADD_LIST       = "(" + SINGLE + "(" + PLUS + SINGLE + ")+" + ")";
      const std::string ADD_RANGE      = "(" + SINGLE + MINUS + SINGLE + ")";
      const std::string ADD_STEP_RANGE = "(" + SINGLE + MINUS + SINGLE + COLON + SINGLE + ")";

      const std::string ANY  = "(" + ADD_STEP_RANGE + "|" + ADD_RANGE + "|" + ADD_LIST + "|" + STEP_RANGE + "|" + RANGE + "|" + SINGLE + ")";
      const std::string LIST = "(" + ANY + "(" + COMMA + ANY + ")*" + ")";
    }
      
    /////////////////////////////////////////////////////////////////////////////
    // Forward declarations.
    /////////////////////////////////////////////////////////////////////////////

    namespace
    {
      // Anonymous helper functions.
      std::vector<std::vector<unsigned int> > & parseToken(std::vector<std::vector<unsigned int> > & parsedRuns, const std::string & token);
      std::vector<std::vector<unsigned int> > generateRange(unsigned int from, unsigned int to, unsigned int stepSize, bool addRuns);
      void validateToken(const std::string & token);
      bool matchesFully(const std::string & stringToMatch, const std::string & regexString);
      std::string getMatchingString(const std::string & regexString, const std::string & toParse);
      std::string pad(std::string run, unsigned int padLength);
    }
    
    /////////////////////////////////////////////////////////////////////////////
    // Scoped, global functions.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * Parses a string containing a comma separated list of run "tokens", where each run
     * token is of one of the allowed forms (a single run or a range of runs or an added
     * range of runs, etc.)
     *
     * @param runString :: a string containing the runs to parse, in the correct format.
     *
     * @returns a vector of vectors of unsigned ints, one int for each run, where runs 
     *    to be added are contained in the same sub-vector.
     * @throws std::runtime_error when runString provided is in an incorrect format.
     */
    std::vector<std::vector<unsigned int> > parseMultiRunString(std::string runString)
    {
      // If the run string is empty, return no runs.
      if(runString.empty())
        return std::vector<std::vector<unsigned int> >();
      
      // Remove whitespace.
      runString.erase(std::remove_if( // ("Erase-remove" idiom.)
          runString.begin(), runString.end(),
          isspace),
        runString.end());

      // Only numeric characters, or occurances of plus, minus, comma and colon are allowed.
      if(!matchesFully(runString,"([0-9]|\\+|\\-|,|:)+"))
        throw std::runtime_error(
          "Non-numeric or otherwise unaccetable character(s) detected.");

      // Tokenize on commas.
      std::vector<std::string> tokens;
      tokens = boost::split(tokens, runString, boost::is_any_of(","));

      // Validate each token.
      std::for_each(
        tokens.begin(), tokens.end(),
        validateToken);

      // Parse each token, accumulate the results, and return them.
      return std::accumulate(
        tokens.begin(), tokens.end(),
        std::vector<std::vector<unsigned int> >(),
        parseToken);
    }

    /////////////////////////////////////////////////////////////////////////////
    // Public member functions of Parser class.
    /////////////////////////////////////////////////////////////////////////////

    /// Constructor.
    Parser::Parser() :
      m_runs(), m_fileNames(), m_multiFileName(), m_dirString(), m_instString(), 
      m_underscoreString(), m_runString(), m_extString(), m_zeroPadding()
    {}
    
    /// Destructor.
    Parser::~Parser()
    {}

    /**
     * Takes the given multiFileName string, and calls other parts of the parser
     * to generate a corresponding vector of vectors of file names.
     *
     * @param multiFileName :: the string containing the multiple file names to be parsed.
     */
    void Parser::parse(const std::string & multiFileName)
    {
      // Clear any contents of the member variables.
      clear();
      
      // Set the string to parse.
      m_multiFileName = multiFileName;
      
      // Split the string to be parsed into sections, and do some validation.
      split();

      // Parse the run section into unsigned ints we can use.
      m_runs = parseMultiRunString(m_runString);

      // Set up helper functor.
      GenerateFileName generateFileName(
        m_dirString + m_instString + m_underscoreString,
        m_extString,
        m_zeroPadding);

      // Generate complete file names for each run using helper functor.
      std::transform(
        m_runs.begin(), m_runs.end(),
        std::back_inserter(m_fileNames),
        generateFileName);
    }

    /////////////////////////////////////////////////////////////////////////////
    // Private member functions of Parser class.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * Clears all member variables.
     */
    void Parser::clear()
    {
      m_runs.clear();
      m_fileNames.clear();
      m_multiFileName.clear();
      m_dirString.clear();
      m_instString.clear();
      m_underscoreString.clear();
      m_runString.clear();
      m_extString.clear();
    }

    /**
     * Splits up the m_multiFileName string into component parts, to be used elsewhere by
     * the parser.  Some validation is done here, and exceptions thrown if required
     * components are missing.
     *
     * @throws std::runtime_error if a required component is not present in the string.
     */
    void Parser::split()
    {
      if(m_multiFileName.empty())
        throw std::runtime_error("No file name to parse.");

      // (We shun the use of Poco::File here as it is unable to deal with certain 
      // combinations of special characters, for example double commas.)

      // Get the extension, if there is one.
      size_t lastDot = m_multiFileName.find_last_of(".");
      if(lastDot != std::string::npos)
        m_extString = m_multiFileName.substr(lastDot);

      // Get the directory, if there is one.
      size_t lastSeparator = m_multiFileName.find_last_of("/\\");
      if(lastSeparator != std::string::npos)
        m_dirString = m_multiFileName.substr(0, lastSeparator + 1);

      // If the directory contains an instance of a comma, then the string is
      // a comma separated list of single *full* file names to load.
      if(std::string::npos != m_dirString.find(","))
        throw std::runtime_error("Unable to parse.");

      // Slice off the directory and extension.
      std::string base = m_multiFileName.substr(
        m_dirString.size(), m_multiFileName.size() - (m_dirString.size() + m_extString.size()));

      // Get the instrument name using a regex.  Throw if not found since this is required.
      m_instString = getMatchingString("^" + Regexs::INST, base);
      if(m_instString.empty())
        throw std::runtime_error("There does not appear to be an instrument name present.");

      // Check if instrument exists, if not then clear the parser, and rethrow an exception.
      try
      {
        InstrumentInfo instInfo = ConfigService::Instance().getInstrument(m_instString);
        m_zeroPadding = instInfo.zeroPadding();
      }
      catch (const Exception::NotFoundError &)
      {
        throw std::runtime_error("There does not appear to be a valid instrument name present.");
      }

      // Check for an underscore after the instrument name.
      size_t underscore = base.find_first_of("_");
      if(underscore == m_instString.size())
        m_underscoreString = "_";

      // We can now deduce the run string.  Throw if not found since this is required.
      m_runString = base.substr(m_underscoreString.size() + m_instString.size());
      if(m_instString.empty())
        throw std::runtime_error("There does not appear to be any runs present.");
    }

    /////////////////////////////////////////////////////////////////////////////
    // Helper functor.
    /////////////////////////////////////////////////////////////////////////////

    /**
     * Constructor, to accept state used in generating file names.
     *
     * @param prefix      :: a string that prefixes the generated file names.
     * @param suffix      :: a string that suffixes the generated file names.
     * @param zeroPadding :: the number of zeros with which to pad the run number of genrerated file names.
     */
    GenerateFileName::GenerateFileName(const std::string & prefix, const std::string & suffix, int zeroPadding) :
        m_prefix(prefix), m_suffix(suffix), m_zeroPadding(zeroPadding)
      {}

    /**
     * Overloaded function operator that takes in a vector of runs, and returns a vector of file names.
     *
     * @param runs :: the vector of runs with which to make file names.
     *
     * @returns the generated vector of file names.
     */
    std::vector<std::string> GenerateFileName::operator()(const std::vector<unsigned int> & runs)
    {
      std::vector<std::string> fileNames; 

      std::transform(
        runs.begin(), runs.end(),
        std::back_inserter(fileNames),
        (*this) // Call other overloaded function operator.
      );

      return fileNames;
    }

    /**
     * Overloaded function operator that takes in a runs, and returns a file name.
     *
     * @param runs :: the vector of runs with which to make file names.
     *
     * @returns the generated vector of file names.
     */
    std::string GenerateFileName::operator()(unsigned int run)
    {
      std::stringstream fileName;

      fileName << m_prefix
               << pad(boost::lexical_cast<std::string>(run), m_zeroPadding)
               << m_suffix;

      return fileName.str();
    }

    /////////////////////////////////////////////////////////////////////////////
    // Anonymous helper functions.
    /////////////////////////////////////////////////////////////////////////////

    namespace // anonymous
    {
      /**
       * Parses a string containing a run "token".
       *
       * Note that this function takes the form required by the "accumulate" algorithm:
       * it takes in the parsed runs so far and a new token to parse, and then returns
       * the result of appending the newly parsed token to the already parsed runs.
       *
       * @param parsedRuns :: the vector of vectors of runs parsed so far.
       * @param token      :: the token to parse.
       *
       * @returns the newly parsed runs appended to the previously parsed runs.
       * @throws std::runtime_error if 
       */
      std::vector<std::vector<unsigned int> > & parseToken(
        std::vector<std::vector<unsigned int> > & parsedRuns, const std::string & token)
      {
        // Tokenise further, on plus, minus or colon.
        std::vector<std::string> subTokens;
        subTokens = boost::split(subTokens, token, boost::is_any_of("+-:"));

        std::vector<unsigned int> rangeDetails;

        // Convert the sub tokens to uInts.
        std::transform(
          subTokens.begin(), subTokens.end(),
          std::back_inserter(rangeDetails),
          boost::lexical_cast<unsigned int, std::string>);

        // We should always end up with at least 1 unsigned int here.
        assert(1 <= rangeDetails.size());
        
        std::vector<std::vector<unsigned int> > runs;

        // E.g. "2012".
        if(matchesFully(token, Regexs::SINGLE))
        {
          runs.push_back(std::vector<unsigned int>(1, rangeDetails[0]));
        }
        // E.g. "2012:2020".
        else if(matchesFully(token, Regexs::RANGE))
        {
          runs = generateRange(
            rangeDetails[0],
            rangeDetails[1],
            1,
            false
          );
        }
        // E.g. "2012:2020:4".
        else if(matchesFully(token, Regexs::STEP_RANGE))
        {
          runs = generateRange(
            rangeDetails[0],
            rangeDetails[1],
            rangeDetails[2],
            false
          );
        }
        // E.g. "2012+2013+2014+2015".
        else if(matchesFully(token, Regexs::ADD_LIST))
        {
          // No need to generate the range here, it's already there for us.
          runs = std::vector<std::vector<unsigned int> >(1, rangeDetails);
        }
        // E.g. "2012-2020".
        else if(matchesFully(token, Regexs::ADD_RANGE))
        {
          runs = generateRange(
            rangeDetails[0],
            rangeDetails[1],
            1,
            true
          );
        }
        // E.g. "2012-2020:4".
        else if(matchesFully(token, Regexs::ADD_STEP_RANGE))
        {
          runs = generateRange(
            rangeDetails[0],
            rangeDetails[1],
            rangeDetails[2],
            true
          );
        }
        else
        {
          // We should never reach here - the validation done on the token previously
          // should prevent any other possible scenario.
          assert(false);
        }
        
        // Add the runs on to the end of parsedRuns, and return it.
        std::copy(
          runs.begin(), runs.end(),
          std::back_inserter(parsedRuns));

        return parsedRuns;
      }
      
      /**
       * Generates a range of runs between the given numbers, increasing
       * or decreasing by the given step size. If addRuns is true, then the
       * runs will all be in the same sub vector, if false they will each be
       * in their own vector.
       *
       * @param from     :: the start of the range
       * @param to       :: the end of the range
       * @param stepSize :: the size of the steps with which to increase/decrease
       * @param addRuns  :: whether or not to add the runs together (place in sume sub-vector)
       *
       * @returns a vector of vectors of runs.
       * @throws std::runtime_error if a step size of zero is specified.
       */
      std::vector<std::vector<unsigned int> > generateRange(
        unsigned int from, 
        unsigned int to, 
        unsigned int stepSize,
        bool addRuns)
      {
        if(stepSize == 0)
          throw std::runtime_error(
            "Unable to generate a range with a step size of zero.");

        unsigned int currentRun = from;
        std::vector<std::vector<unsigned int> > runs;

        // If ascending range
        if(from <= to) 
        {
          while(currentRun <= to)
          {
            if(addRuns)
            {
              if(runs.empty())
                runs.push_back(std::vector<unsigned int>(1, currentRun));
              else
                runs.at(0).push_back(currentRun);
            }
            else
            {
              runs.push_back(std::vector<unsigned int>(1, currentRun));
            }

            currentRun += stepSize;
          }
        }
        // Else descending range
        else
        {
          while(currentRun >= to)
          {
            if(addRuns)
            {
              if(runs.empty())
                runs.push_back(std::vector<unsigned int>(1, currentRun));
              else
                runs.at(0).push_back(currentRun);
            }
            else
            {
              runs.push_back(std::vector<unsigned int>(1, currentRun));
            }

            // Guard against case where stepSize would take us into negative 
            // numbers (not supported by unsigned ints ...).
            if(static_cast<int>(currentRun) - static_cast<int>(stepSize) < 0)
              break;

            currentRun -= stepSize;
          }
        }

        return runs;
      }
      
      /**
       * Validates the given run token.
       *
       * @param :: the run token to validate.
       *
       * @throws std::runtime_error if the token is of an incorrect form.
       */
      void validateToken(const std::string & token)
      {
        // Each token must be non-empty.
        if(token.size() == 0)
          throw std::runtime_error(
            "A comma-separated token is empty.");

        // Each token must begin and end with a numeric character.
        if(!matchesFully(token, "[0-9].+[0-9]|[0-9]"))
          throw std::runtime_error(
            "The token \"" + token + "\" is of an incorrect form.  Does it begin or end with a plus, minus or colon?");

        // Each token must be one of the acceptable forms, i.e. a single run, an added range of runs, etc.
        if(!matchesFully(token, Regexs::ANY))
          throw std::runtime_error(
            "The token \"" + token + "\" is of an incorrect form.");
      }

      /**
       * Convenience function that matches a *complete* given string to the given regex.
       *
       * @param stringToMatch :: the string to match with the given regex.
       * @param regexString   :: the regex with which to match the given string.
       *
       * @returns true if the string matches fully, or false otherwise.
       */
      bool matchesFully(const std::string & stringToMatch, const std::string & regexString)
      {
        const boost::regex regex("^(" + regexString + "$)");
        return boost::regex_match(stringToMatch, regex);
      }

      /**
       * Finds a given regex in a given string, and returns the part of the string
       * that matches the regex.  Returns "" if there is no occurance of the regex.
       * 
       * @param regex - the regular expression to find within the string
       * @param toParse - the string within to find the regex
       *
       * @returns the part (if any) of the given string that matches the given regex
       */
      std::string getMatchingString(const std::string & regexString, const std::string & toParse)
      {
        boost::sregex_iterator it(
          toParse.begin(), toParse.end(), 
          boost::regex(regexString)
        );

        if(it == boost::sregex_iterator())
          return "";

        return it->str();
      }

      /**
       * Zero pads the run number used in a file name to required length.
       *
       * @param run - the run number of the file.  May as well pass by value here.
       * @param count - the required length of the string.
       *
       * @returns the string, padded to the required length.
       * @throws std::runtime_error if run is longer than size of count.
       */
      std::string pad(std::string run, unsigned int padLength)
      {
        if(run.size() < padLength)
          return run.insert(0, padLength - run.size(), '0');
        else if(run.size() > padLength)
          throw std::runtime_error("Could not parse run number \"" + run + 
            "\" since the instrument run number length required is " + boost::lexical_cast<std::string>(padLength));
        return run;
      }

    } // anonymous namespace

  } // namespace MultiFileNameParsing

} // namespace Kernel
} // namespace Mantid
