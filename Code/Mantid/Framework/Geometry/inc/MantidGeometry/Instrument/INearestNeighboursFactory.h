#ifndef MANTID_GEOMETRY_INEARESTNEIGHBOURSFACTORY_H_
#define MANTID_GEOMETRY_INEARESTNEIGHBOURSFACTORY_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/INearestNeighbours.h"

namespace Mantid
{
  namespace Geometry
  {

    /** INearestNeighboursFactory : Abstract class for creating INearestNeighbour products/objects. This implements a factory method.

    @date 2011-11-24

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class MANTID_GEOMETRY_DLL INearestNeighboursFactory
    {
    public:
      /// Factory method
      virtual INearestNeighbours* create(boost::shared_ptr<const Instrument> instrument,
        const ISpectrumDetectorMapping & spectraMap, bool ignoreMasked=false) = 0;
      /// Factory method
      virtual INearestNeighbours* create(int numberOfNeighbours, boost::shared_ptr<const Instrument> instrument,
        const ISpectrumDetectorMapping & spectraMap, bool ignoreMasked=false) = 0;
      /// Destructor
      virtual ~INearestNeighboursFactory(){};
    };


  } // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_NEARESTNEIGHBOURSFACTORY_H_ */
