// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "ISimpleWidget.h"

#include <QObject>
#include <gmock/gmock.h>

namespace MantidQt::MantidWidgets {

class MockSimpleWidget : public ISimpleWidget {
public:
  MOCK_METHOD(void, setSurface, (std::shared_ptr<ProjectionSurface>), (override));
  MOCK_METHOD(std::shared_ptr<ProjectionSurface>, getSurface, (), (override));
  MOCK_METHOD(void, updateView, (bool), (override));
  MOCK_METHOD(void, updateDetectors, (), (override));
  MOCK_METHOD(void, saveToFile, (const QString &), (override));

  // Qt overrides
  MOCK_METHOD(void, qtInstallEventFilter, (QObject *), (override));
  MOCK_METHOD(void, qtUpdate, (), (override));
};
} // namespace MantidQt::MantidWidgets
