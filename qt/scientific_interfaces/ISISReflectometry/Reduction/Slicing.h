// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_SLICING_H_
#define MANTID_CUSTOMINTERFACES_SLICING_H_
#include "Common/DllConfig.h"
#include <boost/variant.hpp>
#include <ostream>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL UniformSlicingByTime {
public:
  explicit UniformSlicingByTime(double secondsPerSlice);
  double sliceLengthInSeconds() const;

private:
  double m_secondsPerSlice;
};

MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &os, UniformSlicingByTime const &slicing);

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(UniformSlicingByTime const &lhs,
                                               UniformSlicingByTime const &rhs);

class MANTIDQT_ISISREFLECTOMETRY_DLL UniformSlicingByNumberOfSlices {
public:
  explicit UniformSlicingByNumberOfSlices(int numberOfSlices);
  int numberOfSlices() const;

private:
  int m_numberOfSlices;
};

MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &os, UniformSlicingByNumberOfSlices const &slicing);

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

MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &os, CustomSlicingByList const &slicing);

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

MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &os, SlicingByEventLog const &slicing);

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(SlicingByEventLog const &lhs,
                                               SlicingByEventLog const &rhs);

class InvalidSlicing {};
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(InvalidSlicing const &lhs,
                                               InvalidSlicing const &rhs);

MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &os, InvalidSlicing const &slicing);

/** Slicing holds information about the type of event slicing
 * to be performed on the input workspace before reduction
 */
using Slicing =
    boost::variant<boost::blank, InvalidSlicing, UniformSlicingByTime,
                   UniformSlicingByNumberOfSlices, CustomSlicingByList,
                   SlicingByEventLog>;

MANTIDQT_ISISREFLECTOMETRY_DLL bool isInvalid(Slicing const &slicing);
MANTIDQT_ISISREFLECTOMETRY_DLL bool isValid(Slicing const &slicing);
MANTIDQT_ISISREFLECTOMETRY_DLL bool isNoSlicing(Slicing const &slicing);
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_SLICING_H_
