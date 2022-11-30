// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Plotting/PlotWidget/PlotModel.h"

#include <gmock/gmock.h>

namespace MantidQt::MantidWidgets {

class MockPlotModel : public PlotModel {
public:
  virtual ~MockPlotModel() = default;

  MOCK_METHOD(std::vector<Mantid::API::MatrixWorkspace_sptr>, getWorkspaces, (), (const, override));
  MOCK_METHOD(std::vector<int>, getWorkspaceIndices, (), (const, override));
  MOCK_METHOD(void, setSpectrum, (const Mantid::API::MatrixWorkspace_sptr &, const size_t), (override));
  MOCK_METHOD(void, setPlotErrorBars, (const bool), (override));
  MOCK_METHOD(bool, getPlotErrorBars, (), (const, override));
};
} // namespace MantidQt::MantidWidgets
