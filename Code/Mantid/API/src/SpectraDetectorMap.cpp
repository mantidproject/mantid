#include "MantidGeometry/DetectorGroup.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/SpectraDetectorMap.h"

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

SpectraDetectorMap::SpectraDetectorMap()
{}

SpectraDetectorMap::~SpectraDetectorMap()
{}

void SpectraDetectorMap::populate(int* _spectable, int* _udettable, int nentries, Instrument* instr)
{
  if (nentries<=0)
  {
    throw std::invalid_argument("Populate : number of entries should be >0");
  }
  IDetector* current;
  for (int i=0; i<nentries; ++i)
  {
    try
    {
      current=instr->getDetector(*_udettable); // Get the Detector associated to the current udet
    }
    catch(Kernel::Exception::NotFoundError& error) // No spectra association possible, continue loop

    {
      ++_spectable;
      ++_udettable;
      continue;
    }
    _s2dmap.insert(std::pair<int,IDetector*>(*_spectable,current)); // Insert current detector with Spectra number as key 
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
  std::vector<IDetector*> dets = getDetectors(oldSpectrum);
  // Add them to the map with the new spectrum number as the key
  std::vector<IDetector*>::const_iterator it;
  for (it = dets.begin(); it != dets.end(); ++it)
  {
    _s2dmap.insert( std::pair<int,IDetector*>(newSpectrum,*it) );
  }
  // Finally, remove the old spectrum number from the map
  _s2dmap.erase(oldSpectrum);
}

const int SpectraDetectorMap::ndet(const int spectrum_number) const
{
  return _s2dmap.count(spectrum_number);
}

std::vector<Geometry::IDetector*> SpectraDetectorMap::getDetectors(const int spectrum_number) const
{
  std::vector<Geometry::IDetector*> detectors;
  int ndets=ndet(spectrum_number);

  if (ndets<1)
  {
    // Will just return an empty vector
    return detectors;
  }
  std::pair<smap_it,smap_it> det_range=_s2dmap.equal_range(spectrum_number);
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
  int ndets=ndet(spectrum_number);
  if ( ndets == 0 )
  {
    g_log.error() << "Spectrum number " << spectrum_number << " not found" << std::endl;
    throw Kernel::Exception::NotFoundError("Spectrum number not found", spectrum_number);
  }
  else if ( ndets == 1) 
  {
    // If only 1 detector for the spectrum number, just return it
    // Have to create the shared pointer with a null deleter in this case so that the detector
    //   doesn't get deleted when the shared pointer goes out of scope
    return boost::shared_ptr<IDetector>(_s2dmap.find(spectrum_number)->second, Kernel::NullDeleter());
  }
  
  // Else need to construct a DetectorGroup and return that
  return boost::shared_ptr<IDetector>(new Geometry::DetectorGroup(getDetectors(spectrum_number)));
}

} // Namespace API 
} // Namespace Mantid
