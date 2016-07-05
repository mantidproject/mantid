#include "MantidQtMantidWidgets/DataProcessorUI/ParseKeyValueString.h"
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
/**
    Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'`
   into a map of key/value pairs
    @param str The input string
    @throws std::runtime_error on an invalid input string
    */
std::map<std::string, std::string> parseKeyValueString(const std::string &str) {
  /*
    This is a bad example of using a tokenizer, and
    Mantid::Kernel::StringTokenizer should
    ideally be used for this (see LoadProcessedNexus, Fit1D or others for
    examples)

    The reason we must use boost::tokenizer here is that passing a list of
    separators is not
    yet possible with Mantid::Kernel::StringTokenizer.
  */
  boost::tokenizer<boost::escaped_list_separator<char>> tok(
      str, boost::escaped_list_separator<char>("\\", ",", "\"'"));
  std::map<std::string, std::string> kvp;

  for (const auto &it : tok) {
    std::vector<std::string> valVec;
    boost::split(valVec, it, boost::is_any_of("="));

    if (valVec.size() > 1) {
      // We split on all '='s. The first delimits the key, the rest are assumed
      // to be part of the value
      std::string key = valVec[0];
      // Drop the key from the values vector
      valVec.begin() = valVec.erase(valVec.begin());
      // Join the remaining sections,
      std::string value = boost::algorithm::join(valVec, "=");

      // Remove any unwanted whitespace
      boost::trim(key);
      boost::trim(value);

      if (key.empty() || value.empty())
        throw std::runtime_error("Invalid key value pair, '" + it + "'");

      kvp[key] = value;
    } else {
      throw std::runtime_error("Invalid key value pair, '" + it + "'");
    }
  }
  return kvp;
}
}
}