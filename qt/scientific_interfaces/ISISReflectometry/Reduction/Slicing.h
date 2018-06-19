#ifndef MANTID_CUSTOMINTERFACES_SLICING_H_
#define MANTID_CUSTOMINTERFACES_SLICING_H_
#include <boost/variant.hpp>
#include <vector>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class UniformSlicingByTime {
public:
  explicit UniformSlicingByTime(double secondsPerSlice);

private:
  double m_secondsPerSlice;
};

class UniformSlicingByNumberOfSlices {
public:
  explicit UniformSlicingByNumberOfSlices(int numberOfSlices);

private:
  int m_numberOfSlices;
};

class CustomSlicingByList {
public:
  explicit CustomSlicingByList(std::vector<double> sliceTimes);

private:
  std::vector<double> m_sliceTimes;
};

class SlicingByEventLog {
public:
  SlicingByEventLog(std::vector<double> sliceAtValues, std::string blockValue);

private:
  std::vector<double> m_sliceAtValues;
  std::string m_blockName;
};

using Slicing = boost::variant<boost::blank, UniformSlicingByTime,
                               UniformSlicingByNumberOfSlices,
                               CustomSlicingByList, SlicingByEventLog>;
}
}

#endif // MANTID_CUSTOMINTERFACES_SLICING_H_
