// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"

#include "MantidKernel/PropertyManager.h"

namespace Mantid::API {
class MANTID_API_DLL AlgorithmRuntimeProps final : private Mantid::Kernel::PropertyManager,
                                                   public Mantid::API::IAlgorithmRuntimeProps {
public:
  AlgorithmRuntimeProps() = default;
  AlgorithmRuntimeProps(const AlgorithmRuntimeProps &) = default;
  AlgorithmRuntimeProps(AlgorithmRuntimeProps &&) = default;

  bool operator==(const Mantid::Kernel::IPropertyManager &other) override = delete;
  using Mantid::Kernel::PropertyManager::existsProperty;
  using Mantid::Kernel::PropertyManager::getDeclaredPropertyNames;
  using Mantid::Kernel::PropertyManager::getPropertyValue;

  TypedValue getProperty(const std::string &name) const override;
  void setPropertyValue(const std::string &name, const std::string &value) override;
};

} // namespace Mantid::API
