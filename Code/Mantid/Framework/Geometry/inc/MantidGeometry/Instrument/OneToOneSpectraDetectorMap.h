#ifndef MANTID_GEOMETRY_ONETOONESPECTRADETECTORMAP_H_
#define MANTID_GEOMETRY_ONETOONESPECTRADETECTORMAP_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/ISpectraDetectorMap.h"

namespace Mantid
{
  namespace Geometry
  {
    /**
       OneToOneSpectraDetectorMap provides a simple 1:1 contiguous mapping between 
       spectra and detectors. This class cannot be used for 1->many mappings or disjointed
       mappings.

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
    class MANTID_GEOMETRY_DLL OneToOneSpectraDetectorMap : public ISpectraDetectorMap
    {
    public:
      /// Default constructor builds an empty map
      OneToOneSpectraDetectorMap();
      /// Constructor
      OneToOneSpectraDetectorMap(const specid_t start, const specid_t nelements);
      /// "Virtual copy constructor"
      OneToOneSpectraDetectorMap * clone() const;

      /** 
       * Return number of detectors contributing to this spectrum, always 1
       * @param spectrumNumber :: The spectrum number (unused)
       */
      inline std::size_t ndet(const specid_t spectrumNumber) const 
      { 
        UNUSED_ARG(spectrumNumber);
        return 1; 
      }
      /// Get a vector of detectors ids contributing to a spectrum.
      std::vector<detid_t> getDetectors(const specid_t spectrumNumber) const;
      /// Gets a list of spectra corresponding to a list of detector numbers
      std::vector<specid_t> getSpectra(const std::vector<detid_t>& detectorList) const;
      /** 
       * Return the size of the map
       * @returns The number of elements specified when the map was created
       */
      inline std::size_t nElements() const { return (m_end - m_start + 1); }
      /**
       * Returns the number of unique spectra in the map. In this case this always
       * matches <code>nElements()</code>
       * @returns The number of unique spectra in the map
       */
      inline std::size_t nSpectra() const { return nElements(); }

      /// Clear the map
      inline void clear() { m_end = 1; m_start = 2; }

      /// Create a map between a single ID & a list of IDs
      boost::shared_ptr<det2group_map> createIDGroupsMap() const;

      /**@name Iterate over the whole map */
      //@{
      /// Begin
      ISpectraDetectorMap::const_iterator cbegin() const;
      /// End
      ISpectraDetectorMap::const_iterator cend() const;
      //@}

    private:
      ///@cond
      class OneToOneProxy : public ISpectraDetectorMap::IteratorProxy
      {
      public:
        OneToOneProxy(const specid_t current)
          : m_current(current,current) {}
        inline void increment() { ++m_current.first; ++m_current.second; }
        inline const ISpectraDetectorMap::value_type & dereference() const 
        { 
          return m_current;
        }
        inline bool equals(const IteratorProxy* other) const 
        {
          const OneToOneProxy *otherOne = (const OneToOneProxy*)other;
          return (otherOne) ? otherOne->m_current == m_current: false;
        }
        /// "Copy constructor"
        virtual OneToOneProxy * clone() const { return new OneToOneProxy(m_current.first); }
      private:
        ISpectraDetectorMap::value_type m_current;
      };
      ///@endcond

    private:
      /// Checks if the given spectrum is in range
      bool isValid(const specid_t spectrumNo) const;

      /// The starting spectrum
      specid_t m_start;
      /// The end spectrum
      specid_t m_end;
    };

  } // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_ONETOONESPECTRADETECTORMAP_H_*/
