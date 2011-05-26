#include "MantidKernel/Exception.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <climits>
#include <iostream>

namespace Mantid
{
  using Geometry::ISpectraDetectorMap;

  namespace API
  {
    // Iterator end flag
    specid_t SpectraDetectorMap::g_iter_end = INT_MIN;

    // Get a reference to the logger
    Kernel::Logger& SpectraDetectorMap::g_log = Kernel::Logger::get("SpectraDetectorMap");

    /**
     * Default constrcutor
     */
    SpectraDetectorMap::SpectraDetectorMap() : m_s2dmap()
    {}
    
    /**
     * Constructor with ID tables
     * @param _spec :: An array of spectrum numbers
     * @param _udet :: An array of detector IDs
     * @param nentries :: The size of the _spec and _udet arrays
     */
    SpectraDetectorMap::SpectraDetectorMap(const specid_t* _spec, const detid_t* _udet, 
                                           int64_t nentries) : 
      m_s2dmap()
    {
      populate(_spec, _udet, nentries);
    }

    /**
     * Constructor with a vector of detector IDs
     * @param udetList :: A vector of detector IDs
     */
    SpectraDetectorMap::SpectraDetectorMap(const std::vector<detid_t>& udetList) 
      : m_s2dmap()
    {
      populateWithVector(udetList);
    }

    /**
     * Copy constructor
     * @param copy :: An object to copy from.  
     */
    SpectraDetectorMap::SpectraDetectorMap(const SpectraDetectorMap& copy) 
      : m_s2dmap(copy.m_s2dmap)
    {}

    /// Destructor
    SpectraDetectorMap::~SpectraDetectorMap()
    {}

    /// "Virtual copy constructor"
    SpectraDetectorMap * SpectraDetectorMap::clone() const
    {
      return new SpectraDetectorMap(*this);
    }

    /** Populate the map with 2 arrays; one detector per spectrum
     *
     * @param _spectable :: bare vector of the spectrum numbers
     * @param _udettable :: bare vector of the detector ids (same length as spectable)
     * @param nentries :: number of entries in the vectors
     */
    void SpectraDetectorMap::populate(const specid_t* _spectable, const detid_t* _udettable, int64_t nentries)
    {
      clear();
      if (nentries<=0)
      {
        g_log.error("Populate : number of entries should be > 0");
        throw std::invalid_argument("Populate : number of entries should be > 0");
      }
      for (int64_t i=0; i<nentries; ++i)
      {
        // Uncomment the line below to get a print out of the mapping as it's loaded
        // g_log.error() << *_spectable << " " << *_udettable << std::endl;
        m_s2dmap.insert(std::pair<specid_t,detid_t>(*_spectable,*_udettable)); // Insert current detector with Spectra number as key
        ++_spectable;
        ++_udettable;
      }
      return;
    }

    //------------------------------------------------------------------------------------------------
    /** Populate a simple spectrum-to-detector map, with a 1:1 correspondence
     *
     * @param start :: first spectrum number
     * @param end :: last spectrum number (not inclusive)
     */
    void SpectraDetectorMap::populateSimple(const detid_t start, const detid_t end)
    {
      clear();
      if (end<=start)
      {
        g_log.error("populateSimple : end should be > start");
        throw std::invalid_argument("populateSimple : end should be > start");
      }
      for (detid_t i=start; i<end; ++i)
      {
        m_s2dmap.insert(std::pair<specid_t,detid_t>(i,i)); // Insert current detector with Spectra number as key
      }
      return;
    }


    //------------------------------------------------------------------------------------------------
    /** Fill the SpectraDetectorMap with a simple list of pixel ids,
     * where the nth entry in the vector has a single detector, specified
     * by the value at that entry in the vector.
     * @param  udetList list of ints where the index = spectrum number; value = pixel ID.
     */
    void SpectraDetectorMap::populateWithVector(const std::vector<detid_t>& udetList)
    {
      specid_t size = static_cast<specid_t>(udetList.size());
      for (specid_t i=0; i < size; i++)
      {
        m_s2dmap.insert(std::pair<specid_t,detid_t>(i, udetList[i]));
      }
    }

    //------------------------------------------------------------------------------------------------
    /** Links a list of UDETs to the given spectrum.
     *  THIS METHOD SHOULD BE USED WITH CARE - IT CAN LEAD TO AN INCONSISTENT MAP
     *  @param spectrum :: The spectrum number to which detectors should be added
     *  @param udetList :: The list of detectors id's to add to the map
     */
    void SpectraDetectorMap::addSpectrumEntries(const specid_t spectrum, const std::vector<detid_t>& udetList)
    {
      std::vector<detid_t>::const_iterator it;
      for (it = udetList.begin(); it != udetList.end(); ++it)
      {
        m_s2dmap.insert(std::pair<specid_t,detid_t>(spectrum,*it));
      }
    }
    

    //------------------------------------------------------------------------------------------------
    /** Links a SET of detector IDs to the given spectrum.
     *  THIS METHOD SHOULD BE USED WITH CARE - IT CAN LEAD TO AN INCONSISTENT MAP
     *  @param spectrum :: The spectrum number to which detectors should be added
     *  @param detectorIDs :: The std::set of detectors id's to add to the map
     */
    void SpectraDetectorMap::addSpectrumEntries(const specid_t spectrum, const std::set<detid_t>& detectorIDs)
    {
      std::set<detid_t>::const_iterator it;
      for (it = detectorIDs.begin(); it != detectorIDs.end(); ++it)
      {
        m_s2dmap.insert(std::pair<specid_t,detid_t>(spectrum,*it));
      }
    }

