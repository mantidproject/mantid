// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Batch/IBatchView.h"
#include "GUI/Common/IJobManager.h"
#include "IInstViewModel.h"
#include "IPreviewModel.h"
#include "IPreviewPresenter.h"
#include "IPreviewView.h"
#include "MantidAPI/RegionSelectorObserver.h"

#include <memory>

namespace MantidQt::Widgets {
class IRegionSelector;
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewPresenter : public IPreviewPresenter,
                                                        public PreviewViewSubscriber,
                                                        public JobManagerSubscriber,
                                                        public Mantid::API::RegionSelectorObserver {
public:
  // Stub class to observe region selector; needed because it has to be passed
  // through as a shared_ptr to python code, so we cannot pass ourselves
  class StubRegionObserver : public Mantid::API::RegionSelectorObserver {
  public:
    // subscribe so we can get the notifcation passed back to us
    void subscribe(RegionSelectorObserver *notifyee) { m_notifyee = notifyee; }
    // override the notification and just pass it through
    void notifyRegionChanged() override { m_notifyee->notifyRegionChanged(); }

  private:
    Mantid::API::RegionSelectorObserver *m_notifyee{nullptr};
  };

  struct Dependencies {
    IPreviewView *view{nullptr};
    std::unique_ptr<IPreviewModel> model;
    std::unique_ptr<IJobManager> jobManager;
    std::unique_ptr<IInstViewModel> instViewModel;
    std::unique_ptr<MantidQt::Widgets::IRegionSelector> regionSelector{nullptr};
  };

  PreviewPresenter(Dependencies dependencies);
  virtual ~PreviewPresenter() = default;

  // PreviewViewSubscriber overrides
  void notifyLoadWorkspaceRequested() override;

  void notifyInstViewZoomRequested() override;
  void notifyInstViewEditRequested() override;
  void notifyInstViewSelectRectRequested() override;
  void notifyInstViewShapeChanged() override;

  void notifyRegionSelectorExportAdsRequested() override;
  void notify1DPlotExportAdsRequested() override;

  void notifyRectangularROIModeRequested() override;

  // JobManagerSubscriber overrides
  void notifyLoadWorkspaceCompleted() override;
  void notifySumBanksCompleted() override;
  void notifyReductionCompleted() override;

  // RegionSelectionObserver overrides
  void notifyRegionChanged() override;

private:
  IPreviewView *m_view{nullptr};
  std::unique_ptr<IPreviewModel> m_model;
  std::unique_ptr<IJobManager> m_jobManager;
  std::unique_ptr<IInstViewModel> m_instViewModel;
  std::unique_ptr<MantidQt::Widgets::IRegionSelector> m_regionSelector;
  std::shared_ptr<StubRegionObserver> m_stubRegionObserver;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
