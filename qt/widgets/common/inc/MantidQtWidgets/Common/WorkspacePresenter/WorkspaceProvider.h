// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Workspace_fwd.h"
#include <map>
#include <memory>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class WorkspaceProviderNotifiable;

using Presenter_sptr = std::shared_ptr<WorkspaceProviderNotifiable>;
using Presenter_wptr = std::weak_ptr<WorkspaceProviderNotifiable>;

/**
\class  WorkspaceProvider
\author Lamar Moore
\date   24-08-2016
\version 1.0
*/
class WorkspaceProvider {
public:
  virtual ~WorkspaceProvider() = default;

  virtual void registerPresenter(Presenter_wptr presenter) = 0;
  virtual bool doesWorkspaceExist(const std::string &wsname) const = 0;
  virtual std::map<std::string, Mantid::API::Workspace_sptr> topLevelItems() const = 0;
  virtual std::string getOldName() const = 0;
  virtual std::string getNewName() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
