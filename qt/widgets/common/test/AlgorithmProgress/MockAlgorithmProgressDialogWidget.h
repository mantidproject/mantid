// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/IAlgorithmProgressDialogWidget.h"
#include "MockAlgorithmProgressWidget.h"

#include <gmock/gmock.h>
#include <memory>

using namespace testing;
using namespace MantidQt::MantidWidgets;
class QProgressBar;
class QWidget;

class MockAlgorithmProgressDialogWidget : public IAlgorithmProgressDialogWidget {
public:
  virtual ~MockAlgorithmProgressDialogWidget() = default;
  MOCK_METHOD((std::pair<QTreeWidgetItem *, QProgressBar *>), addAlgorithm, (Mantid::API::IAlgorithm_sptr), (override));
};
