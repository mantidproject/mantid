#ifndef MANTID_GEOMETRY_ISPECTRADETECTORMAP_H_
#define MANTID_GEOMETRY_ISPECTRADETECTORMAP_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace Mantid
{
  /// Map single det ID of group to its members
  typedef std::map<detid_t, std::vector<detid_t> > det2group_map;

  namespace Geometry
  {
    /**
       ISpectraDetectorMap provides an interface to define a mapping between spectrum number
       and detector ID(UDET).

       Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class MANTID_GEOMETRY_DLL ISpectraDetectorMap
    {
    public:
      /// Typedef for the element type
      typedef std::pair<specid_t, detid_t> value_type;

      ///@cond
      class IteratorProxy
      {
      public:
        virtual ~IteratorProxy() {}
        /// Increment the iterator
        virtual void increment() = 0;
        /// Dereference
        virtual const value_type & dereference() const = 0;
        /// Equality
        virtual bool equals(const IteratorProxy* other) const = 0;
        /// "Copy constructor"
        virtual IteratorProxy * clone() const = 0;
      };     

      class const_iterator //Deliberately breaks Mantid naming conventions for consistency with STL
      {
      public:
        // Iterator traits
        typedef size_t difference_type;
        typedef ISpectraDetectorMap::value_type value_type;
        typedef const value_type* pointer;
        typedef const value_type& reference;
        typedef std::forward_iterator_tag iterator_category;

      public:
        const_iterator() : m_proxy(NULL) {}
        const_iterator(IteratorProxy *itr_proxy)
          : m_proxy(itr_proxy) {}
        ~const_iterator() { delete m_proxy; }
        const_iterator(const const_iterator & other)
        {
          this->operator=(other);
        }
        const_iterator& operator=(const const_iterator& rhs) 
        {
          if( &rhs != this )
          {
            m_proxy = rhs.m_proxy->clone();
          }
          return *this;
        }
        ///Prefix
        const_iterator& operator++()
        {
          m_proxy->increment();
          return *this;
        }
        /// Postfix
        const_iterator operator++(int)
        {
          const_iterator before = *this;
          this->operator++();
          return before;
        }
        // Return the current value
        const value_type & operator*() const
        {
          return m_proxy->dereference();
        }
        // Return a pointer to the current value 
        const value_type* operator->() const
        {
          return &m_proxy->dereference();
        }
        // Comparison operator
        bool operator==(const ISpectraDetectorMap::const_iterator &rhs) const
        {
          return m_proxy->equals(rhs.m_proxy);
        }
        // Comparison operator
        bool operator!=(const ISpectraDetectorMap::const_iterator &rhs) const
        {
          return !(*this==rhs);
        }
      private:
        // A pointer to the proxy that holds the actual iterator
        IteratorProxy * m_proxy;
      };
      ///@endcond
      
    public:
      /// Virtual destructor
      virtual ~ISpectraDetectorMap() {}
      /// "Virtual copy constructor"
      virtual ISpectraDetectorMap * clone() const = 0;

      /// Return number of detectors contributing to this spectrum
      virtual std::size_t ndet(const specid_t spectrumNumber) const = 0;
      /// Get a vector of detectors ids contributing to a spectrum
      virtual std::vector<detid_t> getDetectors(const specid_t spectrumNumber) const = 0;
      /// Gets a list of spectra corresponding to a list of detector numbers
      virtual std::vector<specid_t> getSpectra(const std::vector<detid_t>& detectorList) const = 0;
      /// Return the size of the map
      virtual std::size_t nElements() const = 0;
      /// Returns the number of unique spectra in the map
      virtual std::size_t nSpectra() const = 0;
      /// Clear the map
      virtual void clear() = 0;

      /// Create a map between a single ID & a list of ID having the same spectrum number
      virtual boost::shared_ptr<det2group_map> createIDGroupsMap() const = 0;

      /**@name Iterate over the whole map */
      //@{
      /// Begin
      virtual const_iterator cbegin() const = 0;
      /// End
      virtual const_iterator cend() const = 0;
      //@}

      /// begin method to make type more like a STL container. required so that this type can be used with gmock printers.
      const_iterator begin() const
      {
        return cbegin();
      }

      /// end method to make type more like a STL container. required so that this type can be used with gmock printers.
      const_iterator end() const
      {
        return cend();
      }

    };

    /**
     * Equality test for two objects implementing the ISpectraDetectorMap interface. 
     * @param lhs A reference to the lhs
     * @param rhs A reference to the rhs
     * @returns True if the objects are considered equal, false otherwise
     */
    inline bool operator==(const ISpectraDetectorMap& lhs, const ISpectraDetectorMap& rhs)
    {
      // Quick return
      if( lhs.nElements() != rhs.nElements() ) return false;
      ISpectraDetectorMap::const_iterator l_end = lhs.cend();
      ISpectraDetectorMap::const_iterator l_itr = lhs.cbegin();
      ISpectraDetectorMap::const_iterator r_itr = rhs.cbegin();
      for( ; l_itr != l_end; ++l_itr, ++r_itr )
      {
        if( *l_itr != *r_itr ) return false;
      }
      return true;
    }
    /**
     * Inequality test for two objects of type ISpectraDetectorMap
     * @param lhs A reference to the lhs
     * @param rhs A reference to the rhs
     * @returns True if the objects are not considerd equal, false otherwise
     */
    inline bool operator!=(const ISpectraDetectorMap& lhs, const ISpectraDetectorMap& rhs)
    {
      return !(lhs==rhs);
    }

  } // namespace Geoemetry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_ISPECTRADETECTORMAP_H_*/
