#ifndef MANTIDNEXUSGEOMETRY_INSTRUMENTBUILDER_H
#define MANTIDNEXUSGEOMETRY_INSTRUMENTBUILDER_H

#include "MantidGeometry/Objects/IObject.h"
#include "MantidNexusGeometry/DllConfig.h"
#include "MantidNexusGeometry/TubeBuilder.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <memory>
#include <string>

namespace Mantid {
namespace Geometry {
class Instrument;
class IComponent;
class ICompAssembly;
}
namespace NexusGeometry {

/** InstrumentBuilder : Builder for wrapping the creating of a Mantid
  Instrument. Provides some useful abstractions over the full-blown Instrument
  interface

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_NEXUSGEOMETRY_DLL InstrumentBuilder {
public:
  /// Constructor creates the instrument
  explicit InstrumentBuilder(const std::string &instrumentName);
  /// Adds component to instrument
  Geometry::IComponent *addComponent(const std::string &compName,
                                     const Eigen::Vector3d &position);
  /// Adds tubes (ObjComponentAssemblies) to the last registered bank
  void addTubes(const std::string &bankName,
                const std::vector<detail::TubeBuilder> &tubes,
                boost::shared_ptr<const Mantid::Geometry::IObject> pixelShape);
  /// Adds detector to the root (instrument)
  void addDetectorToInstrument(
      const std::string &detName, int detId, const Eigen::Vector3d &position,
      boost::shared_ptr<const Mantid::Geometry::IObject> &shape);
  /// Adds detector to the last registered bank
  void addDetectorToLastBank(
      const std::string &detName, int detId,
      const Eigen::Vector3d &relativeOffset,
      boost::shared_ptr<const Mantid::Geometry::IObject> shape);
  /// Adds detector to instrument
  void addMonitor(const std::string &detName, int detId,
                  const Eigen::Vector3d &position,
                  boost::shared_ptr<const Mantid::Geometry::IObject> &shape);
  /// Add sample
  void addSample(const std::string &sampleName,
                 const Eigen::Vector3d &position);
  /// Add source
  void addSource(const std::string &sourceName,
                 const Eigen::Vector3d &position);

  void addBank(const std::string &localName, const Eigen::Vector3d &position,
               const Eigen::Quaterniond &rotation);

  /// Returns underlying instrument
  std::unique_ptr<const Geometry::Instrument> createInstrument() const;

private:
  /// Add a single tube to the last registed bank
  void doAddTube(const std::string &compName, const detail::TubeBuilder &tube,
                 boost::shared_ptr<const Mantid::Geometry::IObject> pixelShape);
  /// Sorts detectors
  void sortDetectors() const;
  /// Check that this instance is not locked
  void verifyMutable() const;
  /// product
  mutable std::unique_ptr<Geometry::Instrument> m_instrument;
  /// Last bank added. The instrument is the owner of the bank.
  Geometry::ICompAssembly *m_lastBank = nullptr;
  /// completed
  mutable bool m_finalized = false;
};
}
}
#endif // MANTIDNEXUSGEOMETRY_INSTRUMENTBUILDER_H
