#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONALGORITHM_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONALGORITHM_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** SpecularReflectionAlgorithm : Algorithm base class implementing generic
 methods required for specular reflection calculations.
 *

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SpecularReflectionAlgorithm
    : public Mantid::API::DataProcessorAlgorithm {
protected:
  /// Constructor
  SpecularReflectionAlgorithm() = default;

  /// Get the surface sample component
  Mantid::Geometry::IComponent_const_sptr
  getSurfaceSampleComponent(Mantid::Geometry::Instrument_const_sptr inst) const;

  /// Get the detector component
  Mantid::Geometry::IComponent_const_sptr
  getDetectorComponent(Mantid::API::MatrixWorkspace_sptr workspace,
                       const bool isPointDetector) const;

  /// Does the property have a default value.
  bool isPropertyDefault(const std::string &propertyName) const;

  /// initialize common properties
  void initCommonProperties();

  /// Calculate the twoTheta angle w.r.t the direct beam
  double calculateTwoTheta() const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONALGORITHM_H_ */
