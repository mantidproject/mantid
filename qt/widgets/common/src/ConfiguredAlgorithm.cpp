// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"

namespace MantidQt::API {

ConfiguredAlgorithm::ConfiguredAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                                         std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> properties,
                                         bool const validatePropsPreExec)
    : m_algorithm(std::move(algorithm)), m_properties(std::move(properties)),
      m_validatePropsPreExec(validatePropsPreExec) {}

Mantid::API::IAlgorithm_sptr ConfiguredAlgorithm::algorithm() const { return m_algorithm; }

const Mantid::API::IAlgorithmRuntimeProps &ConfiguredAlgorithm::getAlgorithmRuntimeProps() const noexcept {
  return *m_properties;
}

bool ConfiguredAlgorithm::validatePropsPreExec() const noexcept { return m_validatePropsPreExec; }

} // namespace MantidQt::API
