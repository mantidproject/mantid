#include "MantidVatesAPI/MDLoadingViewSimple.h"

namespace Mantid {
namespace VATES {

void MDLoadingViewSimple::setTime(double time) { m_time = time; }

double MDLoadingViewSimple::getTime() const { return m_time; }

void MDLoadingViewSimple::setRecursionDepth(size_t recursionDepth) {
  m_recursionDepth = recursionDepth;
}

size_t MDLoadingViewSimple::getRecursionDepth() const {
  return m_recursionDepth;
}

void MDLoadingViewSimple::setLoadInMemory(bool loadInMemory) {
  m_loadInMemory = loadInMemory;
}

bool MDLoadingViewSimple::getLoadInMemory() const { return m_loadInMemory; }
} // namespace VATES
} // namespace Mantid
