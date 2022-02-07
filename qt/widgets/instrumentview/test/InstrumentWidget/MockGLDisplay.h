// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidQtWidgets/InstrumentView/IGLDisplay.h"

#include <QGLWidget>
#include <QString>
#include <gmock/gmock.h>
#include <stddef.h>

namespace MantidQt::MantidWidgets {

class MockGLDisplay : public IGLDisplay {
public:
  virtual ~MockGLDisplay() = default;
  MOCK_METHOD(void, setSurface, (std::shared_ptr<ProjectionSurface>), (override));
  MOCK_METHOD(std::shared_ptr<ProjectionSurface>, getSurface, (), (override));
  MOCK_METHOD(void, updateView, (bool), (override));
  MOCK_METHOD(void, updateDetectors, (), (override));
  MOCK_METHOD(void, saveToFile, (const QString &), (override));

  MOCK_METHOD(void, setBackgroundColor, (const QColor &), (override));
  MOCK_METHOD(QColor, currentBackgroundColor, (), (const, override));
  MOCK_METHOD(void, enableLighting, (bool), (override));
  MOCK_METHOD(void, componentSelected, (size_t), (override));

  // Qt overrides
  MOCK_METHOD(void, qtInstallEventFilter, (QObject *), (override));
  MOCK_METHOD(void, qtUpdate, (), (override));
  MOCK_METHOD(void, qtSetMinimumWidth, (int), (override));
};
} // namespace MantidQt::MantidWidgets
