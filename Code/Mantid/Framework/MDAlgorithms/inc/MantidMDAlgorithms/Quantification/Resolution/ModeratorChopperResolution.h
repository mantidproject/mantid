#ifndef MANTID_MDALGORITHMS_MODERATORCHOPPERRESOLUTION_H_
#define MANTID_MDALGORITHMS_MODERATORCHOPPERRESOLUTION_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/ClassMacros.h"

#include <boost/shared_ptr.hpp>

namespace Mantid {
//
// Forward declarations
//
namespace API {
class ChopperModel;
class ExperimentInfo;
class ModeratorModel;
}

namespace MDAlgorithms {
class CachedExperimentInfo;
/**
 *
 * Calculates the energy resolution for a moderator-chopper
 * combination from a given observation
 *
 */
class DLLExport ModeratorChopperResolution {
public:
  /// Constructor
  ModeratorChopperResolution(const CachedExperimentInfo &observation);

  /// Destructor
  ~ModeratorChopperResolution();

  /// Return a width in energy for the model
  double energyWidth(const double deltaE) const;

private:
  DISABLE_DEFAULT_CONSTRUCT(ModeratorChopperResolution);
  DISABLE_COPY_AND_ASSIGN(ModeratorChopperResolution);

  /// Store required cached variables
  void initCaches();

  /// A reference to the observation object
  const CachedExperimentInfo &m_observation;
  /// A pointer to the moderator object
  boost::shared_ptr<const API::ModeratorModel> m_moderator;
  /// A pointer to the chopper object
  boost::shared_ptr<const API::ChopperModel> m_chopper;

  /// Moderator-chopper distance
  double m_modChopDist;
  /// ChopperModel-sample distance
  double m_chopSampleDist;
};
}
}

#endif /* MANTID_MDALGORITHMS_MODERATORCHOPPERRESOLUTION_H_ */
