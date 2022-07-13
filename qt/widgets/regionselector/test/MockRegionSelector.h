// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/RegionSelectorObserver.h"
#include "MantidQtWidgets/RegionSelector/IRegionSelector.h"

namespace MantidQt::Widgets {
class MockRegionSelector : public IRegionSelector {
public:
  MOCK_METHOD(void, subscribe, (std::shared_ptr<Mantid::API::RegionSelectorObserver> const &), (override));
  MOCK_METHOD(void, updateWorkspace, (Mantid::API::Workspace_sptr const &workspace), (override));
  MOCK_METHOD(void, addRectangularRegion, (), (override));
  MOCK_METHOD(Selection, getRegion, (), (override));
};
} // namespace MantidQt::Widgets