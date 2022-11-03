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
  MOCK_METHOD(void, enableApplyButton, (), (override));
  MOCK_METHOD(void, disableApplyButton, (), (override));
  MOCK_METHOD(std::string, getWorkspaceName, (), (const, override));
  MOCK_METHOD(double, getAngle, (), (const, override));
  MOCK_METHOD(void, resetInstView, (), (override));
  MOCK_METHOD(void, plotInstView,
              (MantidWidgets::InstrumentActor *, Mantid::Kernel::V3D const &, Mantid::Kernel::V3D const &), (override));
  MOCK_METHOD(void, setInstViewZoomState, (bool), (override));
  MOCK_METHOD(void, setInstViewEditState, (bool), (override));
  MOCK_METHOD(void, setInstViewSelectRectState, (bool), (override));
  MOCK_METHOD(void, setInstViewSelectRectMode, (), (override));
  MOCK_METHOD(void, setInstViewToolbarEnabled, (bool), (override));
  MOCK_METHOD(void, setRegionSelectorEnabled, (bool), (override));
  MOCK_METHOD(void, setInstViewZoomMode, (), (override));
  MOCK_METHOD(void, setInstViewEditMode, (), (override));
  MOCK_METHOD(void, setRectangularROIState, (bool), (override));
  MOCK_METHOD(void, setEditROIState, (bool), (override));
  MOCK_METHOD(void, setAngle, (double), (override));
  MOCK_METHOD(void, setUpdateAngleButtonEnabled, (bool), (override));
  MOCK_METHOD(std::vector<size_t>, getSelectedDetectors, (), (const, override));
  MOCK_METHOD(std::string, getRegionType, (), (const, override));
  MOCK_METHOD(QLayout *, getRegionSelectorLayout, (), (const, override));
  MOCK_METHOD(MantidQt::MantidWidgets::IPlotView *, getLinePlotView, (), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
