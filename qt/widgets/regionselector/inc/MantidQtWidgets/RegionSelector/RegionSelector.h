// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/RegionSelectorObserver.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/IImageInfoWidget.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/RegionSelector/DllConfig.h"
#include "MantidQtWidgets/RegionSelector/IRegionSelector.h"

class QLayout;

namespace MantidQt::Widgets {
class MANTID_REGIONSELECTOR_DLL RegionSelector : public Common::Python::InstanceHolder, public IRegionSelector {
public:
  RegionSelector(Mantid::API::Workspace_sptr const &workspace, QLayout *layout,
                 MantidWidgets::IImageInfoWidget *imageInfoWidget = nullptr);
  RegionSelector(RegionSelector const &) = delete;
  RegionSelector(RegionSelector &&);
  RegionSelector &operator=(RegionSelector const &) = delete;
  RegionSelector &operator=(RegionSelector &&);

  void subscribe(std::shared_ptr<Mantid::API::RegionSelectorObserver> const &notifyee) override;
  void clearWorkspace() override;
  void updateWorkspace(Mantid::API::Workspace_sptr const &workspace) override;
  void addRectangularRegion(const std::string &regionType, const std::string &color, const std::string &hatch) override;
  void deselectAllSelectors() override;
  Selection getRegion(const std::string &regionType) override;
  void cancelDrawingRegion() override;
  void displayRectangularRegion(const std::string &regionType, const std::string &color, const std::string &hatch,
                                const size_t y1, const size_t y2) override;

private:
  Common::Python::Object getView() const;
  void show() const;

  QLayout *m_layout;
};
} // namespace MantidQt::Widgets
