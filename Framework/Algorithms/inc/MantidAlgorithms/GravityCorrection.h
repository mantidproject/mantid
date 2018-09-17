#ifndef GRAVITYCORRECTION_H_
#define GRAVITYCORRECTION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument_fwd.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>
#include <vector>

namespace Mantid {
// forward declarations
namespace API {
// class AlgorithmHistory;
class Progess;
class SpectrumInfo;
} // namespace API
namespace Geometry {
class DetectorInfo;
}
namespace Kernel {
class V3D;
}

namespace Algorithms {

/*! GravityCorrection : Correction of time-of-flight values and final angles,
 i.e. angles between the reflected beam and the sample, due to gravity for
 2D Workspaces.
 Copyright &copy; 2014-2017 ISIS Rutherford Appleton Laboratory, NScD Oak
 Ridge National Laboratory & European Spallation Source
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

class DLLExport GravityCorrection : public Mantid::API::Algorithm {
public:
  /// Empty constructor
  GravityCorrection() = default;
  /// Algorithm's name. @see Algorithm::name
  const std::string name() const override { return "GravityCorrection"; }
  /// Algorithm's version. @see Algorithm::version
  int version() const override { return (1); }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override {
    return "ILL\\Reflectometry;Reflectometry";
  }
  /// Algorithm's summary. @see Algorith::summary
  const std::string summary() const override {
    return "Correction of time-of-flight values and final angles, i.e. angles "
           "between the reflected beam and the sample, due to gravity for "
           "2DWorkspaces.";
  }
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

protected:
  /// Progress report & cancelling
  std::unique_ptr<Mantid::API::Progress> m_progress{nullptr};

private:
  Mantid::Geometry::PointingAlong m_beamDirection;
  Mantid::Geometry::PointingAlong m_upDirection;
  Mantid::Geometry::PointingAlong m_horizontalDirection;
  std::string m_slit1Name;
  std::string m_slit2Name;
  Mantid::API::MatrixWorkspace_sptr m_ws;
  Mantid::Geometry::Instrument_const_sptr m_virtualInstrument;
  Mantid::API::MatrixWorkspace_sptr m_virtualWorkspace;

  int m_numberOfMonitors{0};
  Mantid::Kernel::V3D m_sample3D{Mantid::Kernel::V3D(
      0.0, 0.0, 0.0)}; /// sample coordinates wavelength dependent

  std::map<double, size_t> m_finalAngles;
  std::map<double, size_t>::key_compare m_smallerThan =
      Mantid::Algorithms::GravityCorrection::m_finalAngles.key_comp();

  /// Initialisation code
  void init() override;
  /// Name of a string component wich may be defined in parameters file
  std::string componentName(std::string propertyName,
                            Mantid::Geometry::Instrument_const_sptr &instr);
  /// Final angle definition between source and sample
  double finalAngle(const double k, size_t i);
  /// Retrieve the coordinate of an instrument component
  double coordinate(
      const std::string componentName,
      Mantid::Geometry::PointingAlong direction,
      Mantid::Geometry::Instrument_const_sptr instrument = nullptr) const;
  /// Retrieve the coodinate of a detector component
  double coordinate(Mantid::Geometry::DetectorInfo &detectorInfo, size_t i,
                    Mantid::Geometry::PointingAlong direction) const;
  /// Retrieve the coordinate of an instrument component
  double coordinate(Mantid::API::SpectrumInfo &spectrumInfo, size_t i,
                    Mantid::Geometry::PointingAlong direction) const;
  /// Retrieve the coordinate of a vector
  double coordinate(Kernel::V3D &pos,
                    Mantid::Geometry::PointingAlong direction) const;
  /// Modify the coordinate of a Vector V3D
  void setCoordinate(Kernel::V3D &pos,
                     Mantid::Geometry::PointingAlong direction, double value);
  /// Generalise instrument setup (origin, handedness, coordinate system)
  void virtualInstrument();
  /// Ensure slits to exist and be correctly ordered
  void slitCheck();
  /// The corrected spectrum number for the initialSpectrumNumber
  size_t spectrumNumber(const double angle,
                        Mantid::API::SpectrumInfo &spectrumInfo, size_t i);
  /// Tells if the corresponding spectrum will be considered for execution
  bool spectrumCheck(Mantid::API::SpectrumInfo &spectrumInfo, size_t i);
  /// Parabola arc length
  double parabolaArcLength(const double arg, double constant = 1.) const;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*GRAVITYCORRECTION_H_*/
