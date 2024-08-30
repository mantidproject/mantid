// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Geometry {
class Goniometer;
}
} // namespace Mantid

namespace Mantid {
namespace Crystal {

using Mantid::Geometry::Goniometer;

/** Define the initial orientation of the sample with respect to the beam and instrument by giving the axes and
 *directions of rotations.
 */
class MANTID_CRYSTAL_DLL RotateSampleShape final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "RotateSampleShape"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Define the initial orientation of the sample with respect to the beam and instrument "
           "by giving the axes, angle and directions of rotations.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SetGoniometer"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Goniometer"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  void prepareGoniometerAxes(Goniometer &gon, const API::ExperimentInfo_sptr &ei);
  bool checkIsValidShape(const API::ExperimentInfo_sptr &ei, std::string &shapeXML, bool &isMeshShape);
};

} // namespace Crystal
} // namespace Mantid
