#ifndef MANTID_GEOMETRY_NEARESTNEIGHBOURSFACTORY_H_
#define MANTID_GEOMETRY_NEARESTNEIGHBOURSFACTORY_H_

#include "MantidGeometry/Instrument/INearestNeighboursFactory.h"
#include "MantidGeometry/Instrument/NearestNeighbours.h"

namespace Mantid {
namespace Geometry {

/** NearestNeighboursFactory : Implementation of INearestNeighbours factory
  returning a NearestNeighbours object upon create.

  @date 2011-11-24

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class MANTID_GEOMETRY_DLL NearestNeighboursFactory
    : public INearestNeighboursFactory {
public:
  /// Constructor
  NearestNeighboursFactory();
  /// Factory Method
  NearestNeighbours *create(boost::shared_ptr<const Instrument> instrument,
                            const ISpectrumDetectorMapping &spectraMap,
                            bool ignoreMasked = false);
  /// Factory Method
  NearestNeighbours *create(int numberOfNeighbours,
                            boost::shared_ptr<const Instrument> instrument,
                            const ISpectrumDetectorMapping &spectraMap,
                            bool ignoreMasked = false);
  /// Destructor
  virtual ~NearestNeighboursFactory();
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_NEARESTNEIGHBOURSFACTORY_H_ */
