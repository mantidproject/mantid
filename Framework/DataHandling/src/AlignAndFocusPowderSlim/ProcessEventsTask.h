#pragma once

#include "BankCalibration.h"
#include <tbb/tbb.h>
#include <vector>

namespace Mantid::DataHandling {

template <typename DetidType, typename TofType> class ProcessEventsTask {

public:
  ProcessEventsTask(const std::vector<DetidType> *detids, const std::vector<TofType> *tofs,
                    const BankCalibration *calibration, const std::vector<double> *binedges);

  ProcessEventsTask(ProcessEventsTask &other, tbb::split);

  void operator()(const tbb::blocked_range<size_t> &range);

  void join(const ProcessEventsTask &other);

  /// Local histogram for this block/thread
  std::vector<uint32_t> y_temp;

private:
  const std::vector<DetidType> *m_detids;
  const std::vector<TofType> *m_tofs;
  const BankCalibration *m_calibration;
  const std::vector<double> *m_binedges;
};
} // namespace Mantid::DataHandling
