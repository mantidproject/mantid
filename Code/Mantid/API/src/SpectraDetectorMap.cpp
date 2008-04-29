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


	} // Namespace API 

} // Namespace Mantid
