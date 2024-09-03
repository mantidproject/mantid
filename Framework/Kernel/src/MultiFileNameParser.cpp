// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/MultiFileNameParser.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Strings.h"

#include <numeric>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <utility>

namespace Mantid::Kernel::MultiFileNameParsing {
/////////////////////////////////////////////////////////////////////////////
// Static constants.
/////////////////////////////////////////////////////////////////////////////

namespace Regexs {
const std::string INST = "([A-Za-z]+|PG3|pg3)";

const std::string UNDERSCORE = "(_{0,1})";
const std::string SPACE = "(\\s*)";

const std::string COMMA = "(" + SPACE + "," + SPACE + ")";
const std::string PLUS = "(" + SPACE + "\\+" + SPACE + ")";
const std::string MINUS = "(" + SPACE + "\\-" + SPACE + ")";
const std::string COLON = "(" + SPACE + ":" + SPACE + ")";

const std::string SINGLE = "(" + INST + "*[0-9]+)";
const std::string RANGE = "(" + SINGLE + COLON + SINGLE + ")";
const std::string STEP_RANGE = "(" + SINGLE + COLON + SINGLE + COLON + SINGLE + ")";
const std::string ADD_RANGE = "(" + SINGLE + MINUS + SINGLE + ")";
const std::string ADD_STEP_RANGE = "(" + SINGLE + MINUS + SINGLE + COLON + SINGLE + ")";
const std::string SINGLE_OR_STEP_OR_ADD_RANGE = "(" + ADD_STEP_RANGE + "|" + ADD_RANGE + "|" + SINGLE + ")";
const std::string ADD_LIST = "(" + SINGLE_OR_STEP_OR_ADD_RANGE + "(" + PLUS + SINGLE_OR_STEP_OR_ADD_RANGE + ")+" + ")";

const std::string ANY =
    "(" + ADD_STEP_RANGE + "|" + ADD_LIST + "|" + ADD_RANGE + "|" + STEP_RANGE + "|" + RANGE + "|" + SINGLE + ")";
const std::string LIST = "(" + ANY + "(" + COMMA + ANY + ")*" + ")";
} // namespace Regexs

/////////////////////////////////////////////////////////////////////////////
// Forward declarations.
/////////////////////////////////////////////////////////////////////////////

namespace {
// Anonymous helper functions.
void parseToken(std::vector<std::vector<unsigned int>> &parsedRuns, const std::string &token);
std::vector<std::vector<unsigned int>> generateRange(const unsigned int from, const unsigned int to,
                                                     const unsigned int stepSize, const bool addRuns);
void validateToken(const std::string &token);
bool matchesFully(const std::string &stringToMatch, const std::string &regexString, const bool caseless = false);
std::string getMatchingString(const std::string &regexString, const std::string &toParse, const bool caseless = false);
std::string pad(const unsigned int run, const std::string &instString);

std::set<std::pair<unsigned int, unsigned int>>
mergeAdjacentRanges(std::set<std::pair<unsigned int, unsigned int>> ranges,
                    const std::pair<unsigned int, unsigned int> &range);

// Helper functor.
struct RangeContainsRun {
  bool operator()(const std::pair<unsigned int, unsigned int> &range, const unsigned int run);
  bool operator()(const unsigned int run, const std::pair<unsigned int, unsigned int> &range);
};

std::string toString(const RunRangeList &runRangeList);
std::string accumulateString(std::string output, std::pair<unsigned int, unsigned int> runRange);
} // namespace

/////////////////////////////////////////////////////////////////////////////
// Scoped, global functions.
/////////////////////////////////////////////////////////////////////////////

/**
 * Suggests a workspace name for the given vector of file names (which, because
 * they are in the same vector, we will assume they are to be added.)  Example:
 *
 * Parsing ["INST_4.ext", "INST_5.ext", "INST_6.ext", "INST_8.ext"] will return
 * "INST_4_to_6_and_8" as a suggested workspace name.
 *
 * @param fileNames :: a vector of file names
 *
 * @returns a string containing a suggested workspace name.
 * @throws std::runtime_error when runString provided is in an incorrect format.
 */
std::string suggestWorkspaceName(const std::vector<std::string> &fileNames) {
  Parser parser;
  RunRangeList runs;

  // For each file name, parse the run number out of it, and add it to a
  // RunRangeList.
  for (const auto &fileName : fileNames) {
    parser.parse(fileName);
    runs.addRun(parser.runs()[0][0]);
  }

  // Return the suggested ws name.
  return parser.instString() + parser.underscoreString() + toString(runs);
}

/////////////////////////////////////////////////////////////////////////////
// Comparator class.
/////////////////////////////////////////////////////////////////////////////

/**
 * Comparator for the set that holds instrument names in Parser.  This is
 * reversed since we want to come across the longer instrument names first.
 * It is caseless so we don't get "inst" coming before "INSTRUMENT" -
 * though this is probably overkill.
 */
bool ReverseCaselessCompare::operator()(const std::string &a, const std::string &b) const {
  std::string lowerA;
  lowerA.resize(a.size());
  std::string lowerB;
  lowerB.resize(b.size());

  std::transform(a.cbegin(), a.cend(), lowerA.begin(), tolower);
  std::transform(b.cbegin(), b.cend(), lowerB.begin(), tolower);

  return lowerA > lowerB;
}

/////////////////////////////////////////////////////////////////////////////
// Public member functions of Parser class.
/////////////////////////////////////////////////////////////////////////////

/// Constructor.
Parser::Parser()
    : m_runs(), m_fileNames(), m_multiFileName(), m_dirString(), m_instString(), m_underscoreString(), m_runString(),
      m_extString(), m_validInstNames(), m_trimWhiteSpaces(true) {
  const ConfigServiceImpl &config = ConfigService::Instance();

  const auto facilities = config.getFacilities();
  for (const auto facility : facilities) {
    const std::vector<InstrumentInfo> instruments = facility->instruments();

    for (const auto &instrument : instruments) {
      m_validInstNames.insert(instrument.name());
      m_validInstNames.insert(instrument.shortName());
    }
  }
}

/**
 * Takes the given multiFileName string, and calls other parts of the parser
 * to generate a corresponding vector of vectors of file names.
 *
 * @param multiFileName :: the string containing the multiple file names to be
 *parsed.
 */
void Parser::parse(const std::string &multiFileName) {
  // Clear any contents of the member variables.
  clear();

  // Set the string to parse.
  m_multiFileName = multiFileName;

  // Split the string to be parsed into sections, and do some validation.
  split();

  // Parse the run section into unsigned ints we can use.
  m_runs = parseMultiRunString(m_runString);

  // Set up helper functor.
  GenerateFileName generateFileName(m_dirString, m_extString, m_instString);

  // Generate complete file names for each run using helper functor.
  std::transform(m_runs.begin(), m_runs.end(), std::back_inserter(m_fileNames), generateFileName);
}

/**
 * Parses a string containing a comma separated list of run "tokens", where
 * each run token is of one of the allowed forms (a single run or a range
 * of runs or an added range of runs, etc.)
 *
 * @param runString :: a string containing the runs to parse, in the correct
 *format.
 * @returns a vector of vectors of unsigned ints, one int for each run, where
 *runs to be added are contained in the same sub-vector.
 * @throws std::runtime_error when runString provided is in an incorrect format.
 */
std::vector<std::vector<unsigned int>> Parser::parseMultiRunString(std::string runString) {
  // If the run string is empty, return no runs.
  if (runString.empty())
    return std::vector<std::vector<unsigned int>>();

  // Remove whitespaces if requested.
  if (trimWhiteSpaces()) {
    runString.erase(std::remove_if( // ("Erase-remove" idiom.)
                        runString.begin(), runString.end(), isspace),
                    runString.end());
  }
  // Only numeric characters, or occurances of plus, minus, comma and colon are
  // allowed.
  if (!matchesFully(runString, "([0-9]|\\+|\\-|,|:)+")) {
    throw std::runtime_error("Non-numeric or otherwise unaccetable character(s) detected.");
  }

  // Tokenize on commas.
  std::vector<std::string> tokens;
  tokens = boost::split(tokens, runString, boost::is_any_of(","));

  // Validate each token.
  std::for_each(tokens.begin(), tokens.end(), validateToken);

  // Parse each token, accumulate the results, and return them.
  std::vector<std::vector<unsigned int>> runGroups;
  for (auto const &token : tokens) {
    parseToken(runGroups, token);
  }
  return runGroups;
}

/**
 * Returns value of trimming whitespace from input
 *
 * @returns True/False
 */
bool Parser::trimWhiteSpaces() const { return m_trimWhiteSpaces; }

/**
 * Sets if the property is set to automatically trim string unput values of
 * whitespace
 *
 * @param setting The new setting value
 */
void Parser::setTrimWhiteSpaces(const bool &setting) { m_trimWhiteSpaces = setting; }

/////////////////////////////////////////////////////////////////////////////
// Private member functions of Parser class.
/////////////////////////////////////////////////////////////////////////////

/**
 * Clears all member variables.
 */
void Parser::clear() {
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
 * Splits up the m_multiFileName string into component parts, to be used
 * elsewhere by  the parser.  Some validation is done here, and exceptions
 * thrown if required components are missing.
 *
 * @throws std::runtime_error if a required component is not present in the
 *string.
 */
void Parser::split() {
  if (m_multiFileName.empty())
    throw std::runtime_error("No file name to parse.");

  // (We shun the use of Poco::File here as it is unable to deal with certain
  // combinations of special characters, for example double commas.)

  // Clear whitespace before getting extentions and directories, if requested.
  if (trimWhiteSpaces()) {
    m_multiFileName.erase(std::remove_if( // ("Erase-remove" idiom.)
                              m_multiFileName.begin(), m_multiFileName.end(), isspace),
                          m_multiFileName.end());
  }
  // Get the extension, if there is one.
  const size_t lastDot = m_multiFileName.find_last_of('.');
  if (lastDot != std::string::npos)
    m_extString = m_multiFileName.substr(lastDot);

  // Get the directory, if there is one.
  const size_t lastSeparator = m_multiFileName.find_last_of("/\\");
  if (lastSeparator != std::string::npos)
    m_dirString = m_multiFileName.substr(0, lastSeparator + 1);

  // If the directory contains an instance of a comma, then the string is
  // a comma separated list of single *full* file names to load.
  if (std::string::npos != m_dirString.find(','))
    throw std::runtime_error("Unable to parse.");

  // Slice off the directory and extension.
  std::string base =
      m_multiFileName.substr(m_dirString.size(), m_multiFileName.size() - (m_dirString.size() + m_extString.size()));

  if (base.empty())
    throw std::runtime_error("There does not appear to be any runs present.");

  auto instrumentNameIt = std::find_if(m_validInstNames.cbegin(), m_validInstNames.cend(), [&base](const auto &name) {
    return matchesFully(base, name + ".*", true);
  }); // USE CASELESS MATCHES HERE.

  // See if the user has typed in one of the available instrument names.
  if (instrumentNameIt != m_validInstNames.cend()) {
    m_instString = getMatchingString("^" + *instrumentNameIt, base, true);
  }

  // If not, use the default, or throw if we encounter an unrecognisable
  // non-numeric string.
  if (m_instString.empty()) {
    if (base.empty())
      throw std::runtime_error("There does not appear to be any runs present.");

    if (isdigit(base[0]))
      m_instString = ConfigService::Instance().getString("default.instrument");
    else
      throw std::runtime_error("There does not appear to be a valid instrument name present.");
  } else {
    // Chop off instrument name.
    base = base.substr(m_instString.size(), base.size());
  }

  if (base.empty())
    throw std::runtime_error("There does not appear to be any runs present.");

  const auto &instInfo = ConfigService::Instance().getInstrument(m_instString);
  // why?
  // m_instString = instInfo.shortName(); // Make sure we're using the shortened
  // form of the isntrument name.

  if (boost::starts_with(base, instInfo.delimiter())) {
    // Store the instrument delimiter, and strip it off the start of the string.
    m_underscoreString = instInfo.delimiter();
    base = base.substr(m_underscoreString.size(), base.size());
  }

  m_runString = getMatchingString("^" + Regexs::LIST, base);

  if (m_runString.size() != base.size()) {
    throw std::runtime_error("There is an unparsable token present.");
  }
}

/////////////////////////////////////////////////////////////////////////////
// Helper functor.
/////////////////////////////////////////////////////////////////////////////

/**
 * Constructor, to accept state used in generating file names.
 *
 * @param prefix      :: a string that prefixes the generated file names.
 * @param suffix      :: a string that suffixes the generated file names.
 * @param instString :: the instrument name
 */
GenerateFileName::GenerateFileName(std::string prefix, std::string suffix, std::string instString)
    : m_prefix(std::move(prefix)), m_suffix(std::move(suffix)), m_instString(std::move(instString)) {}

/**
 * Overloaded function operator that takes in a vector of runs, and returns a
 *vector of file names.
 *
 * @param runs :: the vector of runs with which to make file names.
 *
 * @returns the generated vector of file names.
 */
std::vector<std::string> GenerateFileName::operator()(const std::vector<unsigned int> &runs) {
  std::vector<std::string> fileNames;

  std::transform(runs.begin(), runs.end(), std::back_inserter(fileNames),
                 (*this) // Call other overloaded function operator.
  );

  return fileNames;
}

/**
 * Overloaded function operator that takes in a runs, and returns a file name.
 *
 * @param run :: the vector of runs with which to make file names.
 *
 * @returns the generated vector of file names.
 */
std::string GenerateFileName::operator()(const unsigned int run) {
  std::stringstream fileName;

  fileName << m_prefix << pad(run, m_instString) << m_suffix;

  return fileName.str();
}

/////////////////////////////////////////////////////////////////////////////
// Public member functions of RunRangeList class.
/////////////////////////////////////////////////////////////////////////////

/**
 * Default constructor.
 */
RunRangeList::RunRangeList() : m_rangeList() {}

/**
 * Adds a run to the list of run ranges.  Not particularly effecient.
 *
 * @param run :: the run to add.
 */
void RunRangeList::addRun(const unsigned int run) {
  // If the run is inside one of the ranges, do nothing.
  if (std::binary_search(m_rangeList.begin(), m_rangeList.end(), run, RangeContainsRun()))
    return;

  // Else create a new range, containing a single run, and add it to the list.
  m_rangeList.emplace(run, run);

  // Now merge any ranges that are adjacent.
  m_rangeList = std::accumulate(m_rangeList.begin(), m_rangeList.end(),
                                std::set<std::pair<unsigned int, unsigned int>>(), mergeAdjacentRanges);
}

/**
 * Adds a range of runs of specified length to the list of run ranges.
 *
 * @param from :: the beginning of the run to add
 * @param to   :: the end of the run to add
 */
void RunRangeList::addRunRange(unsigned int from, unsigned int to) {
  for (; from <= to; ++from)
    addRun(from);
}

/**
 * Add a range of runs to the list of run ranges.
 *
 * @param range :: the range to add
 */
void RunRangeList::addRunRange(const std::pair<unsigned int, unsigned int> &range) {
  addRunRange(range.first, range.second);
}

/////////////////////////////////////////////////////////////////////////////
// Anonymous helper functions.
/////////////////////////////////////////////////////////////////////////////

namespace // anonymous
{
/**
 * Parses a string containing a run "token" and adds the runs to the parsedRuns
 * vector.
 *
 * @param parsedRuns :: the vector of vectors of runs parsed so far.
 * @param token      :: the token to parse.
 */
void parseToken(std::vector<std::vector<unsigned int>> &parsedRuns, const std::string &token) {
  std::vector<std::vector<unsigned int>> runs;
  // Tokenise further on plus.
  std::vector<std::string> subTokens;
  boost::split(subTokens, token, boost::is_any_of("+"));
  std::vector<unsigned int> runsToAdd;
  for (auto const &subToken : subTokens) {
    // E.g. "2012".
    if (matchesFully(subToken, Regexs::SINGLE)) {
      runsToAdd.emplace_back(std::stoi(subToken));
    }
    // E.g. "2012:2020".
    else if (matchesFully(subToken, Regexs::RANGE)) {
      // Fill in runs directly.
      constexpr bool addRuns{false};
      std::vector<std::string> rangeDetails;
      rangeDetails.reserve(2);
      boost::split(rangeDetails, subToken, boost::is_any_of(":"));
      runs = generateRange(std::stoi(rangeDetails.front()), std::stoi(rangeDetails.back()), 1, addRuns);
    }
    // E.g. "2012:2020:4".
    else if (matchesFully(subToken, Regexs::STEP_RANGE)) {
      // Fill in runs directly.
      constexpr bool addRuns{false};
      std::vector<std::string> rangeDetails;
      rangeDetails.reserve(3);
      boost::split(rangeDetails, subToken, boost::is_any_of(":"));
      runs = generateRange(std::stoi(rangeDetails[0]), std::stoi(rangeDetails[1]), std::stoi(rangeDetails[2]), addRuns);
    }
    // E.g. "2012-2020".
    else if (matchesFully(subToken, Regexs::ADD_RANGE)) {
      constexpr bool addRuns{true};
      std::vector<std::string> rangeDetails;
      rangeDetails.reserve(2);
      boost::split(rangeDetails, subToken, boost::is_any_of("-"));
      const auto generated = generateRange(std::stoi(rangeDetails.front()), std::stoi(rangeDetails.back()), 1, addRuns);
      std::copy(generated.front().cbegin(), generated.front().cend(), back_inserter(runsToAdd));
    }
    // E.g. "2012-2020:4".
    else if (matchesFully(subToken, Regexs::ADD_STEP_RANGE)) {
      constexpr bool addRuns{true};
      std::vector<std::string> rangeDetails;
      rangeDetails.reserve(3);
      boost::split(rangeDetails, subToken, boost::is_any_of("-:"));
      const auto generated =
          generateRange(std::stoi(rangeDetails[0]), std::stoi(rangeDetails[1]), std::stoi(rangeDetails[2]), addRuns);
      std::copy(generated.front().cbegin(), generated.front().cend(), back_inserter(runsToAdd));
    } else {
      // We should never reach here - the validation done on the token
      // previously should prevent any other possible scenario.
      assert(false);
    }
  }
  if (!runsToAdd.empty()) {
    if (!runs.empty()) {
      // We have either add ranges or step ranges. Never both.
      throw std::runtime_error("Unable to handle a mixture of add ranges and step ranges");
    }
    runs.emplace_back(runsToAdd);
  }
  // Add the runs on to the end of parsedRuns, and return it.
  std::copy(runs.begin(), runs.end(), std::back_inserter(parsedRuns));
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
 * @param addRuns  :: whether or not to add the runs together (place in sume
 *sub-vector)
 *
 * @returns a vector of vectors of runs.
 * @throws std::runtime_error if a step size of zero is specified.
 */
std::vector<std::vector<unsigned int>> generateRange(unsigned int const from, unsigned int const to,
                                                     unsigned int const stepSize, bool const addRuns) {
  if (stepSize == 0)
    throw std::runtime_error("Unable to generate a range with a step size of zero.");

  size_t limit;
  auto limitStr = ConfigService::Instance().getValue<std::string>("loading.multifilelimit");
  if (!limitStr.is_initialized() || !Strings::convert(limitStr.get(), limit)) {
    limit = ConfigService::Instance().getFacility().multiFileLimit();
  }

  unsigned int const orderedTo = from > to ? from : to;
  unsigned int const orderedFrom = from > to ? to : from;
  unsigned int const numberOfFiles = (orderedTo - orderedFrom) / stepSize;
  if (numberOfFiles > limit) {
    std::stringstream sstream;
    sstream << "The range from " << orderedFrom << " to " << orderedTo << " with step " << stepSize
            << " would generate " << numberOfFiles << " files.  "
            << "This is greater than the current limit of " << limit << ".  "
            << "This limit can be configured in the Mantid.user.properties "
               "file using the key loading.multifilelimit=200.";
    throw std::range_error(sstream.str());
  }

  unsigned int currentRun = from;
  std::vector<std::vector<unsigned int>> runs;

  // If ascending range
  if (from <= to) {
    while (currentRun <= to) {
      if (addRuns) {
        if (runs.empty())
          runs.emplace_back(1, currentRun);
        else
          runs.front().emplace_back(currentRun);
      } else {
        runs.emplace_back(1, currentRun);
      }

      currentRun += stepSize;
    }
  }
  // Else descending range
  else {
    while (currentRun >= to) {
      if (addRuns) {
        if (runs.empty())
          runs.emplace_back(1, currentRun);
        else
          runs.front().emplace_back(currentRun);
      } else {
        runs.emplace_back(1, currentRun);
      }

      // Guard against case where stepSize would take us into negative
      // numbers (not supported by unsigned ints ...).
      if (static_cast<int>(currentRun) - static_cast<int>(stepSize) < 0)
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
void validateToken(const std::string &token) {
  // Each token must be non-empty.
  if (token.empty())
    throw std::runtime_error("A comma-separated token is empty.");

  // Each token must begin and end with a numeric character.
  if (!matchesFully(token, "[0-9].+[0-9]|[0-9]"))
    throw std::runtime_error("The token \"" + token +
                             "\" is of an incorrect form.  Does it begin or "
                             "end with a plus, minus or colon?");

  // Each token must be one of the acceptable forms, i.e. a single run, an added
  // range of runs, etc.
  if (!matchesFully(token, Regexs::ANY))
    throw std::runtime_error("The token \"" + token + "\" is of an incorrect form.");
}

/**
 * Convenience function that matches a *complete* given string to the given
 *regex.
 *
 * @param stringToMatch :: the string to match with the given regex.
 * @param regexString   :: the regex with which to match the given string.
 *
 * @returns true if the string matches fully, or false otherwise.
 */
bool matchesFully(const std::string &stringToMatch, const std::string &regexString, const bool caseless) {
  boost::regex regex;

  if (caseless)
    regex = boost::regex("^(" + regexString + "$)", boost::regex::icase);
  else
    regex = boost::regex("^(" + regexString + "$)");

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
std::string getMatchingString(const std::string &regexString, const std::string &toParse, const bool caseless) {
  boost::regex regex;
  if (caseless) {
    regex = boost::regex(regexString, boost::regex::icase);
  } else {
    regex = boost::regex(regexString);
  }

  boost::sregex_iterator it(toParse.begin(), toParse.end(), regex);

  if (it == boost::sregex_iterator())
    return "";

  return it->str();
}

/**
 * Zero pads the run number used in a file name to required length.
 *
 * @param run - the run number of the file.
 * @param instString - the name of the instrument
 *
 * @returns the string, padded to the required length.
 * @throws std::runtime_error if run is longer than size of count.
 */
std::string pad(const unsigned int run, const std::string &instString) {
  InstrumentInfo const instInfo = ConfigService::Instance().getInstrument(instString);
  std::string prefix;
  if (!instInfo.facility().noFilePrefix())
    prefix = instInfo.filePrefix(run) + instInfo.delimiter();
  unsigned int padLength = instInfo.zeroPadding(run);
  std::string runStr = std::to_string(run);
  if (runStr.size() < padLength)
    runStr.insert(0, padLength - runStr.size(), '0');
  else if (padLength > 0 && runStr.size() > padLength)
    throw std::runtime_error("Could not parse run number \"" + runStr +
                             "\" since the instrument run number length required is " + std::to_string(padLength));
  runStr.insert(0, prefix);
  return runStr;
}

/**
 * Overloaded function operators to be used by std::binary_search to test if a
 * range and a run
 * are "equivalent", which in this case we choose to mean whether or not the run
 * is
 * inside the range.
 */
bool RangeContainsRun::operator()(const std::pair<unsigned int, unsigned int> &range, const unsigned int run) {
  return range.second < run;
}
bool RangeContainsRun::operator()(const unsigned int run, const std::pair<unsigned int, unsigned int> &range) {
  return run < range.first;
}

/**
 * Function for use with std::accumulate, the goal of which is merge any ranges
 * that are adjacent.
 *
 * @param ranges :: the set of ranges that have been accumulated so far.
 * @param range  :: the range to add, or merge if it is adjacent
 *
 * @returns the original ranges, with the extra range added/merged.
 */
std::set<std::pair<unsigned int, unsigned int>>
mergeAdjacentRanges(std::set<std::pair<unsigned int, unsigned int>> ranges,
                    const std::pair<unsigned int, unsigned int> &range) {
  // If ranges is empty, just insert the new range.
  if (ranges.empty()) {
    ranges.insert(range);
  }
  // Else there are already some ranges present ...
  else {
    // ... if the last one is adjacent to the new range, merge the two.
    if (ranges.rbegin()->second + 1 == range.first) {
      unsigned int from = ranges.rbegin()->first;
      unsigned int to = range.second;
      std::pair<unsigned int, unsigned int> temp(from, to);

      ranges.erase(--ranges.end(), ranges.end());
      ranges.insert(temp);
    }
    // ... else just insert it.
    else {
      ranges.insert(range);
    }
  }
  return ranges;
}

/**
 * Function for use with std::accumulate.  Takes in a runRange, and appends its
 *details onto the
 * accumulated output string so far.
 *
 * @param output   :: the accumulate output so far.
 * @param runRange :: the range who's details should be appended.
 *
 * @returns the updated output
 */
std::string accumulateString(std::string output, std::pair<unsigned int, unsigned int> runRange) {
  if (!output.empty())
    output += "_and_";

  if (runRange.first == runRange.second)
    output += std::to_string(runRange.first);
  else
    output += std::to_string(runRange.first) + "_to_" + std::to_string(runRange.second);

  return output;
}

/**
 * Converts a RunRangeList object into a readable (and wsName-friendly) string.
 *
 * @param runRangeList :: the runRangeList to convert to a string
 *
 * @returns the converted string.
 */
std::string toString(const RunRangeList &runRangeList) {
  std::set<std::pair<unsigned int, unsigned int>> runRanges = runRangeList.rangeList();

  // For each run range (pair of unsigned ints), call accumulateString and
  // return the accumulated result.
  return std::accumulate(runRanges.begin(), runRanges.end(), std::string(), accumulateString);
}

} // anonymous namespace

} // namespace Mantid::Kernel::MultiFileNameParsing
