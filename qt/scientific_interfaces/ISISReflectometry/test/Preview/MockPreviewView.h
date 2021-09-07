// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPreviewView.h"

#include <gmock/gmock.h>

#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockPreviewView : public IPreviewView {
public:
  MOCK_METHOD(void, subscribe, (PreviewViewSubscriber *), (noexcept, override));
  MOCK_METHOD(std::string, getWorkspaceName, (), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
