// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
namespace Mantid {
namespace DataHandling {

/** RotateSource : Moves the source by a given angle taking into account the
  handedness. The centre of rotation is the sample's position and the rotation
  axis (X, Y, Z) is calculated from the instrument geometry as the axis
  perpendicular to the plane defined by the beam and "up" vectors.
*/
class DLLExport RotateSource : public API::Algorithm {
public:
  const std::string name() const override { return "RotateSource"; };
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"RotateInstrumentComponent"}; }
  const std::string category() const override { return "DataHandling\\Instrument"; };
  const std::string summary() const override { return "Rotates the source by a given angle"; };

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
