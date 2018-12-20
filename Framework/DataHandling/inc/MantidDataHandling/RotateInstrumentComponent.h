#ifndef MANTID_DATAHANDLING_ROTATEINSTRUMENTCOMPONENT_H_
#define MANTID_DATAHANDLING_ROTATEINSTRUMENTCOMPONENT_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidGeometry/Instrument/Component.h"

namespace Mantid {

namespace DataHandling {
/** @class RotateInstrumentComponent RotateInstrumentComponent.h
DataHandling/RotateInstrumentComponent.h

Rotates an instrument component to a new orientation by setting corresponding
parameter ("rot") in ParameterMap.

Required Properties:
<UL>
<LI> Workspace - The workspace to which the change will apply </LI>
<LI> ComponentName - The name of the component which will be rotated </LI>
<LI> DetectorID - The detector id of the component to rotate. Either
ComponentName or DetectorID
     can be used to identify the component. If both are given the DetectorID
will bw used.
</LI>
<LI> X - New x coordinate of the rotation axis in the coordinate system attached
to the component.</LI>
<LI> Y - New y coordinate of the rotation axis in the coordinate system attached
to the component.</LI>
<LI> Z - New z coordinate of the rotation axis in the coordinate system attached
to the component.</LI>
<LI> Angle - The angle of rotation in degrees.</LI>
</UL>

@author Roman Tolchenov, Tessella Support Services plc
@date 21/01/2009

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport RotateInstrumentComponent : public API::DistributedAlgorithm {
public:
  /// Default constructor
  RotateInstrumentComponent();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override {
    return "RotateInstrumentComponent";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rotates an instrument component.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"MoveInstrumentComponent", "SetInstrumentParameter"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_ROTATEINSTRUMENTCOMPONENT_H_*/
