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

namespace {

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

std::vector<std::string> split(const std::string &str,
                               const std::string &delims) {
  std::vector<std::string> output;
  for_each_token(cbegin(str), cend(str), cbegin(delims), cend(delims),
                 [&output](auto first, auto second) {
                   output.emplace_back(first, second);
                 });

  if (!str.empty()) {
    const auto pos = std::find_first_of(str.end() - 1, str.end(),
                                        delims.begin(), delims.end());
    if (pos != str.end()) {
      output.emplace_back();
    }
  }
  return output;
}
}
Mantid::Kernel::StringTokenizer::StringTokenizer(const std::string &str,
                                                 const std::string &separators,
                                                 int options) {

  m_tokens = split(str, separators);

  if (options == 2 || options == 3) {

    for (auto &token : m_tokens) {
      trim(token);
    }
  }

  if (options == 1 || options == 3) {
    m_tokens.erase(
        std::remove_if(m_tokens.begin(), m_tokens.end(),
                       [](std::string &token) { return token.empty(); }),
        m_tokens.end());
  }
};
