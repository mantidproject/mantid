// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobAlgorithm.h"

#include <utility>

#include "MantidAPI/IAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;

BatchJobAlgorithm::BatchJobAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                                     MantidQt::API::ConfiguredAlgorithm::AlgorithmRuntimeProps properties,
                                     UpdateFunction updateFunction, Item *item)
    : ConfiguredAlgorithm(std::move(algorithm), std::move(properties)), m_item(item), m_updateFunction(updateFunction) {
}

Item *BatchJobAlgorithm::item() { return m_item; }

void BatchJobAlgorithm::updateItem() {
  if (m_item)
    m_updateFunction(m_algorithm, *m_item);
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
