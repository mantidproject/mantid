// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
class IUserInputValidator;

class MANTID_SPECTROSCOPY_DLL IRunSubscriber {
public:
  virtual ~IRunSubscriber() = default;

  virtual void handleValidation(IUserInputValidator *validator) const = 0;
  virtual void handleRun() = 0;
  virtual const std::string getSubscriberName() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt
