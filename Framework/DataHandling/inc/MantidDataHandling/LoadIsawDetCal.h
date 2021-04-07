// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument_fwd.h"

namespace Mantid {
namespace Geometry {
class ComponentInfo;
}
namespace API {
class DetectorInfo;
}
namespace DataHandling {
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS, ORNL
 @date 12/02/2010
 */
class DLLExport LoadIsawDetCal : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadIsawDetCal"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Since ISAW already has the capability to calibrate the instrument "
           "using single crystal peaks, this algorithm leverages this in "
           "mantid. It loads in a detcal file from ISAW and moves all of the "
           "detector panels accordingly. The target instruments for this "
           "feature are SNAP and TOPAZ.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"SaveIsawDetCal"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\DataHandling;DataHandling\\Isaw"; }

  /// @copydoc Algorithm::validateInputs()
  std::map<std::string, std::string> validateInputs() override;

private:
  const double CM_TO_M = 0.01;

  struct ComponentScaling {
    std::string componentName;
    double scaleX;
    double scaleY;
  };

  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Set the center of the supplied detector name
  void center(const double x, const double y, const double z, const std::string &detname, const API::Workspace_sptr &ws,
              Geometry::ComponentInfo &componentInfo);

  Geometry::Instrument_sptr getCheckInst(const API::Workspace_sptr &ws);
  std::vector<std::string> getFilenames();

  void doRotation(Kernel::V3D rX, Kernel::V3D rY, Geometry::ComponentInfo &componentInfo,
                  const std::shared_ptr<const Geometry::IComponent> &comp, bool doWishCorrection = false);
  void applyScalings(API::Workspace_sptr &ws, const std::vector<ComponentScaling> &rectangularDetectorScalings);
};

} // namespace DataHandling
} // namespace Mantid
