// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPreviewPresenter.h"
#include "Reduction/PreviewRow.h"

#include <gmock/gmock.h>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockPreviewPresenter : public IPreviewPresenter {
public:
  MOCK_METHOD(void, acceptMainPresenter, (IBatchPresenter *), (override));
  MOCK_METHOD(void, notifyReductionResumed, (), (override));
  MOCK_METHOD(void, notifyReductionPaused, (), (override));
  MOCK_METHOD(void, notifyAutoreductionResumed, (), (override));
  MOCK_METHOD(void, notifyAutoreductionPaused, (), (override));

  MOCK_METHOD(PreviewRow const &, getPreviewRow, (), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
