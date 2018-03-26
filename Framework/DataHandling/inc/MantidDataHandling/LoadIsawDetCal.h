#ifndef MANTID_DATAHANDLING_LOADISAWDETCAL_H_
#define MANTID_DATAHANDLING_LOADISAWDETCAL_H_

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

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  const std::vector<std::string> seeAlso() const override {
    return {"SaveIsawDetCal"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\DataHandling;DataHandling\\Isaw";
  }

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
  void center(const double x, const double y, const double z,
              const std::string &detname, API::Workspace_sptr ws,
              Geometry::ComponentInfo &componentInfo);

  Geometry::Instrument_sptr getCheckInst(API::Workspace_sptr ws);
  std::vector<std::string> getFilenames();

  void doRotation(Kernel::V3D rX, Kernel::V3D rY,
                  Geometry::ComponentInfo &componentInfo,
                  boost::shared_ptr<const Geometry::IComponent> comp,
                  bool doWishCorrection = false);
  void applyScalings(
      API::Workspace_sptr &ws,
      const std::vector<ComponentScaling> &rectangularDetectorScalings);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIFFRACTIONEVENTREADDETCAL_H_*/
