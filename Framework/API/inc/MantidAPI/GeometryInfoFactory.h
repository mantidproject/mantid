#ifndef MANTID_API_GEOMETRYINFOFACTORY_H_
#define MANTID_API_GEOMETRYINFOFACTORY_H_

#include <boost/shared_ptr.hpp>

#include "MantidKernel/V3D.h"
#include "MantidAPI/DllConfig.h"

#include <mutex>

namespace Mantid {

namespace Geometry {
class Instrument;
class IComponent;
}

namespace API {
class MatrixWorkspace;
class GeometryInfo;

/** Factory for GeometryInfo, see there for detailed information.

  @author Simon Heybrock, ESS

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_API_DLL GeometryInfoFactory {
public:
  GeometryInfoFactory(const MatrixWorkspace &workspace);

  /// Returns a GeometryInfo instance for workspace index "index".
  GeometryInfo create(const size_t index) const;

  /// Returns a reference to the instrument. The value is cached, so calling it
  /// repeatedly is cheap.
  const Geometry::Instrument &getInstrument() const;
  /// Returns a reference to the source component. The value is cached, so
  /// calling it repeatedly is cheap.
  const Geometry::IComponent &getSource() const;
  /// Returns a reference to the sample component. The value is cached, so
  /// calling it repeatedly is cheap.
  const Geometry::IComponent &getSample() const;
  /// Returns the source position. The value is cached, so calling it repeatedly
  /// is cheap.
  Kernel::V3D getSourcePos() const;
  /// Returns the sample position. The value is cached, so calling it repeatedly
  /// is cheap.
  Kernel::V3D getSamplePos() const;
  double getL1() const;

private:
  // These cache init functions are not thread-safe! Use only in combination
  // with std::call_once!
  void cacheSource() const;
  void cacheSample() const;
  void cacheL1() const;

  const MatrixWorkspace &m_workspace;
  boost::shared_ptr<const Geometry::Instrument> m_instrument;
  // The following variables are mutable, since they are initialized (cached)
  // only on demand, by const getters.
  mutable boost::shared_ptr<const Geometry::IComponent> m_source;
  mutable boost::shared_ptr<const Geometry::IComponent> m_sample;
  mutable Kernel::V3D m_sourcePos;
  mutable Kernel::V3D m_samplePos;
  mutable double m_L1;
  mutable std::once_flag m_sourceCached;
  mutable std::once_flag m_sampleCached;
  mutable std::once_flag m_L1Cached;
};
}
}

#endif /* MANTID_API_GEOMETRYINFOFACTORY_H_ */
