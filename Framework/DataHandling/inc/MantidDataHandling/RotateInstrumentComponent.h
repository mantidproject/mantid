// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
class DLLExport RotateInstrumentComponent : public API::DistributedAlgorithm {
public:
  /// Default constructor
  RotateInstrumentComponent();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RotateInstrumentComponent"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Rotates an instrument component."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"MoveInstrumentComponent", "SetInstrumentParameter"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Instrument"; }

private:
  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
