#include "MantidKernel/StringTokenizer.h"
#include <algorithm>
#include <iterator> //cbegin,cend

namespace {

// implement our own trim function to avoid the locale overhead in boost::trim.

// trim from start
void trimTokenFromStart(std::string &s) {
  s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ::isspace));
}

// trim from end
void trimTokenFromEnd(std::string &s) {
  s.erase(std::find_if_not(s.rbegin(), s.rend(), ::isspace).base(), s.end());
}

// trim from both ends
void trimToken(std::string &s) {
  trimTokenFromStart(s);
  trimTokenFromEnd(s);
}

// If the final character is a separator, we need to add an empty string to
// tokens.
void addEmptyFinalToken(const std::string &str, const std::string &delims,
                        std::vector<std::string> &tokens) {

  const auto pos = std::find(delims.cbegin(), delims.cend(), str.back());

  if (pos != delims.cend()) {
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

std::vector<std::string>
splitKeepingWhitespaceEmptyTokens(const std::string &str,
                                  const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(str.cbegin(), str.cend(), delims.cbegin(), delims.cend(),
                 [&output](std::string::const_iterator first,
                           std::string::const_iterator second) {
                   output.emplace_back(first, second);
                 });
  return output;
}

std::vector<std::string>
splitKeepingWhitespaceIgnoringEmptyTokens(const std::string &str,
                                          const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(str.cbegin(), str.cend(), delims.cbegin(), delims.cend(),
                 [&output](std::string::const_iterator first,
                           std::string::const_iterator second) {
                   if (first != second)
                     output.emplace_back(first, second);
                 });
  return output;
}

std::vector<std::string>
splitIgnoringWhitespaceKeepingEmptyTokens(const std::string &str,
                                          const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(str.cbegin(), str.cend(), delims.cbegin(), delims.cend(),
                 [&output](std::string::const_iterator first,
                           std::string::const_iterator second) {
                   output.emplace_back(first, second);
                   trimToken(output.back());
                 });
  return output;
}

std::vector<std::string>
splitIgnoringWhitespaceEmptyTokens(const std::string &str,
                                   const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(str.cbegin(), str.cend(), delims.cbegin(), delims.cend(),
                 [&output](std::string::const_iterator first,
                           std::string::const_iterator second) {
                   if (first != second) {
                     output.emplace_back(first, second);
                     trimToken(output.back());
                     if (output.back().empty())
                       output.pop_back();
                   }
                 });
  return output;
}
} // namespace

/**
 * Constructor requiring a string to tokenize and a string of separators.
 * @param str Input string to be separated into tokens.
 * @param separators List of characters used to separate the input string.
 * @param options  tokenizer settings. The number can be found using the
 * StringTokenizer::Options enum
 * @throw Throws std::runtime_error if options > 7.
 * @return a const reference to the index'th token.
 */
Mantid::Kernel::StringTokenizer::StringTokenizer(const std::string &str,
                                                 const std::string &separators,
                                                 unsigned options) {

  // if str is empty, then there is no work to do. exit early.
  if (str.empty())
    return;

  // see comments above for the different options split0,split1,split2 and
  // split3 implement.
  // cases 0-3 will check for a separator in the last place and insert an empty
  // token at the end.
  // cases 4-7 will not check and ignore a potential empty token at the end.
  switch (options) {
  case 0:
    m_tokens = splitKeepingWhitespaceEmptyTokens(str, separators);
    addEmptyFinalToken(str, separators, m_tokens);
    return;
  case TOK_IGNORE_EMPTY:
    m_tokens = splitKeepingWhitespaceIgnoringEmptyTokens(str, separators);
    return;
  case TOK_TRIM:
    m_tokens = splitIgnoringWhitespaceKeepingEmptyTokens(str, separators);
    addEmptyFinalToken(str, separators, m_tokens);
    return;
  case (TOK_TRIM | TOK_IGNORE_EMPTY):
    m_tokens = splitIgnoringWhitespaceEmptyTokens(str, separators);
    return;
  case TOK_IGNORE_FINAL_EMPTY_TOKEN:
    m_tokens = splitKeepingWhitespaceEmptyTokens(str, separators);
    return;
  case (TOK_IGNORE_FINAL_EMPTY_TOKEN | TOK_IGNORE_EMPTY):
    m_tokens = splitKeepingWhitespaceIgnoringEmptyTokens(str, separators);
    return;
  case (TOK_IGNORE_FINAL_EMPTY_TOKEN | TOK_TRIM):
    m_tokens = splitIgnoringWhitespaceKeepingEmptyTokens(str, separators);
    return;
  case (TOK_IGNORE_FINAL_EMPTY_TOKEN | TOK_TRIM | TOK_IGNORE_EMPTY):
    m_tokens = splitIgnoringWhitespaceEmptyTokens(str, separators);
    return;
  }

  // This point is reached only if options > 7.
  throw std::runtime_error(
      "Invalid option passed to Mantid::Kernel::StringTokenizer:" +
      std::to_string(options));
}
