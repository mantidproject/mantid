#include "MantidGeometry/DetectorGroup.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <iostream>

namespace Mantid
{

  namespace Kernel
  {
    /// An object for constructing a shared_ptr that won't ever delete its pointee
    struct NullDeleter
    {
      /// Does nothing
      void operator()(void const *) const
      {}  
    };
  } // namespace Kernel

  namespace API
  {

    using Geometry::IDetector;

    // Get a reference to the logger
    Kernel::Logger& SpectraDetectorMap::g_log = Kernel::Logger::get("SpectraDetectorMap");

    SpectraDetectorMap::SpectraDetectorMap(const Workspace* ws)
        :m_workspace(ws),_s2dmap(new smap)
    {
    }

/*    SpectraDetectorMap::SpectraDetectorMap(const SpectraDetectorMap& copy)
    {
      _s2dmap.insert(copy._s2dmap.begin(),copy._s2dmap.end());
    }*/

    SpectraDetectorMap::~SpectraDetectorMap()
    {}

    void SpectraDetectorMap::populate(int* _spectable, int* _udettable, int nentries)
    {
        _s2dmap.reset(new smap());
        boost::shared_ptr<IInstrument> instr = m_workspace->getInstrument();
        if (!instr)
            throw std::runtime_error("Instrument iz zero!");
      if (nentries<=0)
      {
        throw std::invalid_argument("Populate : number of entries should be >0");
      }
      boost::shared_ptr<IDetector> current;
      bool warn = true;
      for (int i=0; i<nentries; ++i)
      {
        try
        {
          current=instr->getDetector(*_udettable); // Get the Detector associated to the current udet
        }
        catch(Kernel::Exception::NotFoundError& error) // No spectra association possible, continue loop

        {
          if (warn)
          {
            g_log.warning("Some detectors not found in instrument definition");
            warn = false;  // Just print this warning once.
          }
          ++_spectable;
          ++_udettable;
          continue;
        }
        _s2dmap->insert(std::pair<int,int>(*_spectable,*_udettable)); // Insert current detector with Spectra number as key 
        ++_spectable;
        ++_udettable;
      }
      return;
    }

    /** Moves all detectors assigned to a particular spectrum number to a different one.
    *  Does nothing if the oldSpectrum number does not exist in the map.
    *  @param oldSpectrum The spectrum number to be removed and have its detectors reassigned
    *  @param newSpectrum The spectrum number to map the detectors to
    *  @todo Write a test for this method
    */
    void SpectraDetectorMap::remap(const int oldSpectrum, const int newSpectrum)
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
      std::vector<int> dets = getDetectorIDs(oldSpectrum);

      // Add them to the map with the new spectrum number as the key
      std::vector<int>::const_iterator it;
      for (it = dets.begin(); it != dets.end(); ++it)
      {
        _s2dmap->insert( std::pair<int,int>(newSpectrum,*it) );
      }
      // Finally, remove the old spectrum number from the map
      _s2dmap->erase(oldSpectrum);
    }

    const int SpectraDetectorMap::ndet(const int spectrum_number) const
    {
      return _s2dmap->count(spectrum_number);
    }

    std::vector<boost::shared_ptr<Geometry::IDetector> > SpectraDetectorMap::getDetectors(const int spectrum_number) const
    {
      std::vector<boost::shared_ptr<Geometry::IDetector> > detectors;
      int ndets=ndet(spectrum_number);

      if (ndets<1)
      {
        // Will just return an empty vector
        return detectors;
      }
      if (!m_workspace)
          throw std::runtime_error("SpectraDetectorMap has not been populated.");
      boost::shared_ptr<IInstrument> instr = m_workspace->getInstrument();
      std::pair<smap_it,smap_it> det_range=_s2dmap->equal_range(spectrum_number);
      for (smap_it it=det_range.first; it!=det_range.second; ++it)
      {
        detectors.push_back(instr->getDetector(it->second));
      }
      return detectors;
    }

    std::vector<int> SpectraDetectorMap::getDetectorIDs(const int spectrum_number) const
    {
      std::vector<int> detectors;
      int ndets=ndet(spectrum_number);

      if (ndets<1)
      {
        // Will just return an empty vector
        return detectors;
      }
      std::pair<smap_it,smap_it> det_range=_s2dmap->equal_range(spectrum_number);
      for (smap_it it=det_range.first; it!=det_range.second; ++it)
      {
        detectors.push_back(it->second);
      }
      return detectors;
    }

    /** Get the effective detector for this spectrum number.
    *  @param spectrum_number The spectrum number for which the detector is required
    *  @return A single detector object representing the detector(s) contributing
    *  to the given spectrum number. If more than one detector contributes then
    *  the returned object's concrete type will be DetectorGroup.
    *  @todo Write a test for this method
    */
    boost::shared_ptr<Geometry::IDetector> SpectraDetectorMap::getDetector(const int spectrum_number) const
    {
      if (!m_workspace)
          throw std::runtime_error("SpectraDetectorMap has not been populated.");
      int ndets=ndet(spectrum_number);
      if ( ndets == 0 )
      {
        g_log.debug() << "Spectrum number " << spectrum_number << " not found" << std::endl;
        throw Kernel::Exception::NotFoundError("Spectrum number not found", spectrum_number);
      }
      else if ( ndets == 1) 
      {
        // If only 1 detector for the spectrum number, just return it
        // Have to create the shared pointer with a null deleter in this case so that the detector
        //   doesn't get deleted when the shared pointer goes out of scope
          int id = _s2dmap->find(spectrum_number)->second;
          return m_workspace->getInstrument()->getDetector(id);
      }

          std::vector<boost::shared_ptr<Geometry::IDetector> > tmp = getDetectors(spectrum_number);
      // Else need to construct a DetectorGroup and return that
      return boost::shared_ptr<IDetector>(new Geometry::DetectorGroup(getDetectors(spectrum_number)));
    }

    /** Gets a list of spectra corresponding to a list of detector numbers.
    *  @param detectorList A list of detector Ids
    *  @return A vexctor where matching indices correspond to the relevant spectra id
    */
    std::vector<int> SpectraDetectorMap::getSpectra(const std::vector<int>& detectorList) const
    {
      std::vector<int> spectraList;

      //invert the sdmap into a dsMap
      std::multimap<int,int> dsMap;  
      for (smap_it it = _s2dmap->begin();  it != _s2dmap->end(); ++it)
      {
        std::pair<int,int> valuePair(it->second,it->first);
        dsMap.insert(valuePair);
      }

      //use the dsMap to translate the detectorlist and populate the spectralist
      for (std::vector<int>::const_iterator it = detectorList.begin();  it != detectorList.end(); ++it)
      {
        try
        {
            std::multimap<int,int>::iterator found = dsMap.find(*it);
            int spectra = found != dsMap.end()? found->second : 0;
            spectraList.push_back(spectra);
        }
        catch (std::runtime_error& ex)
        {
          //spectra was not found enter 0
          spectraList.push_back(0);
        }
      }
      return spectraList;
    }

    /**
        Copies detector ids from rhs spectra map.
    */
    void SpectraDetectorMap::copy(const SpectraDetectorMap& rhs)
    {
        _s2dmap = rhs._s2dmap;
    }

  } // Namespace API 
} // Namespace Mantid
