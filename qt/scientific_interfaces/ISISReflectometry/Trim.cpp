#include "Trim.h"
#include <algorithm>
#include <cctype>
#include <locale>

namespace MantidQt {
namespace CustomInterfaces {
void trimLeft(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                  [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
void trimRight(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string &s) {
  trimLeft(s);
  trimRight(s);
}
}
}
