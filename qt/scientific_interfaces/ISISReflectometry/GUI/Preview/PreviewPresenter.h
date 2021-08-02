// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPreviewModel.h"
#include "IPreviewPresenter.h"
#include "IPreviewView.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewPresenter : public PreviewViewSubscriber, public IPreviewPresenter {
public:
  PreviewPresenter(IPreviewView *view, std::unique_ptr<IPreviewModel> model);
  virtual ~PreviewPresenter() = default;
  void notifyLoadWorkspaceRequested() override;

private:
  IPreviewView *m_view{nullptr};
  std::unique_ptr<IPreviewModel> m_model;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
