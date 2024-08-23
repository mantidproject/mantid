// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Plotter.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <QHash>
#include <QString>
#include <QVariant>
using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

void Plotter::reflectometryPlot(const std::vector<std::string> &workspaces) const {
  QHash<QString, QVariant> ax_properties;
  ax_properties[QString("yscale")] = QVariant("log");
  ax_properties[QString("xscale")] = QVariant("log");

  // plot(workspaces, spectrum_nums, wksp_indices, fig, plot_kwargs,
  // ax_properties, windows_title, errors, overplot)
  std::string window_title = "ISIS Reflectometry Plot";
  if (!workspaces.empty()) {
    window_title = workspaces[0];
  }

  const bool plotErrorBars = true;
  const std::vector<int> wksp_indices = {0};

  std::vector<std::string> actualWorkspaces;
  actualWorkspaces.reserve(workspaces.size());
  for (const auto &workspace : workspaces) {
    const Mantid::API::Workspace_sptr workspaceObject =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(workspace);
    if (const auto workspaceGroup = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(workspaceObject)) {
      for (auto index = 0u; index < workspaceGroup->size(); ++index) {
        actualWorkspaces.emplace_back(workspaceGroup->getItem(index)->getName());
      }
    } else {
      actualWorkspaces.emplace_back(workspace);
    }
  }

  plot(actualWorkspaces, std::nullopt, wksp_indices, std::nullopt, std::nullopt, ax_properties, window_title,
       plotErrorBars, false);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