    //------------------------------------------------------------------------------------------------
    /** Moves all detectors assigned to a particular spectrum number to a different one.
     *  Does nothing if the oldSpectrum number does not exist in the map.
     *  @param oldSpectrum :: The spectrum number to be removed and have its detectors reassigned
     *  @param newSpectrum :: The spectrum number to map the detectors to
     */
    void SpectraDetectorMap::remap(const specid_t oldSpectrum, const specid_t newSpectrum)
    {
      // Do nothing if the two spectrum numbers given are the same
      if (oldSpectrum == newSpectrum) return;
      // For now at least, the newSpectrum must already exist in the map
      if ( ndet(newSpectrum) == 0 )
      {
        g_log.error("Invalid value of newSpectrum. It is forbidden to create a new spectrum number with this method.");
        return;
      }
      // Get the list of detectors that contribute to the old spectrum
      std::vector<detid_t> dets = getDetectors(oldSpectrum);

      // Add them to the map with the new spectrum number as the key
      std::vector<detid_t>::const_iterator it;
      for (it = dets.begin(); it != dets.end(); ++it)
      {
        m_s2dmap.insert( std::pair<specid_t,detid_t>(newSpectrum,*it) );
      }
      // Finally, remove the old spectrum number from the map
      m_s2dmap.erase(oldSpectrum);
    }
    
    //------------------------------------------------------------------------------------------------
    /** Return the number of detectors for the given spectrum number
     * @param spectrum_number :: which spectrum number */
    std::size_t SpectraDetectorMap::ndet(const specid_t spectrum_number) const
    {
      return m_s2dmap.count(spectrum_number);
    }

    //------------------------------------------------------------------------------------------------
    /** Get a vector of detectors ids contributing to a spectrum
     * @param spectrum_number :: The # of the spectrum you are looking for.
     * @return list of detector ids in map
     */
    std::vector<detid_t> SpectraDetectorMap::getDetectors(const specid_t spectrum_number) const
    {
      const size_t ndets = ndet(spectrum_number);
      std::vector<detid_t> detectors;
      if ( ndets == 0 )
      {
        // Will just return an empty vector
        return detectors;
      }
      detectors.reserve(ndets);
      std::pair<smap_it,smap_it> det_range=m_s2dmap.equal_range(spectrum_number);
      for (smap_it it=det_range.first; it!=det_range.second; ++it)
      {
        detectors.push_back(it->second);
      }
      return detectors;
    }

    //------------------------------------------------------------------------------------------------
    /** Gets a list of spectra corresponding to a list of detector numbers.
    *  @param detectorList :: A list of detector Ids
    *  @return A vector where matching indices correspond to the relevant spectra id
    */
    std::vector<specid_t> SpectraDetectorMap::getSpectra(const std::vector<detid_t>& detectorList) const
    {
      std::vector<specid_t> spectraList;
      spectraList.reserve(detectorList.size());

      //invert the sdmap into a dsMap
      smap dsMap;  
      for (smap_it it = m_s2dmap.begin();  it != m_s2dmap.end(); ++it)
      {
        std::pair<specid_t,detid_t> valuePair(it->second,it->first);
        dsMap.insert(valuePair);
      }

      //use the dsMap to translate the detectorlist and populate the spectralist
      for (std::vector<detid_t>::const_iterator it = detectorList.begin();  it != detectorList.end(); ++it)
      {
        try
        {
          smap_it found = dsMap.find(*it);
          specid_t spectra = found != dsMap.end()? found->second : 0;
          spectraList.push_back(spectra);
        }
        catch (std::runtime_error&)
        {
          //spectra was not found enter 0
          spectraList.push_back(0);
        }
      }
      return spectraList;
    }

    //------------------------------------------------------------------------------------------------
    /**
     * Return an iterator pointing at the first element
     * @returns A ISpectraDetectorMap::const_iterator pointing at the first element
     */
    ISpectraDetectorMap::const_iterator SpectraDetectorMap::cbegin() const
    {
      smap_it beg = m_s2dmap.begin();
      if( beg == m_s2dmap.end() ) //Empty map?
      {
        return cend();
      }
      else
      {
        return ISpectraDetectorMap::const_iterator(this, std::make_pair(beg->first,beg->second));        
      }
    }

    /**
     * Return an iterator pointing at one past the element
     * @returns A ISpectraDetectorMap::const_iterator pointing at one past the 
     * last element
     */
    ISpectraDetectorMap::const_iterator SpectraDetectorMap::cend() const
    {
      return ISpectraDetectorMap::const_iterator(this, std::make_pair(g_iter_end, g_iter_end));
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------
    /**
     * Increment the given iterator by one
     * @param left :: A reference to the iterator to move
     */
    void SpectraDetectorMap::increment(ISpectraDetectorMap::const_iterator& left) const
    {
      smap_it itr = m_s2dmap.begin();
      std::advance(itr, left.increment_count);
      if( itr != m_s2dmap.end() )
      {
        left.item = *itr;
      }
      else
      {
        left.item = std::make_pair(g_iter_end,g_iter_end);        
      }
    }

  } // Namespace API 
} // Namespace Mantid
