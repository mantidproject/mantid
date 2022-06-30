// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/RegionSelector/DllConfig.h"

class QLayout;

namespace MantidQt::Widgets {
class MANTID_REGIONSELECTOR_DLL IRegionSelector {
public:
  virtual ~IRegionSelector() = default;
  virtual void updateWorkspace(Mantid::API::Workspace_sptr const &workspace) = 0;
};
} // namespace MantidQt::Widgets