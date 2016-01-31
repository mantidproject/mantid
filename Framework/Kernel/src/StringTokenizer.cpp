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

Mantid::Kernel::StringTokenizer::StringTokenizer(const std::string &str,
                                                 const std::string &separators,
                                                 int options) {
  std::string newstring = str;

  //restore 1.47 behavior of removing the last token if it is empty.
  //remove last character iff it is a separator.
  if (!str.empty()) {
    std::string meow(str.end() - 1, str.end());
    if (std::find_first_of(separators.begin(), separators.end(), meow.begin(),
                           meow.end()) != separators.end()) {
      newstring.pop_back();
    }
  }

  // 1.47 doesn't 
  // limit whitespace to a single space
  if (separators.find(" ") != std::string::npos) {
    boost::trim_left(newstring);
    Poco::StringTokenizer preTokenizer(newstring, " ", TOK_TRIM);
    newstring = std::string();
    if (preTokenizer.count() > 0) {
      for (auto it = preTokenizer.begin(); it != preTokenizer.end() - 1; ++it) {
        newstring += *it + " ";
      }
      newstring += *(preTokenizer.end() - 1);
    }
  }

  //1.61 fails to find two tokens in str="2- " and sep="-".
  //To work around this, we do the trimming and removing empty tokens ourselves.
  Poco::StringTokenizer tokenizer(newstring, separators,0);
  m_tokens.insert(m_tokens.begin(), tokenizer.begin(), tokenizer.end());

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
