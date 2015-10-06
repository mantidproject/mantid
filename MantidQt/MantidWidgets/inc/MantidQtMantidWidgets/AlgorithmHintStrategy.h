#ifndef MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H
#define MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H

#include "MantidAPI/IAlgorithm.h"
#include "MantidQtMantidWidgets/HintStrategy.h"

using namespace Mantid::API;

namespace MantidQt
{
  namespace MantidWidgets
  {
    /** AlgorithmHintStrategy : Produces hints using a given algorithm's properties.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
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
