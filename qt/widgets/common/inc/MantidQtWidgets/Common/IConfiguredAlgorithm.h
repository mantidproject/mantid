// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"

#include <memory>

namespace MantidQt::API {
class EXPORT_OPT_MANTIDQT_COMMON IConfiguredAlgorithm {
public:
  virtual ~IConfiguredAlgorithm() = default;
  virtual Mantid::API::IAlgorithm_sptr algorithm() const = 0;
  virtual const Mantid::API::IAlgorithmRuntimeProps &getAlgorithmRuntimeProps() const noexcept = 0;
  virtual bool validatePropsPreExec() const noexcept = 0;
};

using IConfiguredAlgorithm_sptr = std::shared_ptr<IConfiguredAlgorithm>;

} // namespace MantidQt::API
