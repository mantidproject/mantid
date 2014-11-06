#include "MantidQtCustomInterfaces/ReflLegacyTransferStrategy.h"

#include <boost/regex.hpp>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    std::vector<std::map<std::string,std::string> > ReflLegacyTransferStrategy::transferRuns(const std::map<std::string,std::string>& runRows)
    {
      std::vector<std::map<std::string,std::string> > output;

      //Iterate over the input map
      for(auto rowIt = runRows.begin(); rowIt != runRows.end(); ++rowIt)
      {
        const std::string& run = rowIt->first;
        const std::string& desc = rowIt->second;

        std::map<std::string,std::string> newRow;

        newRow["runs"] = run;
        newRow["group"] = "0";

        static boost::regex shortTheta("th=([0-9.]+)");
        static boost::regex longTheta("in ([0-9.]+) theta");
        boost::smatch matches;
        if(boost::regex_search(desc, matches, shortTheta)
        || boost::regex_search(desc, matches, longTheta))
          newRow["theta"] = matches[1].str();

        output.push_back(newRow);
      }

      return output;
    }
  }
}
