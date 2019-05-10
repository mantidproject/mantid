// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunLabel.h"

namespace MantidQt {
namespace CustomInterfaces {

RunLabel::RunLabel(const std::string _runNumber, const size_t _bank)
    : runNumber(_runNumber), bank(_bank) {}

bool operator==(const RunLabel &lhs, const RunLabel &rhs) {
  return lhs.bank == rhs.bank && lhs.runNumber == rhs.runNumber;
}

bool operator!=(const RunLabel &lhs, const RunLabel &rhs) {
  return !(lhs == rhs);
}

bool operator<(const RunLabel &lhs, const RunLabel &rhs) {
  return lhs.runNumber < rhs.runNumber ||
         (lhs.runNumber == rhs.runNumber && lhs.bank < rhs.bank);
}

} // namespace CustomInterfaces
} // namespace MantidQt
