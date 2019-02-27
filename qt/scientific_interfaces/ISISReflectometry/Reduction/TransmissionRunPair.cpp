// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "TransmissionRunPair.h"
#include <boost/algorithm/string/join.hpp>

namespace MantidQt {
namespace CustomInterfaces {

TransmissionRunPair::TransmissionRunPair()
    : m_firstTransmissionRunNumbers(), m_secondTransmissionRunNumbers() {}

TransmissionRunPair::TransmissionRunPair( // cppcheck-suppress passedByValue
    std::string firstTransmissionRun,
    // cppcheck-suppress passedByValue
    std::string secondTransmissionRun)
    : m_firstTransmissionRunNumbers{std::move(firstTransmissionRun)},
      m_secondTransmissionRunNumbers{std::move(secondTransmissionRun)} {}

TransmissionRunPair::TransmissionRunPair(
    // cppcheck-suppress passedByValue
    std::vector<std::string> firstTransmissionRunNumbers,
    // cppcheck-suppress passedByValue
    std::vector<std::string> secondTransmissionRunNumbers)
    : m_firstTransmissionRunNumbers(std::move(firstTransmissionRunNumbers)),
      m_secondTransmissionRunNumbers(std::move(secondTransmissionRunNumbers)) {}

std::vector<std::string> const &
TransmissionRunPair::firstTransmissionRunNumbers() const {
  return m_firstTransmissionRunNumbers;
}
std::vector<std::string> const &
TransmissionRunPair::secondTransmissionRunNumbers() const {
  return m_secondTransmissionRunNumbers;
}

std::string TransmissionRunPair::firstRunList() const {
  return boost::algorithm::join(m_firstTransmissionRunNumbers, ", ");
}

std::string TransmissionRunPair::secondRunList() const {
  return boost::algorithm::join(m_secondTransmissionRunNumbers, ", ");
}

bool operator==(TransmissionRunPair const &lhs,
                TransmissionRunPair const &rhs) {
  return lhs.firstTransmissionRunNumbers() ==
             rhs.firstTransmissionRunNumbers() &&
         lhs.secondTransmissionRunNumbers() ==
             rhs.secondTransmissionRunNumbers();
}

bool operator!=(TransmissionRunPair const &lhs,
                TransmissionRunPair const &rhs) {
  return !(lhs == rhs);
}

} // namespace CustomInterfaces
} // namespace MantidQt
