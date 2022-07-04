// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/RegionSelectorObserver.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/RegionSelector/DllConfig.h"
#include "MantidQtWidgets/RegionSelector/IRegionSelector.h"

class QLayout;

namespace MantidQt::Widgets {
class MANTID_REGIONSELECTOR_DLL RegionSelector : public Common::Python::InstanceHolder, public IRegionSelector {
public:
  RegionSelector(Mantid::API::Workspace_sptr const &workspace, QLayout *layout);
  RegionSelector(RegionSelector const &) = delete;
  RegionSelector(RegionSelector &&);
  RegionSelector &operator=(RegionSelector const &) = delete;
  RegionSelector &operator=(RegionSelector &&);

  void subscribe(std::shared_ptr<Mantid::API::RegionSelectorObserver> const &notifyee) override;
  void updateWorkspace(Mantid::API::Workspace_sptr const &workspace) override;
  void addRectangularRegion() override;
  Selection getRegion() override;

private:
  Common::Python::Object getView() const;
  void show() const;

  QLayout *m_layout;
};
} // namespace MantidQt::Widgets
