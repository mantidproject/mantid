#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENT_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENT_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/Container.h"

namespace Mantid {
namespace Kernel {
class PseudoRandomNumberGenerator;
}
namespace Geometry {
class Track;

/**
  Defines a single instance of a SampleEnvironment. It houses a single can
  along with the other components specified by a SampleEnvironmentSpec.

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
class MANTID_GEOMETRY_DLL SampleEnvironment {
public:
  SampleEnvironment(std::string name, Container_const_sptr container);

  /// @return The name of kit
  inline const std::string name() const { return m_name; }
  /// @return The name of can
  inline const std::string containerID() const { return container()->id(); }
  /// @return A const ptr to the can instance
  inline Container_const_sptr container() const {
    return boost::static_pointer_cast<const Container>(m_components.front());
  }
  /// @return The number of elements the environment is composed of
  inline size_t nelements() const { return m_components.size(); }

  Geometry::BoundingBox boundingBox() const;
  /// Select a random point within a component
  Kernel::V3D generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                            const size_t maxAttempts) const;
  /// Select a random point within a component that is also bound by a
  /// given region
  Kernel::V3D generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                            const BoundingBox &activeRegion,
                            const size_t maxAttempts) const;
  bool isValid(const Kernel::V3D &point) const;
  int interceptSurfaces(Track &track) const;

  void add(const IObject_const_sptr &component);

private:
  std::string m_name;
  // Element zero is always assumed to be the can
  std::vector<IObject_const_sptr> m_components;
};

// Typedef a unique_ptr
using SampleEnvironment_uptr = std::unique_ptr<SampleEnvironment>;
// Typedef a unique_ptr to const
using SampleEnvironment_const_uptr = std::unique_ptr<const SampleEnvironment>;
} // namespace Geometry
} // namespace Mantid

#endif // MANTID_GEOMETRY_SAMPLEENVIRONMENT_H_
