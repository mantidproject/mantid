#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/SpectraDetectorMap.h"

namespace Mantid
{
	namespace API
	{

SpectraDetectorMap::SpectraDetectorMap()
{
	
}
SpectraDetectorMap::~SpectraDetectorMap()
{
	
}
void SpectraDetectorMap::populate(int* _spectable,int* _udettable, int nentries, Instrument* instr)
{
	if (nentries<=0) 
	{
		throw std::invalid_argument("Populate : number of entries should be >0");
	}
	Mantid::Geometry::IDetector* current;
	for (int i=0;i<nentries;i++)
	{
		try
		{
		  current=instr->getDetector(*_udettable); // Get the Detector associated to the current udet
		}catch(Kernel::Exception::NotFoundError& error) // No spectra association possible, continue loop
		{
			_spectable++;
			_udettable++;
			continue;
		}
		_s2dmap.insert(std::pair<int,Mantid::Geometry::IDetector*>(*_spectable,current)); // Insert current detector with Spectra number as key 
		_spectable++;
		_udettable++;
	}
	return;
}

int SpectraDetectorMap::ndet(int spectra_number) const
{
	return _s2dmap.count(spectra_number);
}

void SpectraDetectorMap::getDetectors(int spectra_key,std::vector<Mantid::Geometry::IDetector*>& detectors)
{
	int ndets=ndet(spectra_key);
	
	if  (ndets<1)
	{
		detectors.empty();
		return;
	}
	std::pair<smap_it,smap_it> det_range=_s2dmap.equal_range(spectra_key); 
	int i=0;
	detectors.resize(ndets);
	smap_it it;
	for (it=det_range.first;it!=det_range.second;it++)
		detectors[i++]=(*it).second;
	return;
}
	
	
	} // Namespace API 

} // Namespace Mantid
