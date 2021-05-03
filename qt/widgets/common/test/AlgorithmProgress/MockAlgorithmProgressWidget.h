// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressWidget.h"

#include <boost/enable_shared_from_this.hpp>
#include <gmock/gmock.h>
#include <memory>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace testing;
using namespace MantidQt::MantidWidgets;
class QProgressBar;
class QWidget;

class MockAlgorithmProgressWidget : public IAlgorithmProgressWidget {
public:
  MockAlgorithmProgressWidget() {
    m_presenter = std::make_shared<AlgorithmProgressPresenter>(static_cast<QWidget *>(nullptr), this);
  }
  virtual ~MockAlgorithmProgressWidget() {}

  MOCK_METHOD0(algorithmStarted, void());
  MOCK_METHOD0(algorithmEnded, void());
  MOCK_METHOD4(updateProgress, void(const double, const QString &, const double, const int));
  MOCK_METHOD0(showDetailsDialog, void());

  std::shared_ptr<AlgorithmProgressPresenter> m_presenter;
};