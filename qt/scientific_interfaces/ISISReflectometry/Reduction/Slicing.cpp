// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Slicing.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

UniformSlicingByTime::UniformSlicingByTime(double secondsPerSlice)
    : m_secondsPerSlice(secondsPerSlice) {}

double UniformSlicingByTime::sliceLengthInSeconds() const {
  return m_secondsPerSlice;
}

bool operator==(UniformSlicingByTime const &lhs,
                UniformSlicingByTime const &rhs) {
  return lhs.sliceLengthInSeconds() == rhs.sliceLengthInSeconds();
}

std::ostream &operator<<(std::ostream &os,
                         UniformSlicingByTime const &slicing) {
  return (os << "even slices " << slicing.sliceLengthInSeconds()
             << " seconds long");
}

bool isNoSlicing(Slicing const &slicing) { return slicing.which() == 0; }

bool isInvalid(Slicing const &slicing) { return slicing.which() == 1; }

bool isValid(Slicing const &slicing) { return !isInvalid(slicing); }

bool operator==(InvalidSlicing const &, InvalidSlicing const &) { return true; }

UniformSlicingByNumberOfSlices::UniformSlicingByNumberOfSlices(
    int numberOfSlices)
    : m_numberOfSlices(numberOfSlices) {}

int UniformSlicingByNumberOfSlices::numberOfSlices() const {
  return m_numberOfSlices;
}

bool operator==(UniformSlicingByNumberOfSlices const &lhs,
                UniformSlicingByNumberOfSlices const &rhs) {
  return lhs.numberOfSlices() == rhs.numberOfSlices();
}

std::ostream &operator<<(std::ostream &os,
                         UniformSlicingByNumberOfSlices const &slicing) {
  return (os << slicing.numberOfSlices() << " even slices");
}

CustomSlicingByList::CustomSlicingByList( // cppcheck-suppress passedByValue
    std::vector<double> sliceTimes)
    : m_sliceTimes(std::move(sliceTimes)) {}

std::vector<double> const &CustomSlicingByList::sliceTimes() const {
  return m_sliceTimes;
}

bool operator==(CustomSlicingByList const &lhs,
                CustomSlicingByList const &rhs) {
  return lhs.sliceTimes() == rhs.sliceTimes();
}

std::ostream &operator<<(std::ostream &os, CustomSlicingByList const &slicing) {
  os << "slices at the following times\n";
  auto const &sliceTimes = slicing.sliceTimes();
  if (sliceTimes.size() < 2) {
    os << "  no slices\n";
  } else {
    for (auto iter = sliceTimes.cbegin() + 1; iter != sliceTimes.cend();
         ++iter) {
      os << "  " << *prev(iter) << " to " << *iter << " seconds,\n";
    }
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, InvalidSlicing const &) {
  return (os << "invalid slices");
}

SlicingByEventLog::SlicingByEventLog( // cppcheck-suppress passedByValue
    std::vector<double> sliceAtValues, std::string blockValue)
    : m_sliceAtValues(std::move(sliceAtValues)),
      m_blockName(std::move(blockValue)) {}

std::vector<double> const &SlicingByEventLog::sliceAtValues() const {
  return m_sliceAtValues;
}

std::string const &SlicingByEventLog::blockName() const { return m_blockName; }

bool operator==(SlicingByEventLog const &lhs, SlicingByEventLog const &rhs) {
  return lhs.blockName() == rhs.blockName() &&
         lhs.sliceAtValues() == rhs.sliceAtValues();
}

std::ostream &operator<<(std::ostream &os, SlicingByEventLog const &slicing) {
  os << "slices at the times when the log value for the block '"
     << slicing.blockName() << "' is between\n";
  auto const &logValueBreakpoints = slicing.sliceAtValues();
  if (logValueBreakpoints.size() < 2) {
    os << "  no slices\n";
  } else {
    for (auto iter = logValueBreakpoints.cbegin() + 1;
         iter != logValueBreakpoints.cend(); ++iter) {
      os << "  " << *prev(iter) << " and " << *iter << ",\n";
    }
  }
  return os;
}
} // namespace CustomInterfaces
} // namespace MantidQt
