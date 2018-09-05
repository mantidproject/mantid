/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTID_CUSTOMINTERFACES_SLICING_H_
#define MANTID_CUSTOMINTERFACES_SLICING_H_
#include <boost/variant.hpp>
#include <string>
#include <vector>

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
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_SLICING_H_
