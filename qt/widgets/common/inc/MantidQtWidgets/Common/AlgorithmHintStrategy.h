#ifndef MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H
#define MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/HintStrategy.h"

namespace MantidQt {
namespace MantidWidgets {
/** AlgorithmHintStrategy : Produces hints using a given algorithm's properties.

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class AlgorithmHintStrategy : public HintStrategy {
public:
  AlgorithmHintStrategy(Mantid::API::IAlgorithm_sptr algorithm,
                        std::vector<std::string> blacklist)
      : m_algorithm(algorithm), m_blacklist(blacklist) {}

  AlgorithmHintStrategy(std::string const& algorithmName,
                        std::vector<std::string> blacklist)
      : m_algorithm(Mantid::API::AlgorithmManager::Instance().create(algorithmName)), m_blacklist(std::move(blacklist)) {}

  bool isBlacklisted(std::string const &propertyName) {
    return std::find(m_blacklist.cbegin(), m_blacklist.cend(), propertyName) !=
           m_blacklist.cend();
  }

  std::vector<Hint> createHints() override {
    std::vector<Hint> hints;
    auto properties = m_algorithm->getProperties();
    properties.erase(
        std::remove_if(properties.begin(), properties.end(),
                       [this](Mantid::Kernel::Property *property)
                           -> bool { return isBlacklisted(property->name()); }),
        properties.end());
    hints.reserve(properties.size());
    std::transform(
        properties.cbegin(), properties.cend(), std::back_inserter(hints),
        [](Mantid::Kernel::Property *property) -> Hint {
          return Hint(property->name(), property->briefDocumentation());
        });

    return hints;
  }

private:
  Mantid::API::IAlgorithm_sptr m_algorithm;
  std::vector<std::string> m_blacklist;
};
}
}

#endif /* MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGY_H */
