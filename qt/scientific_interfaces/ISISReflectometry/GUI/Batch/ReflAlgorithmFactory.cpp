// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "ReflAlgorithmFactory.h"
#include "Reduction/Batch.h"
#include "Reduction/PreviewRow.h"
#include "RowPreprocessingAlgorithm.h"
#include "SumBanksAlgorithm.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

ReflAlgorithmFactory::ReflAlgorithmFactory(IBatch const &batch) : m_batch(batch) {}

MantidQt::API::IConfiguredAlgorithm_sptr ReflAlgorithmFactory::makePreprocessingAlgorithm(PreviewRow &row) const {
  return PreprocessRow::createConfiguredAlgorithm(m_batch, row, nullptr);
}

MantidQt::API::IConfiguredAlgorithm_sptr ReflAlgorithmFactory::makeSumBanksAlgorithm(PreviewRow &row) const {
  return SumBanks::createConfiguredAlgorithm(m_batch, row, nullptr);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry