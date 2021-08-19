// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class PreviewViewSubscriber {
public:
  virtual ~PreviewViewSubscriber() = default;
  virtual void notifyLoadWorkspaceRequested() = 0;
};

class IPreviewView {
public:
  virtual ~IPreviewView() = default;
  virtual void subscribe(PreviewViewSubscriber *notifyee) noexcept = 0;
  virtual std::string getWorkspaceName() const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
