#ifndef MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H
#define MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H

#include "MantidAPI/IAlgorithm.h"
#include "MantidQtMantidWidgets/HintStrategy.h"

using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class AlgorithmHintStrategy : public HintStrategy
    {
    public:
      AlgorithmHintStrategy(IAlgorithm_sptr algorithm, std::set<std::string> blacklist) : m_algorithm(algorithm), m_blacklist(blacklist)
      {
      }

      virtual ~AlgorithmHintStrategy() {};

      virtual std::map<std::string,std::string> createHints()
      {
        std::map<std::string,std::string> hints;

        auto properties = m_algorithm->getProperties();
        for(auto it = properties.begin(); it != properties.end(); ++it)
        {
          const std::string name = (*it)->name();

          //If it's not in the blacklist, add the property to our hints
          if(m_blacklist.find(name) == m_blacklist.end())
            hints[name] = (*it)->briefDocumentation();
        }

        return hints;
      }
    private:
      IAlgorithm_sptr m_algorithm;
      std::set<std::string> m_blacklist;
    };
  }
}

#endif /* MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H */
