// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPreviewView.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"

#include <gmock/gmock.h>

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockPreviewView : public IPreviewView {
public:
  MOCK_METHOD(void, subscribe, (PreviewViewSubscriber *), (noexcept, override));
  MOCK_METHOD(std::string, getWorkspaceName, (), (const, override));
  MOCK_METHOD(void, plotInstView,
              (MantidWidgets::InstrumentActor *, Mantid::Kernel::V3D const &, Mantid::Kernel::V3D const &), (override));
  MOCK_METHOD(void, setInstViewZoomState, (bool), (override));
  MOCK_METHOD(void, setInstViewEditState, (bool), (override));
  MOCK_METHOD(void, setInstViewSelectRectState, (bool), (override));
  MOCK_METHOD(void, setInstViewSelectRectMode, (), (override));
  MOCK_METHOD(void, setInstViewToolbarEnabled, (bool), (override));
  MOCK_METHOD(void, setInstViewZoomMode, (), (override));
  MOCK_METHOD(void, setInstViewEditMode, (), (override));
  MOCK_METHOD(std::vector<size_t>, getSelectedDetectors, (), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
