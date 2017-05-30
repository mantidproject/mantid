#ifndef MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_TWO_STEP_H_
#define MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_TWO_STEP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/Matrix.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMDAlgorithms/Integrate3DEvents.h"

namespace Mantid {
namespace MDAlgorithms {

/** @class IntegrateEllipsoidsTwoStep

  IntegrateEllipsoidsTwoStep provides a two pass peak integration algorithm.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport IntegrateEllipsoidsTwoStep : public API::Algorithm {
public:
  /// Get the name of this algorithm
  const std::string name() const override;
  /// Get the version of this algorithm
  int version() const override;
  /// Get the category of this algorithm
  const std::string category() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate Single Crystal Diffraction Bragg peaks using 3D "
           "ellipsoids.";
  }

private:
  void init() override;
  void exec() override;
  IntegrationParameters
  makeIntegrationParameters(const Kernel::V3D &peak_q) const;

  void qListFromHistoWS(Integrate3DEvents &integrator, API::Progress &prog,
                        DataObjects::Workspace2D_sptr &wksp,
                        const Kernel::DblMatrix &UBinv, bool hkl_integ);
  void qListFromEventWS(Integrate3DEvents &integrator, API::Progress &prog,
                        DataObjects::EventWorkspace_sptr &wksp,
                        const Kernel::DblMatrix &UBinv, bool hkl_integ);
  /// Calculate if this Q is on a detector
  void calculateE1(const API::DetectorInfo &detectorInfo);
  void runMaskDetectors(Mantid::DataObjects::PeaksWorkspace_sptr peakWS,
                        std::string property, std::string values);

  /// integrate a collection of strong peaks
  DataObjects::PeaksWorkspace_sptr
  integratePeaks(DataObjects::PeaksWorkspace_sptr peaks,
                 API::MatrixWorkspace_sptr ws);
  /// save all detector pixels
  std::vector<Kernel::V3D> E1Vec;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_TWO_STEP_H_ */
