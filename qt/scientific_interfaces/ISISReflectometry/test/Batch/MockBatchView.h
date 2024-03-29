// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Batch/IBatchView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MockBatchView : public IBatchView {
public:
  MOCK_CONST_METHOD0(runs, IRunsView *());
  MOCK_CONST_METHOD0(eventHandling, IEventView *());
  MOCK_CONST_METHOD0(save, ISaveView *());
  MOCK_CONST_METHOD0(experiment, IExperimentView *());
  MOCK_CONST_METHOD0(instrument, IInstrumentView *());
  MOCK_CONST_METHOD0(preview, IPreviewView *());
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
