// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressWidget.h"

#include <boost/enable_shared_from_this.hpp>
#include <gmock/gmock.h>
#include <memory>

using namespace testing;
using namespace MantidQt::MantidWidgets;
class QProgressBar;
class QWidget;

class MockAlgorithmProgressWidget : public IAlgorithmProgressWidget {
public:
  virtual ~MockAlgorithmProgressWidget() = default;
  MOCK_METHOD(void, algorithmStarted, (), (override));
  MOCK_METHOD(void, algorithmEnded, (), (override));
  MOCK_METHOD(void, updateProgress, (const double, const QString &, const double, const int), (override));
  MOCK_METHOD(void, showDetailsDialog, (), (override));
};
