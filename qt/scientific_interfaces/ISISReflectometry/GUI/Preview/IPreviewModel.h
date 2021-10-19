// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IJobManager;

class IPreviewModel {
public:
  IPreviewModel() = default;
  virtual ~IPreviewModel() = default;
  virtual bool loadWorkspaceFromAds(std::string const &workspaceName) = 0;
  virtual void loadAndPreprocessWorkspaceAsync(std::string const &workspaceName, IJobManager &jobManager) = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getLoadedWs() const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
