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
#include <vector>
#include <string>
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL UniformSlicingByTime {
public:
  explicit UniformSlicingByTime(double secondsPerSlice);
  double sliceLengthInSeconds() const;

private:
  double m_secondsPerSlice;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(UniformSlicingByTime const &lhs,
                                               UniformSlicingByTime const &rhs);

class MANTIDQT_ISISREFLECTOMETRY_DLL UniformSlicingByNumberOfSlices {
public:
  explicit UniformSlicingByNumberOfSlices(int numberOfSlices);
  int numberOfSlices() const;

private:
  int m_numberOfSlices;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(UniformSlicingByNumberOfSlices const &lhs,
           UniformSlicingByNumberOfSlices const &rhs);

class MANTIDQT_ISISREFLECTOMETRY_DLL CustomSlicingByList {
public:
  explicit CustomSlicingByList(std::vector<double> sliceTimes);
  std::vector<double> const &sliceTimes() const;

private:
  std::vector<double> m_sliceTimes;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(CustomSlicingByList const &lhs,
                                               CustomSlicingByList const &rhs);

class MANTIDQT_ISISREFLECTOMETRY_DLL SlicingByEventLog {
public:
  SlicingByEventLog(std::vector<double> sliceAtValues, std::string blockValue);
  std::vector<double> const &sliceAtValues() const;
  std::string const &blockName() const;

private:
  std::vector<double> m_sliceAtValues;
  std::string m_blockName;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(SlicingByEventLog const &lhs,
                                               SlicingByEventLog const &rhs);

class InvalidSlicing {};
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(InvalidSlicing const &lhs,
                                               InvalidSlicing const &rhs);

using Slicing =
    boost::variant<boost::blank, InvalidSlicing, UniformSlicingByTime,
                   UniformSlicingByNumberOfSlices, CustomSlicingByList,
                   SlicingByEventLog>;

MANTIDQT_ISISREFLECTOMETRY_DLL bool isInvalid(Slicing const &slicing);
MANTIDQT_ISISREFLECTOMETRY_DLL bool isValid(Slicing const &slicing);
MANTIDQT_ISISREFLECTOMETRY_DLL bool isNoSlicing(Slicing const &slicing);
}
}

#endif // MANTID_CUSTOMINTERFACES_SLICING_H_
