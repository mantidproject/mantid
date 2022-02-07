// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobAlgorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/IAlgorithmRuntimeProps.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;

BatchJobAlgorithm::BatchJobAlgorithm(Mantid::API::IAlgorithm_sptr algorithm,
                                     std::unique_ptr<MantidQt::API::IAlgorithmRuntimeProps> properties,
                                     UpdateFunction updateFunction, Item *item)
    : ConfiguredAlgorithm(std::move(algorithm), std::move(properties)), m_item(item), m_updateFunction(updateFunction) {
}

Item *BatchJobAlgorithm::item() { return m_item; }

void BatchJobAlgorithm::updateItem() {
  if (m_item)
    m_updateFunction(m_algorithm, *m_item);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
