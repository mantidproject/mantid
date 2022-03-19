// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../Reduction/Experiment.h"
#include "../../Reduction/IBatch.h"
#include "../../Reduction/Instrument.h"
#include "../../Reduction/LookupRow.h"
#include "MantidFrameworkTestHelpers/FallbackBoostOptionalIO.h"

#include <gmock/gmock.h>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class MockBatch : public IBatch {
public:
  virtual ~MockBatch() = default;
  MOCK_METHOD(Experiment const &, experiment, (), (const, override));
  MOCK_METHOD(Instrument const &, instrument, (), (const, override));
  MOCK_METHOD(Slicing const &, slicing, (), (const, override));
  MOCK_METHOD(boost::optional<LookupRow>, findWildcardLookupRow, (), (const, override));
  MOCK_METHOD(boost::optional<LookupRow>, findLookupRow, (Row const &), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
