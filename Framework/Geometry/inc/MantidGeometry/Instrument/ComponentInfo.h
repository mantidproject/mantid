#ifndef MANTID_GEOMETRY_COMPONENTINFO_H_
#define MANTID_GEOMETRY_COMPONENTINFO_H_

#include "MantidBeamline/ComponentType.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/DateAndTime.h"
#include <boost/shared_ptr.hpp>
#include <unordered_map>
#include <vector>

namespace Mantid {

namespace Kernel {
class Quat;
class V3D;
} // namespace Kernel

namespace Geometry {
class IComponent;
class IObject;
} // namespace Geometry

namespace Beamline {
class ComponentInfo;
}

namespace Geometry {
class Instrument;

/** ComponentInfo : Provides a component centric view on to the instrument.
  Indexes are per component.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL ComponentInfo {
private:
  /// Pointer to the actual ComponentInfo object (non-wrapping part).
  std::unique_ptr<Beamline::ComponentInfo> m_componentInfo;
  /// Collection of component ids
  boost::shared_ptr<const std::vector<Geometry::IComponent *>> m_componentIds;
  /// Map of component ids to indexes
  boost::shared_ptr<const std::unordered_map<Geometry::IComponent *, size_t>>
      m_compIDToIndex;

  /// Shapes for each component
  boost::shared_ptr<std::vector<boost::shared_ptr<const Geometry::IObject>>>
      m_shapes;

  BoundingBox componentBoundingBox(const size_t index,
                                   const BoundingBox *reference) const;

  /// Private copy constructor. Do not make public.
  ComponentInfo(const ComponentInfo &other);
  void
  growBoundingBoxAsRectuangularBank(size_t index,
                                    const Geometry::BoundingBox *reference,
                                    Geometry::BoundingBox &mutableBB) const;
  void growBoundingBoxAsOutline(size_t index,
                                const Geometry::BoundingBox *reference,
                                Geometry::BoundingBox &mutableBB) const;

public:
  struct QuadrilateralComponent {
    size_t topLeft;
    size_t bottomLeft;
    size_t bottomRight;
    size_t topRight;
    size_t nX;
    size_t nY;
  };

  ComponentInfo(
      std::unique_ptr<Beamline::ComponentInfo> componentInfo,
      boost::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>>
          componentIds,
      boost::shared_ptr<
          const std::unordered_map<Geometry::IComponent *, size_t>>
          componentIdToIndexMap,
      boost::shared_ptr<std::vector<boost::shared_ptr<const Geometry::IObject>>>
          shapes);
  ~ComponentInfo();
  /// Copy assignment is not possible for ComponentInfo
  ComponentInfo &operator=(const ComponentInfo &) = delete;
  std::unique_ptr<ComponentInfo> cloneWithoutDetectorInfo() const;
  std::vector<size_t> detectorsInSubtree(size_t componentIndex) const;
  std::vector<size_t> componentsInSubtree(size_t componentIndex) const;
  const std::vector<size_t> &children(size_t componentIndex) const;
  size_t size() const;
  QuadrilateralComponent
  quadrilateralComponent(const size_t componentIndex) const;
  size_t indexOf(Geometry::IComponent *id) const;
  size_t indexOfAny(const std::string &name) const;
  bool isDetector(const size_t componentIndex) const;
  Kernel::V3D position(const size_t componentIndex) const;
  Kernel::V3D position(const std::pair<size_t, size_t> index) const;
  Kernel::Quat rotation(const size_t componentIndex) const;
  Kernel::Quat rotation(const std::pair<size_t, size_t> index) const;
  Kernel::V3D relativePosition(const size_t componentIndex) const;
  Kernel::Quat relativeRotation(const size_t componentIndex) const;
  void setPosition(size_t componentIndex, const Kernel::V3D &newPosition);
  void setRotation(size_t componentIndex, const Kernel::Quat &newRotation);
  void setPosition(const std::pair<size_t, size_t> index,
                   const Kernel::V3D &newPosition);
  void setRotation(const std::pair<size_t, size_t> index,
                   const Kernel::Quat &newRotation);
  size_t parent(const size_t componentIndex) const;
  bool hasParent(const size_t componentIndex) const;
  bool hasDetectorInfo() const;
  Kernel::V3D sourcePosition() const;
  Kernel::V3D samplePosition() const;
  bool hasSource() const;
  bool hasSample() const;
  size_t source() const;
  size_t sample() const;
  double l1() const;
  Kernel::V3D scaleFactor(const size_t componentIndex) const;
  const std::string &name(const size_t componentIndex) const;
  void setScaleFactor(const size_t componentIndex,
                      const Kernel::V3D &scaleFactor);
  size_t root() const;

  const IComponent *componentID(const size_t componentIndex) const {
    return m_componentIds->operator[](componentIndex);
  }
  bool hasValidShape(const size_t componentIndex) const;

  const Geometry::IObject &shape(const size_t componentIndex) const;

  double solidAngle(const size_t componentIndex,
                    const Kernel::V3D &observer) const;
  BoundingBox boundingBox(const size_t componentIndex,
                          const BoundingBox *reference = nullptr) const;
  Beamline::ComponentType componentType(const size_t componentIndex) const;
  void setScanInterval(const std::pair<int64_t, int64_t> &interval);
  // void setScanInterval(const std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>
        // &interval);
  void merge(const ComponentInfo &other);
  size_t scanSize() const;
  friend class Instrument;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTINFO_H_ */
