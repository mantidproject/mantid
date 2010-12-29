#ifndef MANTID_KERNEL_CACHE_H_
#define MANTID_KERNEL_CACHE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <map>
#include "MantidKernel/System.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid
{
  namespace Kernel
  {
    /** @class Cache Cache.h Kernel/Cache.h

    Cache is a generic caching storage class.

    @author Nick Draper, Tessella Support Services plc
    @date 20/10/2009

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    template< class KEYTYPE, class VALUETYPE >
    class DLLExport Cache
    {
    public:

      /// No-arg Constructor
      Cache():m_cacheHit(0),m_cacheMiss(0)
      {
      }

      ///Clears the cache
      void clear()
      {
        m_cacheHit = 0;
        m_cacheMiss = 0;
        m_cacheMap.clear();
      }

      ///The number of cache entries
      int size() {return static_cast<int>(m_cacheMap.size()); }

      ///total number of times the cache has contained the requested information
      int hitCount() { return m_cacheHit; }
      ///total number of times the cache has contained the requested information
      int missCount() { return m_cacheMiss; }
      ///total number of times the cache has contained the requested information/the total number of requests
      double hitRatio() 
      {
        double hitRatio = 0.0;
        if ((m_cacheHit+m_cacheMiss)>0)
        {
          hitRatio = 100.0*(m_cacheHit*1.0)/(m_cacheHit+m_cacheMiss);
        }
        return hitRatio; 
      }

      ///Sets a cached value on the rotation cache
      void setCache(const KEYTYPE& key, const VALUETYPE& value)
      {
        m_cacheMap[key] = value;
      }

      ///Attempts to retreive a value from the cache
      bool getCache(const KEYTYPE& key, VALUETYPE& value) const
      {
        bool found = getCacheNoStats(key,value);
        if (found) 
        {
          PARALLEL_ATOMIC
          m_cacheHit++;
        }
        else
        {
          PARALLEL_ATOMIC
          m_cacheMiss++;
        }
        return found;
      }

      ///removes the value associated with a key
      void removeCache(const KEYTYPE& key)
      {
        m_cacheMap.erase(key);
      }

    private:
      ///Attempts to retreive a value from the cache
      bool getCacheNoStats(const KEYTYPE key, VALUETYPE& value) const
      {
        CacheMapConstIterator it_found = m_cacheMap.find(key);
        if (it_found == m_cacheMap.end()) 
        {
          return false; // did not find the component
        }
        value = it_found->second;
        return true;
      }


      ///total number of times the cache has contained the requested information
      mutable int m_cacheHit;
      ///total number of times the cache has not contained the requested information
      mutable int m_cacheMiss;
      /// internal cache map
      std::map<const KEYTYPE,VALUETYPE > m_cacheMap;
      /// iterator typedef 
      typedef typename std::map<const KEYTYPE,VALUETYPE >::iterator CacheMapIterator;
      /// const_iterator typedef 
      typedef typename std::map<const KEYTYPE,VALUETYPE >::const_iterator CacheMapConstIterator;
    };

  } // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_CACHE_H_*/
