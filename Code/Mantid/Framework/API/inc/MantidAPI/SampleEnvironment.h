#ifndef MANTID_API_SAMPLEENVIRONMENT_H_
#define MANTID_API_SAMPLEENVIRONMENT_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/ClassMacros.h"

namespace Mantid {
namespace Geometry {
class Track;
}
namespace API {
/**
  This class stores details regarding the sample environment that was used
  during
  a specific run. It is implemented as a collection of pairs of Object elements

  Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL SampleEnvironment {
public:
  /// Constructor defining the name of the environment
  SampleEnvironment(const std::string &name);

  /// @return The name of kit
  inline const std::string name() const { return m_name; }
  /// @return The number of elements the environment is composed of
  inline size_t nelements() const { return m_elements.size(); }
  /// Return the bounding box of all of the elements
  Geometry::BoundingBox boundingBox() const;

  /// Add an element
  void add(const Geometry::Object &element);

  /// Is the point given a valid point within the environment
  bool isValid(const Kernel::V3D &point) const;
  /// Update the given track with intersections within the environment
  void interceptSurfaces(Geometry::Track &track) const;

private:
  DISABLE_DEFAULT_CONSTRUCT(SampleEnvironment)

  // Name of the kit
  std::string m_name;
  // The elements
  std::vector<Geometry::Object> m_elements;
};
}
}

#endif // MANTID_API_SAMPLEENVIRONMENT_H_
