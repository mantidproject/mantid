//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "MantidKernel/Exception.h"
#include <iostream>

namespace Mantid
{
  namespace Geometry
  {
    /**
     * Default constructor
     * @param start :: The starting spectra/detector ID (inclusive)
     * @param end :: The end spectra/detector ID (inclusive)
     */
    OneToOneSpectraDetectorMap::OneToOneSpectraDetectorMap(const specid_t start, const specid_t end)
      : m_start(start), m_end(end), m_current(start)
    {
    }
    
    /**
     * Get a vector of detectors ids contributing to a spectrum.
     * @param spectrumNo :: The spectrum number (unused)
     * @returns A vector containing the spectrumNumber itself
     * @throws std::out_of_range If the number is greater than the number
     * of elements in the map
     */
    std::vector<detid_t> 
    OneToOneSpectraDetectorMap::getDetectors(const specid_t spectrumNo) const
    {
      if( isValid(spectrumNo) )
      {
	return std::vector<detid_t>(1, detid_t(spectrumNo));
      }
      else
      {
	throw std::out_of_range("OneToOneSpectraDetectorMap::getDetectors - spectraNumber out of range");
      }
    }
    
    /**
     * Gets a list of spectra corresponding to a list of detector numbers
     * @param detectorList :: If the list contains elements that are within the map's range
     * then the element is returned
     * @throws std::invalid_argument if the supplied list contains a number outside the
     * range.
     */
    std::vector<specid_t> 
    OneToOneSpectraDetectorMap::getSpectra(const std::vector<detid_t>& detectorList) const
    {
      const size_t nElements(detectorList.size());
      std::vector<specid_t> spectra(nElements);
      for(size_t i = 0; i < nElements; ++i)
      {
	specid_t spectrumNo = static_cast<specid_t>(detectorList[i]);
	if( isValid(spectrumNo) )
	{
	  spectra[i] = spectrumNo;
	}
	else
	{
	  throw std::invalid_argument("OneToOneSpectraDetectorMap::getSpectra - ID out of range");
	}
      }
      return spectra;
    }

    /**
     * Setup the map for iteration from the beginning
     */
    void OneToOneSpectraDetectorMap::moveIteratorToStart() const
    {
      m_current = m_start;
    }
    /**
     * Returns whether a next element exists
     * @returns True if a call to advanceIterator leaves the iterator in range
     */
    bool OneToOneSpectraDetectorMap::hasNext() const
    {
      return (m_current < m_end);
    }
    /**
     * Advance the iterator to the next element
     */
    void OneToOneSpectraDetectorMap::advanceIterator() const
    {
      ++m_current;
    }
    /**
     * Returns the current element of the sequence
     * @returns The value currently pointed to by the iterator
     */
    inline specid_t OneToOneSpectraDetectorMap::getCurrentSpectrum() const
    {
      return m_current;
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------

    /**
     * Checks if the given spectrum is in range
     * @param spectrumNo :: A spectrum number
     * @returns True if the spectrum number is within the map's range, false otherwise
     */
    bool OneToOneSpectraDetectorMap::isValid(const specid_t spectrumNo) const
    {
      return (spectrumNo >= m_start && spectrumNo <= m_end);
    }

  }
}
