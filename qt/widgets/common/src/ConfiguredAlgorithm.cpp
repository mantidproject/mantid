// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/IAlgorithmRuntimeProps.h"

namespace MantidQt::API {

ConfiguredAlgorithm::ConfiguredAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                                         std::unique_ptr<MantidQt::API::IAlgorithmRuntimeProps> properties)
    : m_algorithm(std::move(algorithm)), m_properties(std::move(properties)) {}

Mantid::API::IAlgorithm_sptr ConfiguredAlgorithm::algorithm() const { return m_algorithm; }

const MantidQt::API::IAlgorithmRuntimeProps &ConfiguredAlgorithm::getAlgorithmRuntimeProps() const noexcept {
  return *m_properties;
}

} // namespace MantidQt::API
