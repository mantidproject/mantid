// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
 */

namespace Mantid {
namespace Kernel {

class DLLExport StringTokenizer final {
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

  /** Get the total number of tokens.
   * @return the total number of tokens.
   */
  std::size_t size() const noexcept { return m_tokens.size(); }

private:
  std::vector<std::string> m_tokens;
};
} // namespace Kernel
} // namespace Mantid

#endif /* StringTokenizer_h */
