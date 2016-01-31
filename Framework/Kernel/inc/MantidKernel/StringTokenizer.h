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
    TOK_TRIM = 2          /// remove leading and trailing whitespace from tokens
  };

  typedef std::vector<std::string> TokenVec;
  typedef Poco::StringTokenizer::Iterator Iterator;
  StringTokenizer() = default;
  StringTokenizer(const std::string &str, const std::string &separators,
                  int options = 0);
  ~StringTokenizer() = default;
  /// Destroys the tokenizer.

  Iterator begin() const { return m_tokens.begin(); };
  Iterator end() const { return m_tokens.end(); };

const std::string &operator[](std::size_t index) const {
  return m_tokens[index];
};
/// Returns const reference the index'th token.
/// Throws a RangeException if the index is out of range.

std::string &operator[](std::size_t index) { return m_tokens[index]; };
/// Returns reference to the index'th token.
/// Throws a RangeException if the index is out of range.

bool has(const std::string &token) const {
  return std::find(m_tokens.begin(), m_tokens.end(), token) != m_tokens.end();
};
/// Returns true if token exists, false otherwise.

/*std::size_t find(const std::string &token, std::size_t pos = 0) const {
  return
std::distance(m_tokens.begin(),std::find(m_tokens.begin()+pos,m_tokens.end(),token));
};*/
/// Returns the index of the first occurrence of the token
/// starting at position pos.
/// Throws a NotFoundException if the token is not found.

std::size_t count() const { return m_tokens.size(); };
/// Returns the total number of tokens.

/*std::size_t count(const std::string &token) const {
  return std::count(m_tokens.begin(),m_tokens.end(),token);
};
/// Returns the number of tokens equal to the specified token.
*/
private:
  std::vector<std::string> m_tokens;
};
}
}

#endif /* StringTokenizer_h */
