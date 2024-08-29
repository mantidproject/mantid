// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/System.h"

#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#endif
#include <iosfwd>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

/** Holds support functions for strings.
 */

namespace Strings {

//------------------------------------------------------------------------------------------------
/** Join a set or vector of (something that turns into a string) together
 * into one string, separated by a string.
 * Returns an empty string if the range is null.
 * Does not add the separator after the LAST item.
 *
 * For example, join a vector of strings with commas with:
 *  out = join(v.begin(), v.end(), ", ");
 *
 * This is a simple default version that works in all cases but is potentially
 * slow.
 *
 * @param begin :: iterator at the start
 * @param end :: iterator at the end
 * @param separator :: string to append.
 * @return
 */
template <typename ITERATOR_TYPE>
DLLExport std::string simpleJoin(ITERATOR_TYPE begin, ITERATOR_TYPE end, const std::string &separator) {
  std::ostringstream output;
  ITERATOR_TYPE it;
  for (it = begin; it != end;) {
    output << *it;
    it++;
    if (it != end)
      output << separator;
  }
  return output.str();
}

//------------------------------------------------------------------------------------------------
/** Join a set or vector of (something that turns into a string) together
 * into one string, separated by a string.
 * Returns an empty string if the range is null.
 * Does not add the separator after the LAST item.
 *
 * For example, join a vector of strings with commas with:
 *  out = join(v.begin(), v.end(), ", ");
 *
 * This version is used for random access iterators (e.g. map, set), and
 * it calls simpleJoin().
 *
 * @param begin :: iterator at the start
 * @param end :: iterator at the end
 * @param separator :: string to append.
 * @return
 */
template <typename ITERATOR_TYPE>
DLLExport std::string
join(ITERATOR_TYPE begin, ITERATOR_TYPE end, const std::string &separator,
     typename std::enable_if<!(std::is_same<typename std::iterator_traits<ITERATOR_TYPE>::iterator_category,
                                            std::random_access_iterator_tag>::value)>::type * = nullptr) {
  return simpleJoin(begin, end, separator);
}

//------------------------------------------------------------------------------------------------
/** Join a set or vector of (something that turns into a string) together
 * into one string, separated by a string.
 * Returns an empty string if the range is null.
 * Does not add the separator after the LAST item.
 *
 * For example, join a vector of strings with commas with:
 *  out = join(v.begin(), v.end(), ", ");
 *
 * This is a faster threaded version of the join() function above.
 * It is used only if the iterators are not random access (e.g. vector), as it
 * needs to be able to determine the distance between begin and end.
 * It reverts to calling simpleJoin() if the input array is small.
 *
 * @param begin :: iterator at the start
 * @param end :: iterator at the end
 * @param separator :: string to append.
 * @return
 */
template <typename ITERATOR_TYPE>
DLLExport std::string
join(ITERATOR_TYPE begin, ITERATOR_TYPE end, const std::string &separator,
     typename std::enable_if<(std::is_same<typename std::iterator_traits<ITERATOR_TYPE>::iterator_category,
                                           std::random_access_iterator_tag>::value)>::type * = nullptr) {

  // Get max number of threads
  int nmaxThreads = static_cast<int>(PARALLEL_GET_MAX_THREADS);

  // Define minimum size for using threading
  int min_size = 500 * nmaxThreads;

  // Get the distance between begining and end
  int dist = static_cast<int>(std::distance(begin, end));

  if (dist < min_size) {

    // If the input array is small, use the simpler function to avoid
    // unnecessary overhead from generating the parallel section
    return simpleJoin(begin, end, separator);

  } else {

    // Allocate vector space
    std::vector<std::string> output(nmaxThreads);
    size_t stream_size = 0;

    // Actual number of threads in the current region
    int nThreads = 1;
#pragma omp parallel reduction(+ : stream_size)
    {
      nThreads = static_cast<int>(PARALLEL_NUMBER_OF_THREADS);
      int idThread = static_cast<int>(PARALLEL_THREAD_NUMBER);

      // Initialise ostringstream
      std::ostringstream thread_stream;

/* To make sure the loop is done in the right order, we use schedule(static).

   From the OpenMP documentation:
   "When schedule(static, chunk_size) is specified, iterations are divided into
   chunks of size chunk_size, and the chunks are assigned to the threads in the
   team in a round-robin fashion **in the order of the thread number**."

   "When no chunk_size is specified, the iteration space is divided into chunks
   that are approximately equal in size, and at most one chunk is distributed
   to each thread."
*/
#pragma omp for schedule(static)
      for (int i = 0; i < dist; i++) {
        thread_stream << separator << *(begin + i);
      }
      output[idThread] = thread_stream.str();
      stream_size += output[idThread].length();
    }

    // Reserve space in memory for output string
    std::string master_string = output[0].erase(0, separator.length());
    master_string.reserve(stream_size - separator.length());

    // Concatenate the contributions from the remaning threads
    for (int i = 1; i < nThreads; i++) {
      master_string += output[i];
    }

    return master_string;
  }
}

//------------------------------------------------------------------------------------------------
/** Join a set or vector of (something that turns into a string) together
 * into one string, separated by a separator,
 * adjacent items that are precisely 1 away from each other
 * will be compressed into a list syntax e.g. 1-5.
 * Returns an empty string if the range is null.
 * Does not add the separator after the LAST item.
 *
 * For example, join a vector of strings with commas with:
 *  out = join(v.begin(), v.end(), ", ");
 *
 * @param begin :: iterator at the start
 * @param end :: iterator at the end
 * @param separator :: string to append between items.
 * @param listSeparator :: string to append between list items.
 * @return A string with contiguous values compressed using the list syntax
 */
template <typename ITERATOR_TYPE>
DLLExport std::string joinCompress(ITERATOR_TYPE begin, ITERATOR_TYPE end, const std::string &separator = ",",
                                   const std::string &listSeparator = "-") {

  if (begin == end) {
    return "";
  }
  std::stringstream result;

  ITERATOR_TYPE i = begin;
  // Always include the first value
  result << *begin;
  // move on to the next value
  ITERATOR_TYPE previousValue = i;
  ++i;

  std::string currentSeparator = separator;
  for (; i != end; ++i) {
    // if it is one higher than the last value
    if (*i == (*previousValue + 1)) {
      currentSeparator = listSeparator;
    } else {
      if (currentSeparator == listSeparator) {
        // add the last value that was the end of the list
        result << currentSeparator;
        result << *previousValue;
        currentSeparator = separator;
      }
      // add the current value
      result << currentSeparator;
      result << *i;
    }
    previousValue = i;
  }
  // if we have got to the end and part of a list output the last value
  if (currentSeparator == listSeparator) {
    result << currentSeparator;
    result << *previousValue;
  }
  return result.str();
}
/// Converts long strings into "start ... end"
MANTID_KERNEL_DLL std::string shorten(const std::string &input, const size_t max_length);

/// Return a string with all matching occurence-strings
MANTID_KERNEL_DLL std::string replace(const std::string &input, const std::string &find_what,
                                      const std::string &replace_with);
/// Return a string with all occurrences of the characters in the input replaced
/// by the replace string
MANTID_KERNEL_DLL std::string replaceAll(const std::string &input, const std::string &charStr,
                                         const std::string &substitute);

/// Converts string to all lowercase
MANTID_KERNEL_DLL std::string toLower(const std::string &input);

/// Converts string to all uppercase
MANTID_KERNEL_DLL std::string toUpper(const std::string &input);

/// determine if a character group exists in a string
MANTID_KERNEL_DLL int confirmStr(const std::string &S, const std::string &fullPhrase);
/// Get a word from a string
MANTID_KERNEL_DLL int extractWord(std::string &Line, const std::string &Word, const int cnt = 4);
/// Get an int from the end of a word
MANTID_KERNEL_DLL int endsWithInt(const std::string &word);

/// strip all spaces
MANTID_KERNEL_DLL std::string removeSpace(const std::string &CLine);
/// strip pre/post spaces
MANTID_KERNEL_DLL std::string fullBlock(const std::string &A);
/// strip pre/post spaces
MANTID_KERNEL_DLL std::string strip(const std::string &A);
/// strip trailling comments
MANTID_KERNEL_DLL void stripComment(std::string &A);
/// Determines if a string is only spaces
MANTID_KERNEL_DLL int isEmpty(const std::string &A);
/// Determines if a string starts with a #
MANTID_KERNEL_DLL bool skipLine(const std::string &line);
/// Get a line and strip comments
/// Use only for a single call
MANTID_KERNEL_DLL std::string getLine(std::istream &fh);
/// Get a line and strip comments
/// Use within a loop
MANTID_KERNEL_DLL void getLine(std::istream &fh, std::string &Line);
/// Peek at a line without extracting it from the stream
MANTID_KERNEL_DLL std::string peekLine(std::istream &fh);
/// get a part of a long line
MANTID_KERNEL_DLL int getPartLine(std::istream &fh, std::string &Out, std::string &Excess, const int spc = 256);

/// Takes a character string and evaluates the first [typename T] object
template <typename T> int convPartNum(const std::string &A, T &out);

/// Convert a string into a number
template <typename T> int convert(const std::string &A, T &out);
/// Convert a char* into a number
template <typename T> int convert(const char *A, T &out);

/// Convert a number to a string
template <typename T> std::string toString(const T &value);

/// Convert a vector to a string
template <typename T> std::string toString(const std::vector<T> &value);

/// Convert a set to a string
template <typename T> std::string toString(const std::set<T> &value);

template <typename T> int setValues(const std::string &Line, const std::vector<int> &Index, std::vector<T> &Out);

/// Convert and cut a string
template <typename T> int sectPartNum(std::string &A, T &out);

/// Convert and cut a string
template <typename T> int section(std::string &A, T &out);
/// Convert and cut a char*
template <typename T> int section(char *cA, T &out);

/// Convert and cut a string for MCNPX
template <typename T> int sectionMCNPX(std::string &A, T &out);

/// Write file in standard MCNPX input form
MANTID_KERNEL_DLL void writeMCNPX(const std::string &Line, std::ostream &OX);

/// Split tring into spc deliminated components
MANTID_KERNEL_DLL std::vector<std::string> StrParts(const std::string &Ln);

/// Splits a string into key value pairs
MANTID_KERNEL_DLL std::map<std::string, std::string>
splitToKeyValues(const std::string &input, const std::string &keyValSep = "=", const std::string &listSep = ",");

/// Write a set of containers to a file
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const T &step, const V<T, A> &Y);
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const V<T, A> &X, const V<T, A> &Y);
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const V<T, A> &X, const V<T, A> &Y, const V<T, A> &Err);

