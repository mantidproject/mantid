// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <gmock/gmock.h>

namespace MantidQt::MantidWidgets {

class MockInstrumentWidgetMaskTab final : public InstrumentWidgetMaskTab {
public:
  explicit MockInstrumentWidgetMaskTab(InstrumentWidget *instrWidget) : InstrumentWidgetMaskTab(instrWidget) {};

  MOCK_METHOD(void, showMessageBox, (const QString &), (override));
};

} // namespace MantidQt::MantidWidgets
