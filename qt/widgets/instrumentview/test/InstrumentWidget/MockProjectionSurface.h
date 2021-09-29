// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/GLDisplay.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"

#include <gmock/gmock.h>

namespace MantidQt::MantidWidgets {
class MockProjectionSurface : public ProjectionSurface {
public:
  MockProjectionSurface() : ProjectionSurface(nullptr) {}
  virtual ~MockProjectionSurface() = default;
  MOCK_METHOD(void, init, (), (override));
  MOCK_METHOD(void, componentSelected, (size_t), (override));
  MOCK_METHOD(void, getSelectedDetectors, (std::vector<size_t> &), (override));
  MOCK_METHOD(void, getMaskedDetectors, (std::vector<size_t> &), (const, override));
  MOCK_METHOD(void, drawSurface, (GLDisplay *, bool), (const, override));
  MOCK_METHOD(void, changeColorMap, (), (override));
};
} // namespace MantidQt::MantidWidgets
