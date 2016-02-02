//
//  StringTokenizer.cpp
//  Mantid
//
//  Created by Hahn, Steven E. on 1/31/16.
//
//

#include "MantidKernel/StringTokenizer.h"
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iostream>

namespace {
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
                               const std::string &delims,
                               bool skip_empty = false) {
  std::vector<std::string> output;
  for_each_token(cbegin(str), cend(str), cbegin(delims), cend(delims),
                 [&output, skip_empty](auto first, auto second) {
                   if (first != second || !skip_empty) {
                     output.emplace_back(first, second);
                   }
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
      boost::trim(token);
    }
  }

  if (options == 1 || options == 3) {
    m_tokens.erase(std::remove_if(m_tokens.begin(), m_tokens.end(),
                                  [](std::string &token) {
                                    return token == std::string();
                                  }),
                   m_tokens.end());
  }
};
