// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/RegionSelector/IRegionSelector.h"

namespace MantidQt::Widgets {
class MockRegionSelector : public IRegionSelector {
public:
  MOCK_METHOD(void, updateWorkspace, (Mantid::API::Workspace_sptr const &workspace), (override));
};
} // namespace MantidQt::Widgets