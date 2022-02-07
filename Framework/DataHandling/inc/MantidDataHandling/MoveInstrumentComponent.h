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
/** @class MoveInstrumentComponent MoveInstrumentComponent.h
DataHandling/MoveInstrumentComponent.h

Moves an instrument component to a new position by setting corresponding
parameter in ParameterMap.

Required Properties:
<UL>
<LI> Workspace - The workspace to which the change will apply </LI>
<LI> ComponentName - The name of the component which will be moved. A pathname
delited by '/' may be used for non-unique name. </LI>
<LI> DetectorID - The detector id of the component to move. Either ComponentName
or DetectorID
     can be used to identify the component. If both are given the DetectorID
will bw used.
</LI>
<LI> X - New x coordinate of the component or the shift along x axis depending
on the value of RelativePosition property.</LI>
<LI> Y - New y coordinate of the component or the shift along y axis depending
on the value of RelativePosition property.</LI>
<LI> Z - New z coordinate of the component or the shift along z axis depending
on the value of RelativePosition property.</LI>
<LI> RelativePosition - Boolean. If false (X,Y,Z) is the new absolute position
of the component, if true (X,Y,Z) is the
shift and NewPos = OldPos + (X,Y,Z). The default value is true.</LI>
</UL>

@author Roman Tolchenov, Tessella Support Services plc
@date 21/01/2009
*/
class DLLExport MoveInstrumentComponent : public API::DistributedAlgorithm {
public:
  /// Default constructor
  MoveInstrumentComponent();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MoveInstrumentComponent"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Moves an instrument component to a new position."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"RotateInstrumentComponent", "SetInstrumentParameter"};
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
