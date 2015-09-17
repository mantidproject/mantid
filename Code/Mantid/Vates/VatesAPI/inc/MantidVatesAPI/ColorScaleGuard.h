#ifndef MANTID_VATES_API_COLOR_SCALE_LOCK_H
#define MANTID_VATES_API_COLOR_SCALE_LOCK_H
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger g_log("ColorScaleGuard");
}

namespace Mantid {
namespace VATES {

class DLLExport ColorScaleLock {
public:
  ColorScaleLock() : m_isLocked(false){};
  ~ColorScaleLock() {}
  bool isLocked() { return m_isLocked; }
  void lock() { m_isLocked = true; }
  void unlock() { m_isLocked = false; }

private:
  bool m_isLocked;
};

class DLLExport ColorScaleLockGuard {
public:
  ColorScaleLockGuard(ColorScaleLock *lock) {
    if (lock == NULL || lock->isLocked()) {
      m_lock = NULL;
    } else {
      m_lock = lock;
      m_lock->lock();
    }
  }

  ~ColorScaleLockGuard() {
    if (m_lock != NULL) {
      m_lock->unlock();
    }
  }

private:
  ColorScaleLock *m_lock;
};
}
}
#endif
