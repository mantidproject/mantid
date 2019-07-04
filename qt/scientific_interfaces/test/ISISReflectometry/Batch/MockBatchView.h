// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../../../ISISReflectometry/GUI/Batch/IBatchView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockBatchView : public IBatchView {
public:
  MOCK_METHOD1(subscribe, void(BatchViewSubscriber *));
  MOCK_CONST_METHOD0(runs, IRunsView *());
  MOCK_CONST_METHOD0(eventHandling, IEventView *());
  MOCK_CONST_METHOD0(save, ISaveView *());
  MOCK_CONST_METHOD0(experiment, IExperimentView *());
  MOCK_CONST_METHOD0(instrument, IInstrumentView *());
  MOCK_METHOD0(clearAlgorithmQueue, void());
  MOCK_METHOD1(setAlgorithmQueue,
               void(std::deque<API::IConfiguredAlgorithm_sptr>));
  MOCK_METHOD0(executeAlgorithmQueue, void());
  MOCK_METHOD0(cancelAlgorithmQueue, void());
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
