#include "MantidQtCustomInterfaces/ParseKeyValueString.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>


namespace MantidQt {
  namespace CustomInterfaces {
/**
    Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'` into a map of key/value pairs
    @param str The input string
    @throws std::runtime_error on an invalid input string
    */
    std::map<std::string, std::string> parseKeyValueString(const std::string &str) {
      //Tokenise, using '\' as an escape character, ',' as a delimiter and " and ' as quote characters
      boost::tokenizer <boost::escaped_list_separator<char>> tok(str,
                                                                 boost::escaped_list_separator<char>("\\", ",", "\"'"));

      std::map<std::string, std::string> kvp;

      for (auto it = tok.begin(); it != tok.end(); ++it) {
        std::vector<std::string> valVec;
        boost::split(valVec, *it, boost::is_any_of("="));

        if (valVec.size() > 1) {
          //We split on all '='s. The first delimits the key, the rest are assumed to be part of the value
          std::string key = valVec[0];
          //Drop the key from the values vector
          valVec.erase(valVec.begin());
          //Join the remaining sections,
          std::string value = boost::algorithm::join(valVec, "=");

          //Remove any unwanted whitespace
          boost::trim(key);
          boost::trim(value);

          if (key.empty() || value.empty())
            throw std::runtime_error("Invalid key value pair, '" + *it + "'");


          kvp[key] = value;
        }
        else {
          throw std::runtime_error("Invalid key value pair, '" + *it + "'");
        }
      }
      return kvp;
    }
  }
}