#ifndef MANTID_GEOMETRY_ISPECTRADETECTORMAP_H_
#define MANTID_GEOMETRY_ISPECTRADETECTORMAP_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/IDetector.h" //For detid_t
#include <vector>

namespace Mantid
{

  /// Typedef for a spectrum index (ID)
  typedef int32_t specid_t;

  namespace Geometry
  {
    /**
       ISpectraDetectorMap provides an interface to define a mapping between spectrum number
       and detector ID(UDET).

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
    class DLLExport ISpectraDetectorMap
    {
    public:
      /// Virtual destructor
      virtual ~ISpectraDetectorMap() {}

      /// Return number of detectors contributing to this spectrum
      virtual std::size_t ndet(const specid_t spectrumNumber) const = 0;
      /// Get a vector of detectors ids contributing to a spectrum
      virtual std::vector<detid_t> getDetectors(const specid_t spectrumNumber) const = 0;
      /// Gets a list of spectra corresponding to a list of detector numbers
      virtual std::vector<specid_t> getSpectra(const std::vector<detid_t>& detectorList) const = 0;
      /// Return the size of the map
      virtual std::size_t nElements() const = 0;

      /**@name Iterate over the whole map */
      //@{
      /// Setup the map for iteration from the beginning
      virtual void moveIteratorToStart() const = 0;
      /// Returns whether a next element exists
      virtual bool hasNext() const = 0;
      /// Advance the iterator to the next element
      virtual void advanceIterator() const = 0 ;
      /// Returns the current element of the sequence
      virtual specid_t getCurrentSpectrum() const = 0;
      //@}
    };

  } // namespace Geoemetry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_ISPECTRADETECTORMAP_H_*/
