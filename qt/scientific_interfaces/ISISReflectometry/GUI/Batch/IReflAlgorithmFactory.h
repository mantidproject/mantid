// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class Group;
class PreviewRow;
class Row;

class IReflAlgorithmFactory {
public:
  virtual ~IReflAlgorithmFactory() = default;

  virtual MantidQt::API::IConfiguredAlgorithm_sptr makePostprocessingAlgorithm(Group &group) const = 0;
  virtual MantidQt::API::IConfiguredAlgorithm_sptr makeReductionAlgorithm(Row &row) const = 0;
  virtual MantidQt::API::IConfiguredAlgorithm_sptr makePreprocessingAlgorithm(PreviewRow &row) const = 0;
  virtual MantidQt::API::IConfiguredAlgorithm_sptr makeSumBanksAlgorithm(PreviewRow &row) const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
