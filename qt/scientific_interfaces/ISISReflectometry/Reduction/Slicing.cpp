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

CustomSlicingByList::CustomSlicingByList(std::vector<double> sliceTimes)
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
  std::adjacent_find(sliceTimes.cbegin(), sliceTimes.cend(),
                     [&os](double start, double end) -> bool {
                       os << "  " << start << " to " << end << " seconds,\n";
                       return false;
                     });
  return os;
}

std::ostream &operator<<(std::ostream &os, InvalidSlicing const &) {
  return (os << "invalid slices");
}

SlicingByEventLog::SlicingByEventLog(std::vector<double> sliceAtValues,
                                     std::string blockValue)
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
  std::adjacent_find(logValueBreakpoints.cbegin(), logValueBreakpoints.cend(),
                     [&os](double start, double end) -> bool {
                       os << "  " << start << " and " << end << ",\n";
                       return false;
                     });
  return os;
}

class PrintSlicingVisitor : boost::static_visitor<std::ostream &> {
public:
  PrintSlicingVisitor(std::ostream &os) : m_os(os) {}

  template <typename T> std::ostream &operator()(T const &slicing) const {
    return (m_os << slicing);
  }

private:
  std::ostream &m_os;
};

std::ostream &operator<<(std::ostream &os, Slicing const &slicing) {
  os << "Slicing: ";
  return boost::apply_visitor(PrintSlicingVisitor(os), slicing);
}
}
}
