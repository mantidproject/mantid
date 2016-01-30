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
#include "MantidKernel/make_unique.h"

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

  typedef Poco::StringTokenizer::TokenVec TokenVec;
  typedef Poco::StringTokenizer::Iterator Iterator;
  StringTokenizer(const std::string &str, const std::string &separators,
                  int options = 0){/*if(str.back() == ' '){
              std::string newstr(str,0,str.size()-1);
              m_tokenizer =
Mantid::Kernel::make_unique<Mantid::Kernel::StringTokenizer>(newstr,separators,options);
            }
            else*/ {
      m_tokenizer = Mantid::Kernel::make_unique<Poco::StringTokenizer>(
          str, separators, options);
}
};
/// Splits the given string into tokens. The tokens are expected to be
/// separated by one of the separator characters given in separators.
/// Additionally, options can be specified:
///   * TOK_IGNORE_EMPTY: empty tokens are ignored
///   * TOK_TRIM: trailing and leading whitespace is removed from tokens.

~StringTokenizer() = default;
/// Destroys the tokenizer.

Iterator begin() const { return m_tokenizer->begin(); };
Iterator end() const { return m_tokenizer->end(); };

const std::string &operator[](std::size_t index) const {
  return (*m_tokenizer)[index];
};
/// Returns const reference the index'th token.
/// Throws a RangeException if the index is out of range.

std::string &operator[](std::size_t index) { return (*m_tokenizer)[index]; };
/// Returns reference to the index'th token.
/// Throws a RangeException if the index is out of range.

bool has(const std::string &token) const { return m_tokenizer->has(token); };
/// Returns true if token exists, false otherwise.

std::size_t find(const std::string &token, std::size_t pos = 0) const {
  return m_tokenizer->find(token, pos);
};
/// Returns the index of the first occurrence of the token
/// starting at position pos.
/// Throws a NotFoundException if the token is not found.

std::size_t replace(const std::string &oldToken, const std::string &newToken,
                    std::size_t pos = 0) {
  return m_tokenizer->replace(oldToken, newToken, pos);
};
/// Starting at position pos, replaces all subsequent tokens having value
/// equal to oldToken with newToken.
/// Returns the number of modified tokens.

std::size_t count() const { return m_tokenizer->count(); };
/// Returns the total number of tokens.

std::size_t count(const std::string &token) const {
  return m_tokenizer->count(token);
};
/// Returns the number of tokens equal to the specified token.

private:
std::unique_ptr<Poco::StringTokenizer> m_tokenizer;
};
}
}

#endif /* StringTokenizer_h */
