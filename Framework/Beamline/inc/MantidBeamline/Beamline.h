#ifndef MANTID_BEAMLINE_BEAMLINE_H_
#define MANTID_BEAMLINE_BEAMLINE_H_

#include "MantidBeamline/DllConfig.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Beamline {

class ComponentInfo;
class DetectorInfo;
/** Beamline : Also known as Instrument 2.0

  This is the top-level object for accessing ComponentInfo, DetectorInfo and
  modern beamline functionality.

  Since ComponentInfo and DetectorInfo have non-owning pointers to each other,
  this type ensures
  that client code is not exposed to creation or setup internals. Beamline owns
  both ComponentInfo and DetectorInfo.

  Beamline is deliberately trivial to copy.

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
class MANTID_BEAMLINE_DLL Beamline {
public:
  Beamline();
  Beamline(const Beamline &other);
  Beamline &operator=(const Beamline &other);
  Beamline(Beamline &&) = default;
  Beamline &operator=(Beamline &&) = default;
  Beamline(ComponentInfo &&componentInfo, DetectorInfo &&detectorInfo);
  const ComponentInfo &componentInfo() const;
  const DetectorInfo &detectorInfo() const;
  ComponentInfo &mutableComponentInfo();
  DetectorInfo &mutableDetectorInfo();
  Beamline alias();
  bool empty() const;

private:
  // For internal use only.
  Beamline(boost::shared_ptr<ComponentInfo> &componentInfo,
           boost::shared_ptr<DetectorInfo> &detectorInfo);
  bool m_empty = true;
  boost::shared_ptr<ComponentInfo> m_componentInfo;
  boost::shared_ptr<DetectorInfo> m_detectorInfo;
};

} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_BEAMLINE_H_ */
