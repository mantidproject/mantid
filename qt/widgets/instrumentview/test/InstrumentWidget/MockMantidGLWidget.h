// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "IMantidGLWidget.h"
#include "MantidKernel/WarningSuppressions.h"

#include <QGLWidget>
#include <QString>
#include <gmock/gmock.h>
#include <stddef.h>

namespace MantidQt::MantidWidgets {

class MockMantidGLWidget : public IMantidGLWidget {
public:
  MOCK_METHOD(void, setSurface, (std::shared_ptr<ProjectionSurface>), (override));
  MOCK_METHOD(std::shared_ptr<ProjectionSurface>, getSurface, (), (override));
  MOCK_METHOD(void, updateView, (bool), (override));
  MOCK_METHOD(void, updateDetectors, (), (override));
  MOCK_METHOD(void, saveToFile, (const QString &), (override));

  MOCK_METHOD(void, setBackgroundColor, (const QColor &), (override));
  MOCK_METHOD(QColor, currentBackgroundColor, (), (const, override));
  MOCK_METHOD(void, enableLighting, (bool), (override));
  MOCK_METHOD(void, componentSelected, (size_t), (override));
};
} // namespace MantidQt::MantidWidgets
