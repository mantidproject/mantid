#ifndef MANTID_KERNEL_CACHE_H_
#define MANTID_KERNEL_CACHE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <map>
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace Kernel {
/** @class Cache Cache.h Kernel/Cache.h

Cache is a generic caching storage class.

@author Nick Draper, Tessella Support Services plc
@date 20/10/2009

Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
template <class KEYTYPE, class VALUETYPE> class DLLExport Cache {
public:
  /// No-arg Constructor
  Cache() : m_cacheHit(0), m_cacheMiss(0), m_cacheMap(), m_mutex() {}

  /**
   * Copy constructor (mutex cannot be copied)
   * @param src The object that this object shall be constructed from.
   */
  Cache(const Cache<KEYTYPE, VALUETYPE> &src)
      : m_cacheHit(src.m_cacheHit), m_cacheMiss(src.m_cacheMiss),
        m_cacheMap(src.m_cacheMap), m_mutex() // New mutex which is unlocked
  {}

  /**
   * Copy-assignment operator as we have a non-default copy constructor
   * @param rhs The object that is on the RHS of the assignment
   */
  Cache<KEYTYPE, VALUETYPE> &operator=(const Cache<KEYTYPE, VALUETYPE> &rhs) {
    if (this == &rhs)
      return *this; // handle self-assignment
    m_cacheHit = rhs.m_cacheHit;
    m_cacheMiss = rhs.m_cacheMiss;
    m_cacheMap = rhs.m_cacheMap;
    // mutex is untouched
    return *this;
  }

  /// Clears the cache
  void clear() {
    MutexLocker lock(m_mutex);
    m_cacheHit = 0;
    m_cacheMiss = 0;
    m_cacheMap.clear();
  }

  /// The number of cache entries
  int size() { return static_cast<int>(m_cacheMap.size()); }

  /// total number of times the cache has contained the requested information
  int hitCount() { return m_cacheHit; }
  /// total number of times the cache has contained the requested information
  int missCount() { return m_cacheMiss; }
  /// total number of times the cache has contained the requested
  /// information/the total number of requests
  double hitRatio() {
    double hitRatio = 0.0;
    if ((m_cacheHit + m_cacheMiss) > 0) {
      hitRatio = 100.0 * (m_cacheHit * 1.0) / (m_cacheHit + m_cacheMiss);
    }
    return hitRatio;
  }

  /**
   * Inserts/updates a cached value with the given key
   * @param key The key
   * @param value The new value for the key
   */
  void setCache(const KEYTYPE &key, const VALUETYPE &value) {
    MutexLocker lock(m_mutex);
    m_cacheMap[key] = value;
  }

  /**
   * Attempts to retrieve a value from the cache, with optional cache stats
   * tracking @see USE_CACHE_STATS compiler define
   * @param key The key for the requested value
   * @param value An output reference for the value, set to the curretn value if
   * found, otherwise it is untouched
   * @returns True if the value was found, false otherwise
   */
  bool getCache(const KEYTYPE &key, VALUETYPE &value) const {
#ifdef USE_CACHE_STATS
    bool found = getCacheNoStats(key, value);
    if (found) {
      PARALLEL_ATOMIC
      m_cacheHit++;
    } else {
      PARALLEL_ATOMIC
      m_cacheMiss++;
    }
    return found;
#else
    return getCacheNoStats(key, value);
#endif
  }

  /**
   * Attempts to remove a value from the cache. If the key does not exist, it
   * does nothing
   * @param key The key whose value should be removed
   */
  void removeCache(const KEYTYPE &key) {
    MutexLocker lock(m_mutex);
    m_cacheMap.erase(key);
  }

private:
  /**
   * Attempts to retrieve a value from the cache
   * @param key The key for the requested value
   * @param value An output reference for the value, set to the curretn value if
   * found, otherwise it is untouched
   * @returns True if the value was found, false otherwise
   */
  bool getCacheNoStats(const KEYTYPE key, VALUETYPE &value) const {
    MutexLocker lock(m_mutex);
    CacheMapConstIterator it_found = m_cacheMap.find(key);
    if (it_found == m_cacheMap.end()) {
      return false; // did not find the component
    }
    value = it_found->second;
    return true;
  }

  /// total number of times the cache has contained the requested information
  mutable int m_cacheHit;
  /// total number of times the cache has not contained the requested
  /// information
  mutable int m_cacheMiss;
  /// internal cache map
  std::map<const KEYTYPE, VALUETYPE> m_cacheMap;
  /// internal mutex
  mutable Poco::FastMutex m_mutex;
  /// typedef for Scoped Lock
  typedef Poco::FastMutex::ScopedLock MutexLocker;
  /// iterator typedef
  typedef typename std::map<const KEYTYPE, VALUETYPE>::iterator
      CacheMapIterator;
  /// const_iterator typedef
  typedef typename std::map<const KEYTYPE, VALUETYPE>::const_iterator
      CacheMapConstIterator;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_CACHE_H_*/
