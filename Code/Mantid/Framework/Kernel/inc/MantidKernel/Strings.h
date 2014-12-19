#ifndef MANTID_KERNEL_STRINGS_H
#define MANTID_KERNEL_STRINGS_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <iosfwd>
#include <set>
#include <sstream>
#include <vector>
#include <map>
#include <iterator>

namespace Mantid {
namespace Kernel {

/** Holds support functions for strings.

Copyright & copy; 2007-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
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
 * @param begin :: iterator at the start
 * @param end :: iterator at the end
 * @param separator :: string to append.
 * @return
 */
template <typename ITERATOR_TYPE>
DLLExport std::string join(ITERATOR_TYPE begin, ITERATOR_TYPE end,
                           const std::string separator) {
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

/// Return a string with all matching occurence-strings
MANTID_KERNEL_DLL std::string replace(const std::string &input,
                                      const std::string &find_what,
                                      const std::string &replace_with);
/// Return a string with all occurrences of the characters in the input replaced
/// by the replace string
MANTID_KERNEL_DLL std::string replaceAll(const std::string &input,
                                         const std::string &charStr,
                                         const std::string &substitute);

/// determine if a character group exists in a string
MANTID_KERNEL_DLL int confirmStr(const std::string &S,
                                 const std::string &fullPhrase);
/// Get a word from a string
MANTID_KERNEL_DLL int extractWord(std::string &Line, const std::string &Word,
                                  const int cnt = 4);
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
MANTID_KERNEL_DLL std::string getLine(std::istream &fh, const int spc = 256);
/// Peek at a line without extracting it from the stream
MANTID_KERNEL_DLL std::string peekLine(std::istream &fh);
/// get a part of a long line
MANTID_KERNEL_DLL int getPartLine(std::istream &fh, std::string &Out,
                                  std::string &Excess, const int spc = 256);

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

template <typename T>
int setValues(const std::string &Line, const std::vector<int> &Index,
              std::vector<T> &Out);

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
MANTID_KERNEL_DLL std::vector<std::string> StrParts(std::string Ln);

/// Splits a string into key value pairs
MANTID_KERNEL_DLL std::map<std::string, std::string>
splitToKeyValues(const std::string &input, const std::string &keyValSep = "=",
                 const std::string &listSep = ",");

/// Write a set of containers to a file
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const T &step, const V<T, A> &Y);
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const V<T, A> &X, const V<T, A> &Y);
template <template <typename T, typename A> class V, typename T, typename A>
int writeFile(const std::string &Fname, const V<T, A> &X, const V<T, A> &Y,
              const V<T, A> &Err);

/// Convert a VAX number to x86 little eindien
float getVAXnum(const float A);

/// Eat everything from the stream until the next EOL
MANTID_KERNEL_DLL void readToEndOfLine(std::istream &in, bool ConsumeEOL);
/// Returns the next word in the stream
MANTID_KERNEL_DLL std::string getWord(std::istream &in, bool consumeEOL);
///  function parses a path, found in input string "path" and returns vector of
///  the folders contributed into the path */
MANTID_KERNEL_DLL size_t split_path(const std::string &path,
                                    std::vector<std::string> &path_components);

/// Loads the entire contents of a text file into a string
MANTID_KERNEL_DLL std::string loadFile(const std::string &filename);

/// checks if the candidate is the member of the group
MANTID_KERNEL_DLL int isMember(const std::vector<std::string> &group,
                               const std::string &candidate);

/// Parses a number range, e.g. "1,4-9,54-111,3,10", to the vector containing
/// all the elements
/// within the range
MANTID_KERNEL_DLL std::vector<int>
parseRange(const std::string &str, const std::string &elemSep = ",",
           const std::string &rangeSep = "-");

/// Extract a line from input stream, discarding any EOL characters encountered
MANTID_KERNEL_DLL std::istream &extractToEOL(std::istream &is,
                                             std::string &str);

} // NAMESPACE Strings

} // NAMESPACE Kernel

} // NAMESPACE Mantid

#endif // MANTID_KERNEL_STRINGS_H
