// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/MultiThreaded.h"
#include <map>

#include <mutex>

namespace Mantid {
namespace Kernel {
/** @class Cache Cache.h Kernel/Cache.h

Cache is a generic caching storage class.

@author Nick Draper, Tessella Support Services plc
@date 20/10/2009
*/
template <class KEYTYPE, class VALUETYPE> class MANTID_KERNEL_DLL Cache {
public:
  /// No-arg Constructor
  Cache() : m_cacheHit(0), m_cacheMiss(0), m_cacheMap(), m_mutex() {}

  /**
   * Copy constructor (mutex cannot be copied)
   * @param src The object that this object shall be constructed from.
   */
  Cache(const Cache<KEYTYPE, VALUETYPE> &src)
      : m_cacheHit(src.m_cacheHit), m_cacheMiss(src.m_cacheMiss), m_cacheMap(src.m_cacheMap),
        m_mutex() // New mutex which is unlocked
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
    std::lock_guard<std::mutex> lock(m_mutex);
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
    std::lock_guard<std::mutex> lock(m_mutex);
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
    std::lock_guard<std::mutex> lock(m_mutex);
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
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it_found = m_cacheMap.find(key);
    bool isValid = it_found != m_cacheMap.end();

    if (isValid) {
      value = it_found->second;
    }

    return isValid;
  }

  /// total number of times the cache has contained the requested information
  mutable int m_cacheHit;
  /// total number of times the cache has not contained the requested
  /// information
  mutable int m_cacheMiss;
  /// internal cache map
  std::map<KEYTYPE, VALUETYPE> m_cacheMap;
  /// internal mutex
  mutable std::mutex m_mutex;
  /// iterator typedef
  using CacheMapIterator = typename std::map<KEYTYPE, VALUETYPE>::iterator;
  /// const_iterator typedef
  using CacheMapConstIterator = typename std::map<KEYTYPE, VALUETYPE>::const_iterator;
};

} // namespace Kernel
} // namespace Mantid
