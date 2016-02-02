//
//  StringTokenizer.h
//  Mantid
//
//  Created by Hahn, Steven E. on 1/29/16.
//
//

#ifndef StringTokenizer_h
#define StringTokenizer_h

#include <Poco/StringTokenizer.h>

namespace Mantid {
namespace Kernel {

class StringTokenizer
    /// A simple tokenizer that splits a string into
    /// tokens, which are separated by separator characters.
    /// An iterator is used to iterate over all tokens.
    {
public:
  enum Options {
    TOK_IGNORE_EMPTY = 1, /// ignore empty tokens
    TOK_TRIM = 2,         /// remove leading and trailing whitespace from tokens
    TOK_IGNORE_FINAL_EMPTY_TOKEN =
        4 // don't check if there is an empty token at the end.
  };

  typedef std::vector<std::string> TokenVec;
  typedef std::vector<std::string>::const_iterator Iterator;
  StringTokenizer() = default;
  StringTokenizer(const std::string &str, const std::string &separators,
                  unsigned options = 0);
  ~StringTokenizer() = default;
  /// Destroys the tokenizer.

  Iterator begin() const { return m_tokens.begin(); };
  Iterator end() const { return m_tokens.end(); };

const std::string &operator[](std::size_t index) const {
  return m_tokens[index];
};
/// Returns const reference the index'th token.

std::string &operator[](std::size_t index) { return m_tokens[index]; };
/// Returns reference to the index'th token.

const TokenVec &asVector() { return m_tokens; };
// Returns a vector of tokenized strings.

const std::string &at(std::size_t index) const { return m_tokens.at(index); };
/// Returns const reference the index'th token.
/// Throws a RangeException if the index is out of range.

std::string &at(std::size_t index) { return m_tokens.at(index); };
/// Returns reference to the index'th token.
/// Throws a RangeException if the index is out of range.

std::size_t count() const { return m_tokens.size(); };
/// Returns the total number of tokens.

private:
  std::vector<std::string> m_tokens;
};
}
}

#endif /* StringTokenizer_h */
