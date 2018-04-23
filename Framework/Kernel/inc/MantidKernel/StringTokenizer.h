#ifndef MANTID_KERNEL_STRINGTOKENIZER_H_
#define MANTID_KERNEL_STRINGTOKENIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <cstddef> //std::size_t
#include <string>
#include <vector>

#include "MantidKernel/System.h" //DLLExport

/** StringTokenizer: A simple tokenizer that splits a string into tokens, which
 are separated by separator characters. An iterator or index can used to iterate
 over all tokens or the result returned as a std::vector<std::string>>

 Copyright © 2007-2011 ISIS Rutherford Appleton Laboratory, NScD Oak
 Ridge National Laboratory & European Spallation Source

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

 File change history is stored at:
 <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at <http://doxygen.mantidproject.org>
 */

namespace Mantid {
namespace Kernel {

class DLLExport StringTokenizer {
public:
  /// Specify tokenizer options. These can be combined using + or |.
  enum Options {
    TOK_IGNORE_EMPTY = 1, ///< ignore empty tokens
    TOK_TRIM = 2, ///< remove leading and trailing whitespace from tokens
    TOK_IGNORE_FINAL_EMPTY_TOKEN =
        4 ///< ignore an empty token at the end of the string.
  };
  using TokenVec = std::vector<std::string>;
  using Iterator = std::vector<std::string>::iterator;
  using ConstIterator = std::vector<std::string>::const_iterator;
  /// Constructs an object from an empty string.
  StringTokenizer() = default;
  /// Constructor requiring a string to tokenize and a string of separators.
  StringTokenizer(const std::string &str, const std::string &separators,
                  unsigned options = 0);

  /// Destroys the tokenizer.
  ~StringTokenizer() = default;

  /** Iterator referring to first element in the container.
  * @return an iterator referring to the first element in the container.
  */
  Iterator begin() { return m_tokens.begin(); }

  /** Iterator referring to the past-the-end element in the container.
   * @return an iterator referring to the past-the-end element in the container.
   */
  Iterator end() { return m_tokens.end(); }

  /** Const iterator referring to first element in the container.
   * @return a const iterator referring to the first element in the container.
   */
  ConstIterator begin() const { return m_tokens.cbegin(); }
  /** Const iterator referring to the past-the-end element in the container.
   * @return a const iterator referring to the past-the-end element in the
   * container.
   */
  ConstIterator end() const { return m_tokens.cend(); }

  /** Const iterator referring to first element in the container.
  * @return a const iterator referring to the first element in the container.
  */
  ConstIterator cbegin() const { return m_tokens.cbegin(); }
  /** Const iterator referring to the past-the-end element in the container.
   * @return a const iterator referring to the past-the-end element in the
   * container.
   */
  ConstIterator cend() const { return m_tokens.cend(); }

  /** Get a const reference to the index'th token. Indexing an out-of-range
   * element won't throw, but is otherwise undefined behavior.
   * @param index Index of the requested token.
   * @return a const reference to the index'th token.
   */
  const std::string &operator[](std::size_t index) const {
    return m_tokens[index];
  }

  /** Get a const reference to the index'th token. Indexing an out-of-range
  * element won't throw, but is otherwise undefined behavior.
  * @param index Index of the requested token.
  * @return a const reference to the index'th token.
  */
  std::string &operator[](std::size_t index) { return m_tokens[index]; }

  /// Returns a vector of tokenized strings.
  const TokenVec &asVector() { return m_tokens; }

  /** Get a const reference to the index'th token.
   * @param index Index of the requested token.
   * @return a const reference to the index'th token.
   * @throw Throws std::out_of_range if the index is out of range.
   */
  const std::string &at(std::size_t index) const { return m_tokens.at(index); }

  /** Get a reference to the index'th token.
   * @param index Index of the requested token.
   * @return a reference to the index'th token.
   * @throw Throws std::out_of_range if the index is out of range.
   */
  std::string &at(std::size_t index) { return m_tokens.at(index); }

  /** Get the total number of tokens.
   * @return the total number of tokens.
   */
  std::size_t count() const { return m_tokens.size(); }

private:
  std::vector<std::string> m_tokens;
};
}
}

#endif /* StringTokenizer_h */
