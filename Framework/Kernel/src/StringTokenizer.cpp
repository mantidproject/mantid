//
//  StringTokenizer.cpp
//  Mantid
//
//  Created by Hahn, Steven E. on 1/31/16.
//
//

#include "MantidKernel/StringTokenizer.h"
#include <algorithm>
#include <functional>
#include <cassert>

namespace {

// implement our own trim function to avoid the locale overhead in boost::trim.

// trim from start
void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ::isspace));
}

// trim from end
void rtrim(std::string &s) {
  s.erase(std::find_if_not(s.rbegin(), s.rend(), ::isspace).base(), s.end());
}

// trim from both ends
void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

// If the final character is a separator, we need to add an empty string to
// tokens.
void emptyFinalToken(const std::string &str, const std::string &delims,
                     std::vector<std::string> &tokens) {
  const auto pos = std::find_first_of(str.end() - 1, str.end(), delims.begin(),
                                      delims.end());
  if (pos != str.end()) {
    tokens.emplace_back();
  }
}

// generic tokenizer using std::find_first_of modelled after
// http://tcbrindle.github.io/posts/a-quicker-study-on-tokenising/
// MIT licensed.
template <class InputIt, class ForwardIt, class BinOp>
void for_each_token(InputIt first, InputIt last, ForwardIt s_first,
                    ForwardIt s_last, BinOp binary_op) {
  while (first != last) {
    const auto pos = std::find_first_of(first, last, s_first, s_last);
    binary_op(first, pos);
    if (pos == last)
      break;
    first = std::next(pos);
  }
}

// split keeping whitespace and empty tokens.
std::vector<std::string> split0(const std::string &str,
                                const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(cbegin(str), cend(str), cbegin(delims), cend(delims),
                 [&output](auto first, auto second) {
                   output.emplace_back(first, second);
                 });
  return output;
}

// split keeping whitespace and ignoring empty tokens.
std::vector<std::string> split1(const std::string &str,
                                const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(cbegin(str), cend(str), cbegin(delims), cend(delims),
                 [&output](auto first, auto second) {
                   if (first != second)
                     output.emplace_back(first, second);
                 });
  return output;
}

// split trimming whitespace and keeping empty tokens.
std::vector<std::string> split2(const std::string &str,
                                const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(cbegin(str), cend(str), cbegin(delims), cend(delims),
                 [&output](auto first, auto second) {
                   output.emplace_back(first, second);
                   trim(output.back());
                 });
  return output;
}

// split trimming whitespace and ignoring empty tokens.
std::vector<std::string> split3(const std::string &str,
                                const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(cbegin(str), cend(str), cbegin(delims), cend(delims),
                 [&output](auto first, auto second) {
                   if (first != second) {
                     output.emplace_back(first, second);
                     trim(output.back());
                     if (output.back().empty())
                       output.pop_back();
                   }
                 });
  return output;
}
}

/**
 * Constructor requiring a string to tokenize and a string of separators.
 * @param str Input string to be separated into tokens.
 * @param separators List of characters used to separate the input string.
 * @param options  tokenizer settings. The number can be found using the
 * StringTokenizer::Options enum
 * @return a const reference to the index'th token.
 */
Mantid::Kernel::StringTokenizer::StringTokenizer(const std::string &str,
                                                 const std::string &separators,
                                                 unsigned options) {
  // check options variable is in the range 0-3.
  assert(options < 8);
  // if str is empty, then there is no workt to do. exit early.
  if (str.empty())
    return;

  // see comments above for the different options split0,split1,split2 and
  // split3 implement.
  // cases 0-3 will check for a separator in the last place and insert an empty
  // token at the end.
  // cases 4-7 will not check and ignore a potential empty token at the end.
  switch (options) {
  case 0:
    m_tokens = split0(str, separators);
    emptyFinalToken(str, separators, m_tokens);
    break;
  case 1:
    m_tokens = split1(str, separators);
    break;
  case 2:
    m_tokens = split2(str, separators);
    emptyFinalToken(str, separators, m_tokens);
    break;
  case 3:
    m_tokens = split3(str, separators);
    break;
  case 4:
    m_tokens = split0(str, separators);
    break;
  case 5:
    m_tokens = split1(str, separators);
    break;
  case 6:
    m_tokens = split2(str, separators);
    break;
  case 7:
    m_tokens = split3(str, separators);
    break;
  default:
    throw std::runtime_error(
        "Invalid option passed to Mantid::Kernel::StringTokenizer:" +
        std::to_string(options));
    break;
  }
};
