// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/RegionSelectorObserver.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/RegionSelector/DllConfig.h"

#include <vector>

class QLayout;

namespace MantidQt::Widgets {
class MANTID_REGIONSELECTOR_DLL IRegionSelector {
public:
  using Selection = std::vector<double>;
  virtual ~IRegionSelector() = default;
  virtual void subscribe(std::shared_ptr<Mantid::API::RegionSelectorObserver> const &notifyee) = 0;
  virtual void updateWorkspace(Mantid::API::Workspace_sptr const &workspace) = 0;
  virtual void addRectangularRegion() = 0;
  virtual Selection getRegion() = 0;
};
} // namespace MantidQt::Widgets