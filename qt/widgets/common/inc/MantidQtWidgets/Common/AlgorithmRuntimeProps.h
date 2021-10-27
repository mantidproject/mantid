// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IAlgorithmRuntimeProps.h"

#include "MantidKernel/PropertyManager.h"

namespace MantidQt::API {
class EXPORT_OPT_MANTIDQT_COMMON AlgorithmRuntimeProps final : private Mantid::Kernel::PropertyManager,
                                                               public IAlgorithmRuntimeProps {
public:
  AlgorithmRuntimeProps() = default;
  AlgorithmRuntimeProps(const AlgorithmRuntimeProps &) = default;
  AlgorithmRuntimeProps(AlgorithmRuntimeProps &&) = default;
  ~AlgorithmRuntimeProps() = default;

  bool operator==(const AlgorithmRuntimeProps &);
  using Mantid::Kernel::PropertyManager::getPropertyValue;

  TypedValue getProperty(const std::string &name) const noexcept override;
  void setPropertyValue(const std::string &name, const std::string &value) override;
};

} // namespace MantidQt::API
