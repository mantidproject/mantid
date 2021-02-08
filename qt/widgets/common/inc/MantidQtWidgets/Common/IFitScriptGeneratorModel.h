// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON IFitScriptGeneratorModel {

public:
  virtual ~IFitScriptGeneratorModel() = default;

  virtual void removeWorkspaceDomain(std::string const &workspaceName,
                                     WorkspaceIndex workspaceIndex) = 0;
  virtual void addWorkspaceDomain(std::string const &workspaceName,
                                  WorkspaceIndex workspaceIndex, double startX,
                                  double endX) = 0;

  [[nodiscard]] virtual bool isStartXValid(std::string const &workspaceName,
                                           WorkspaceIndex workspaceIndex,
                                           double startX) const = 0;
  [[nodiscard]] virtual bool isEndXValid(std::string const &workspaceName,
                                         WorkspaceIndex workspaceIndex,
                                         double endX) const = 0;

  virtual void updateStartX(std::string const &workspaceName,
                            WorkspaceIndex workspaceIndex, double startX) = 0;
  virtual void updateEndX(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double endX) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
