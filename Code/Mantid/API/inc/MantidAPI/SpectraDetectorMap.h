#ifndef SPECTRADETECTORMAP_
#define SPECTRADETECTORMAP_

#ifdef _WIN32
#include <map>
#else
#include <tr1/unordered_map>
#endif


//Forward declaration of IDetector
class Mantid::Geometry::IDetector; 
class Instrument;
//

namespace Mantid
{
	namespace API
	{
	/** @class SpectraDetectorMap SpectraDetectorMap.h
	 
	 SpectraDetectorMap provides a multimap between Spectra number (int)
	 and IDetector*. For efficiency, an unordered_multimaop is used. The TR1/unordered_map
	 header is not included in MVSC++ Express Edition so an alternative with multimap is 
	 provided.
	 
	 @author Laurent C Chapon, ISIS, RAL
	 @date 29/04/2008
	 
	 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories
	 
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
class SpectraDetectorMap
{
public:
#ifdef _WIN32
	typedef std::multimap<int,Mantid::Geometry::IDetector*> smap;
#else
	typedef std::tr1::unordered_multimap<int,Mantid::Geometry::IDetector*> smap;
#endif
	SpectraDetectorMap();
	SpectraDetectorMap(const SpectraDetectorMap& copy);
	SpectraDetectorMap& operator=(const SpectraDetectorMap& rhs);
	virtual ~SpectraDetectorMap();
	void populate(int* _spec, int* _udet, int nentries, Instrument*);
	/// Return number of detectors contributing to this spectra
	int ndet(int spectra_number) const;
private:
	smap _s2dmap;
};

	} // Namespace API 

} // Namespace Mantid

#endif /*SPECTRADETECTORMAP_*/
