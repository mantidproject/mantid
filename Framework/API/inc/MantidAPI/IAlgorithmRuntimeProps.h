// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid::API {
class MANTID_API_DLL IAlgorithmRuntimeProps : public virtual Mantid::Kernel::IPropertyManager {
public:
  IAlgorithmRuntimeProps() = default;
  ~IAlgorithmRuntimeProps() override = default;

  using Mantid::Kernel::IPropertyManager::getDeclaredPropertyNames;

  // Trying to compare properties downcasts to string types, which results in a bad_cast for sptr types
  // so you will need to manually call getProperty(name) and cast to type T before comparing
  virtual bool operator==(const Mantid::Kernel::IPropertyManager &) = delete;

  template <typename T> void setProperty(const std::string &name, const T &value) {
    if (!existsProperty(name)) {
      declareProperty(name, value);
    } else {
      Mantid::Kernel::IPropertyManager::setProperty(name, value);
    }
  }

  void setPropertyValue(const std::string &, const std::string &) override = 0;
  TypedValue getProperty(const std::string &name) const override = 0;
};

} // namespace Mantid::API
