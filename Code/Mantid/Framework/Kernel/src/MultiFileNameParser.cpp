//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/MultiFileNameParser.h"

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
    } // namespace Regexs
      
    /////////////////////////////////////////////////////////////////////////////
    // Forward declarations.
    /////////////////////////////////////////////////////////////////////////////

    namespace
    {
      std::vector<std::vector<unsigned int> > & parseToken(std::vector<std::vector<unsigned int> > & parsedRuns, const std::string & token);
      std::vector<std::vector<unsigned int> > generateRange(unsigned int from, unsigned int to, unsigned int stepSize, bool addRuns);
      void validateToken(const std::string & token);
      bool matchesFully(const std::string & stringToMatch, const std::string & regexString);
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

    } // anonymous namespace

  } // namespace MultiFileNameParsing

} // namespace Kernel
} // namespace Mantid
