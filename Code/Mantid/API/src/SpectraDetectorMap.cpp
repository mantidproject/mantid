#include "MantidGeometry/DetectorGroup.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/SpectraDetectorMap.h"

namespace Mantid
{
namespace API
{

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
  Mantid::Geometry::IDetector* current;
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
    _s2dmap.insert(std::pair<int,Mantid::Geometry::IDetector*>(*_spectable,current)); // Insert current detector with Spectra number as key 
    ++_spectable;
    ++_udettable;
  }
  return;
}

int SpectraDetectorMap::ndet(int spectrum_number) const
{
  return _s2dmap.count(spectrum_number);
}

void SpectraDetectorMap::getDetectors(int spectrum_number,std::vector<Mantid::Geometry::IDetector*>& detectors)
{
  int ndets=ndet(spectrum_number);

  if (ndets<1)
  {
    detectors.clear();
    return;
  }
  std::pair<smap_it,smap_it> det_range=_s2dmap.equal_range(spectrum_number);
  int i=0;
  detectors.resize(ndets);
  smap_it it;
  for (it=det_range.first; it!=det_range.second; ++it)
  {
    detectors[i++]=(*it).second;
  }
  return;
}

Geometry::IDetector* SpectraDetectorMap::getDetector(int spectrum_number)
{
  int ndets=ndet(spectrum_number);
  if ( ndets == 0 )
  {
    g_log.error() << "Spectrum number " << spectrum_number << " not found" << std::endl;
    throw Kernel::Exception::NotFoundError("Spectrum number not found", spectrum_number);
  }
  else if ( ndets == 1) 
  {
    // Just only 1 detector for the spectrum number, just return it
    return _s2dmap.find(spectrum_number)->second;
  }
  
  // Else need to construct a DetectorGroup and return that
  // @todo MEMORY LEAK ALERT!!!
  Geometry::DetectorGroup *group = new Geometry::DetectorGroup;
  std::pair<smap_it,smap_it> det_range=_s2dmap.equal_range(spectrum_number);
  for (smap_it it=det_range.first; it!=det_range.second; ++it)
  {
    group->addDetector( (*it).second );
  }
  return group;
}

} // Namespace API 
} // Namespace Mantid
