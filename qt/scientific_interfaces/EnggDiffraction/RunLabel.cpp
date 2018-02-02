#include "RunLabel.h"

namespace MantidQt {
namespace CustomInterfaces {

RunLabel::RunLabel(const int _runNumber, const size_t _bank)
    : runNumber(_runNumber), bank(_bank) {}

bool operator==(const RunLabel &lhs, const RunLabel &rhs) {
  return lhs.bank == rhs.bank && lhs.runNumber == rhs.runNumber;
}

} // namespace CustomInterfaces
} // namespace MantidQt
