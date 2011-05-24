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
    class DLLExport OneToOneSpectraDetectorMap : public ISpectraDetectorMap
    {
    public:
      /// Constructor
      OneToOneSpectraDetectorMap(const specid_t start, const specid_t nelements);

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

      /**@name Iterate over the whole map */
      //@{
      /// Setup the map for iteration from the beginning
      void moveIteratorToStart() const;
      /// Returns whether a next element exists
      bool hasNext() const;
      /// Advance the iterator to the next element
      void advanceIterator() const;
      /// Returns the current element of the sequence
      specid_t getCurrentSpectrum() const;
      //@}

    private:
      /// Checks if the given spectrum is in range
      bool isValid(const specid_t spectrumNo) const;

      /// The starting spectrum
      const specid_t m_start;
      /// The end spectrum
      const specid_t m_end;
      /// A 'pointer' to the current element
      mutable specid_t m_current;
    };

  } // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_ONETOONESPECTRADETECTORMAP_H_*/
