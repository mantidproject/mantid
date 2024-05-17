// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

namespace MantidQt {
namespace CustomInterfaces {

class IRunSubscriber {
public:
  virtual ~IRunSubscriber() = default;

  virtual void handleRunClicked() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt