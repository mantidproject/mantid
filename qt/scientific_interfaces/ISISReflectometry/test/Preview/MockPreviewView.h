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
  MOCK_METHOD(QLayout *, getDockedWidgetsLayout, (), (noexcept, override));
  MOCK_METHOD(MantidWidgets::IImageInfoWidget *, getImageInfo, (), (noexcept, override));
  MOCK_METHOD(void, enableMainWidget, (), (override));
  MOCK_METHOD(void, disableMainWidget, (), (override));
  MOCK_METHOD(std::string, getWorkspaceName, (), (const, override));
  MOCK_METHOD(double, getAngle, (), (const, override));
  MOCK_METHOD(void, setAngle, (double), (override));
  MOCK_METHOD(void, setUpdateAngleButtonEnabled, (bool), (override));
  MOCK_METHOD(void, setTitle, (const std::string &), (override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
