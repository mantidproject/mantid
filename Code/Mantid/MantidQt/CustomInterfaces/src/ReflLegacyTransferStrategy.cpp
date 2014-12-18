#include "MantidQtCustomInterfaces/ReflLegacyTransferStrategy.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    std::vector<std::map<std::string,std::string> > ReflLegacyTransferStrategy::transferRuns(const std::map<std::string,std::string>& runRows)
    {
      /*
       * If the descriptions are the same except for theta: same group, different rows.
       * If the descriptions are the same including theta: same row with runs separated by '+'
       * We always prefill theta if we can.
       */

      //maps descriptions to runs. Multiple runs are joined with '+'
      std::map<std::string,std::string> runsByDesc;
      //Counter used to feed fresh group ids
      int nextGroupId = 0;
      //maps a description to a group. If descriptions only differ by theta, they'll share a group
      std::map<std::string,std::string> groupsByDesc;
      //maps descriptions to the value of theta they contain
      std::map<std::string,std::string> thetaByDesc;

      //Iterate over the input and build the maps
      for(auto rowIt = runRows.begin(); rowIt != runRows.end(); ++rowIt)
      {
        const std::string run = rowIt->first;
        const std::string desc = rowIt->second;
        std::string cleanDesc = desc;

        //See if theta is in the description
        static boost::regex regexTheta("(?|th[:=](?<theta>[0-9.]+)|in (?<theta>[0-9.]+) theta)");
        boost::smatch matches;
        if(boost::regex_search(desc, matches, regexTheta))
        {
          //We have theta. Let's get a clean description
          size_t matchOffset = matches.position("theta");
          const std::string theta = matches["theta"].str();
          const std::string descPreTheta = desc.substr(0, matchOffset);
          const std::string descPostTheta = desc.substr(matchOffset + theta.length(), std::string::npos);
          cleanDesc = descPreTheta + "?" + descPostTheta;
          thetaByDesc[desc] = theta;
        }

        //map the description to the run, making sure to join with a + if one already exists
        const std::string prevRun = runsByDesc[desc];
        if(prevRun.empty())
          runsByDesc[desc] = run;
        else
          runsByDesc[desc] = prevRun + "+" + run;

        //If there isn't a group for this description (ignoring differences in theta) yet, make one
        if(groupsByDesc[cleanDesc].empty())
          groupsByDesc[cleanDesc] = boost::lexical_cast<std::string>(nextGroupId++);

        //Assign this description to the group it belongs to
        groupsByDesc[desc] = groupsByDesc[cleanDesc];
      }

      //All the data we need is now properly organised, so we can quickly throw out the rows needed
      std::vector<std::map<std::string,std::string> > output;
      for(auto run = runsByDesc.begin(); run != runsByDesc.end(); ++run)
      {
        std::map<std::string,std::string> row;
        row["runs"] = run->second;
        row["theta"] = thetaByDesc[run->first];
        row["group"] = groupsByDesc[run->first];
        output.push_back(row);
      }
      std::sort(output.begin(), output.end());
      return output;
    }
  }
}
