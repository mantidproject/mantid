// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewJobManager.h"
#include "GUI/Batch/IReflAlgorithmFactory.h"
#include "GUI/Batch/ReflAlgorithmFactory.h"
#include "Reduction/IBatch.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

PreviewJobManager::PreviewJobManager(IBatch &batch, std::unique_ptr<IReflAlgorithmFactory> algFactory)
    : m_algFactory(std::move(algFactory)) {
  if (!m_algFactory)
    m_algFactory = std::make_unique<ReflAlgorithmFactory>(batch);
}

MantidQt::API::IConfiguredAlgorithm_sptr PreviewJobManager::getPreprocessingAlgorithm(PreviewRow &row) const {
  return m_algFactory->makePreprocessingAlgorithm(row);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry