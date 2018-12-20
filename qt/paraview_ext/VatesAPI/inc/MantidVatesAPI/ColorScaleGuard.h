#ifndef MANTID_VATES_API_COLOR_SCALE_LOCK_H
#define MANTID_VATES_API_COLOR_SCALE_LOCK_H
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace VATES {

class DLLExport ColorScaleLock {
public:
  bool isLocked() { return m_isLocked; }
  void lock() { m_isLocked = true; }
  void unlock() { m_isLocked = false; }

private:
  bool m_isLocked{false};
};

class DLLExport ColorScaleLockGuard {
public:
  ColorScaleLockGuard(ColorScaleLock *lock) {
    if (!lock || lock->isLocked()) {
      m_lock = nullptr;
    } else {
      m_lock = lock;
      m_lock->lock();
    }
  }

  ~ColorScaleLockGuard() {
    if (m_lock) {
      m_lock->unlock();
    }
  }

private:
  ColorScaleLock *m_lock;
};
} // namespace VATES
} // namespace Mantid
#endif
