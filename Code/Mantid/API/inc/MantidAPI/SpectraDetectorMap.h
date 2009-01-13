#ifndef SPECTRADETECTORMAP_
#define SPECTRADETECTORMAP_

#include "MantidKernel/System.h"
#include "boost/shared_ptr.hpp"
#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

#include <vector>

//Forward declaration of IDetector
//class Mantid::Geometry::IDetector;
//class Mantid::API::Instrument;

namespace Mantid
{
namespace API
{
class Workspace;
/** @class SpectraDetectorMap SpectraDetectorMap.h

 SpectraDetectorMap provides a multimap between Spectra number (int)
 and IDetector*. For efficiency, an unordered_multimaop is used. The TR1/unordered_map
 header is not included in MVSC++ Express Edition so an alternative with multimap is
 provided.

 @author Laurent C Chapon, ISIS, RAL
 @date 29/04/2008

 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
class DLLExport SpectraDetectorMap
{
public:
#ifndef HAS_UNORDERED_MAP_H
  /// Spectra Detector map typedef
  typedef std::multimap<int,int> smap;
  /// Spectra Detector map iterator typedef
  typedef std::multimap<int,int>::const_iterator smap_it;
#else
  /// Spectra Detector map typedef
  typedef std::tr1::unordered_multimap<int,int> smap;
  /// Spectra Detector map iterator typedef
  typedef std::tr1::unordered_multimap<int,int>::const_iterator smap_it;
#endif
  ///Constructor
  SpectraDetectorMap(const Workspace* ws);
  ///virtual destructor
  virtual ~SpectraDetectorMap();
  /// populate the Map with _spec and _udet C array
  void populate(int* _spec, int* _udet, int nentries);
  /// Move a detector from one spectrum to another
  void remap(const int oldSpectrum, const int newSpectrum);
  /// Return number of detectors contributing to this spectrum
  const int ndet(const int spber) const;
  /// Get a vector of IDetector contributing to a spectrum
  std::vector<Geometry::IDetector_sptr> getDetectors(const int spectrum_number) const;
  /// Get a detector object (Detector or DetectorGroup) for the given spectrum number
  Geometry::IDetector_sptr getDetector(const int spectrum_number) const;
  /// Gets a list of spectra corresponding to a list of detector numbers
  std::vector<int> getSpectra(const std::vector<int>& detectorList) const;
  /// Return the size of the map
  int nElements() const {return _s2dmap->size();}
  /// Copy data from rhs.
  void copy(const SpectraDetectorMap& rhs);
private:
  ///Assignment operator
  SpectraDetectorMap& operator=(const SpectraDetectorMap& rhs);
  ///Copy Contructor
  SpectraDetectorMap(const SpectraDetectorMap& copy);
  /// insternal spectra detector map instance
  boost::shared_ptr<smap> _s2dmap;

  const Workspace* m_workspace;
  std::vector<int> getDetectorIDs(const int spectrum_number) const;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

/// Shared pointer to the SpectraDetectorMap
typedef boost::shared_ptr<SpectraDetectorMap> SpectraMap_sptr;
/// Shared pointer to the SpectraDetectorMap (const version)
typedef const boost::shared_ptr<const SpectraDetectorMap> SpectraMap_const_sptr;

} // Namespace API
} // Namespace Mantid

#endif /*SPECTRADETECTORMAP_*/