/// Convert a VAX number to x86 little eindien
float getVAXnum(const float A);

/// Eat everything from the stream until the next EOL
MANTID_KERNEL_DLL void readToEndOfLine(std::istream &in, bool ConsumeEOL);
/// Returns the next word in the stream
MANTID_KERNEL_DLL std::string getWord(std::istream &in, bool consumeEOL);
///  function parses a path, found in input string "path" and returns vector of
///  the folders contributed into the path */
MANTID_KERNEL_DLL size_t split_path(const std::string &path, std::vector<std::string> &path_components);

/// Loads the entire contents of a text file into a string
MANTID_KERNEL_DLL std::string loadFile(const std::string &filename);

/// checks if the candidate is the member of the group
MANTID_KERNEL_DLL int isMember(const std::vector<std::string> &group, const std::string &candidate);

/// Parses a number range, e.g. "1,4-9,54-111,3,10", to the vector containing
/// all the elements within the range
MANTID_KERNEL_DLL std::vector<int> parseRange(const std::string &str, const std::string &elemSep = ",",
                                              const std::string &rangeSep = "-");

/// Parses unsigned integer groups, e.g. "1+2,4-7,9,11" to a nested vector
/// structure.
template <typename Integer> std::vector<std::vector<Integer>> parseGroups(const std::string &str) {
  std::vector<std::vector<Integer>> groups;

  // Local helper functions.
  auto translateAdd = [&groups](const std::string &str) {
    const auto tokens = Kernel::StringTokenizer(
        str, "+", Kernel::StringTokenizer::TOK_TRIM | Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    std::vector<Integer> currentGroup;
    currentGroup.reserve(tokens.count());
    std::transform(tokens.cbegin(), tokens.cend(), std::back_inserter(currentGroup),
                   [](const auto &t) { return boost::lexical_cast<Integer>(t); });
    groups.emplace_back(std::move(currentGroup));
  };

  auto translateSumRange = [&groups](const std::string &str) {
    // add a group with the numbers in the range
    const auto tokens = Kernel::StringTokenizer(
        str, "-", Kernel::StringTokenizer::TOK_TRIM | Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    if (tokens.count() != 2)
      throw std::runtime_error("Malformed range (-) operation.");
    Integer first = boost::lexical_cast<Integer>(tokens[0]);
    Integer last = boost::lexical_cast<Integer>(tokens[1]);
    if (first > last)
      std::swap(first, last);
    // add all the numbers in the range to the output group
    std::vector<Integer> group;
    group.reserve(last - first + 1);
    for (Integer i = first; i <= last; ++i)
      group.emplace_back(i);
    if (!group.empty())
      groups.emplace_back(std::move(group));
  };

  auto translateRange = [&groups](const std::string &str) {
    // add a group per number
    const auto tokens = Kernel::StringTokenizer(
        str, ":", Kernel::StringTokenizer::TOK_TRIM | Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
    if (tokens.count() != 2)
      throw std::runtime_error("Malformed range (:) operation.");
    Integer first = boost::lexical_cast<Integer>(tokens[0]);
    Integer last = boost::lexical_cast<Integer>(tokens[1]);
    if (first > last)
      std::swap(first, last);
    // add all the numbers in the range to separate output groups
    for (Integer i = first; i <= last; ++i) {
      groups.emplace_back(1, i);
    }
  };

  try {
    // split into comma separated groups, each group potentially containing
    // an operation (+-:) that produces even more groups.
    const auto tokens = StringTokenizer(str, ",", StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);
    for (const auto &token : tokens) {
      // Look for the various operators in the string. If one is found then
      // do the necessary translation into groupings.
      if (token.find('+') != std::string::npos) {
        translateAdd(token);
      } else if (token.find('-') != std::string::npos) {
        translateSumRange(token);
      } else if (token.find(':') != std::string::npos) {
        translateRange(token);
      } else if (!token.empty()) {
        // contains a single number, just add it as a new group
        groups.emplace_back(1, boost::lexical_cast<Integer>(token));
      }
    }
  } catch (boost::bad_lexical_cast &) {
    throw std::runtime_error("Cannot parse numbers from string: '" + str + "'");
  }

  return groups;
}

/**
 * Generates random alpha-numeric string.
 * @param len :: Length of the string
 * @return Random string of the given length
 */
MANTID_KERNEL_DLL std::string randomString(const size_t len);

/// Extract a line from input stream, discarding any EOL characters encountered
MANTID_KERNEL_DLL std::istream &extractToEOL(std::istream &is, std::string &str);

} // NAMESPACE Strings

} // NAMESPACE Kernel

} // NAMESPACE Mantid
