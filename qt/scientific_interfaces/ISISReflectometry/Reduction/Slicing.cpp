#include "Slicing.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

UniformSlicingByTime::UniformSlicingByTime(double secondsPerSlice)
    : m_secondsPerSlice(secondsPerSlice) {}

UniformSlicingByNumberOfSlices::UniformSlicingByNumberOfSlices(
    int numberOfSlices)
    : m_numberOfSlices(numberOfSlices) {}

CustomSlicingByList::CustomSlicingByList(std::vector<double> sliceTimes)
    : m_sliceTimes(std::move(sliceTimes)) {}

SlicingByEventLog::SlicingByEventLog(std::vector<double> sliceAtValues,
                                     std::string blockValue)
    : m_sliceAtValues(std::move(sliceAtValues)),
      m_blockName(std::move(blockValue)) {}
}
}
