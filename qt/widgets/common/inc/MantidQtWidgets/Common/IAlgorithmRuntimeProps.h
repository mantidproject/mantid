// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidKernel/IPropertyManager.h"

namespace MantidQt::API {
class EXPORT_OPT_MANTIDQT_COMMON IAlgorithmRuntimeProps : public virtual Mantid::Kernel::IPropertyManager {
public:
  IAlgorithmRuntimeProps() = default;
  virtual ~IAlgorithmRuntimeProps() = default;

  template <typename T> void setProperty(const std::string &name, const T &value) {
    if (!existsProperty(name)) {
      declareProperty(name, value);
    } else {
      Mantid::Kernel::IPropertyManager::setProperty(name, value);
    }
  }

  virtual void setPropertyValue(const std::string &, const std::string &) override = 0;
  virtual TypedValue getProperty(const std::string &name) const noexcept override = 0;
};

} // namespace MantidQt::API
