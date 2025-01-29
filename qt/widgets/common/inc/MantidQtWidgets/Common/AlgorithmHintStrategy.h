// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <utility>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/HintStrategy.h"

namespace MantidQt {
namespace MantidWidgets {
/** AlgorithmHintStrategy : Produces hints using a given algorithm's properties.
 */
class AlgorithmHintStrategy : public HintStrategy {
public:
  AlgorithmHintStrategy(Mantid::API::IAlgorithm_sptr algorithm, std::vector<std::string> blacklist)
      : m_algorithm(std::move(algorithm)), m_blacklist(std::move(blacklist)) {}

  AlgorithmHintStrategy(std::string const &algorithmName, std::vector<std::string> blacklist)
      : m_algorithm(Mantid::API::AlgorithmManager::Instance().create(algorithmName)),
        m_blacklist(std::move(blacklist)) {}

  bool isBlacklisted(std::string const &propertyName) {
    return std::find(m_blacklist.cbegin(), m_blacklist.cend(), propertyName) != m_blacklist.cend();
  }

  std::vector<Hint> createHints() override {
    std::vector<Hint> hints;
    auto properties = m_algorithm->getProperties();
    properties.erase(std::remove_if(properties.begin(), properties.end(),
                                    [this](const Mantid::Kernel::Property *property) -> bool {
                                      return isBlacklisted(property->name());
                                    }),
                     properties.end());
    hints.reserve(properties.size());
    std::transform(properties.cbegin(), properties.cend(), std::back_inserter(hints),
                   [](const Mantid::Kernel::Property *property) -> Hint {
                     return Hint(property->name(), property->documentation());
                   });

    return hints;
  }

private:
  Mantid::API::IAlgorithm_sptr m_algorithm;
  std::vector<std::string> m_blacklist;
};
} // namespace MantidWidgets
} // namespace MantidQt
