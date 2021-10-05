// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/Reduction/IBatch.h"
#include <gmock/gmock.h>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class MockBatch : public IBatch {
public:
  virtual ~MockBatch() = default;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry