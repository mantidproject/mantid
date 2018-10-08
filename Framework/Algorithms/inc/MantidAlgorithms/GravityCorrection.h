// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GRAVITYCORRECTION_H_
#define GRAVITYCORRECTION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
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

class MANTID_ALGORITHMS_DLL GravityCorrection : public Mantid::API::Algorithm {
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
  /// Sign of a value
  template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

protected:
  /// Progress report & cancelling
  std::unique_ptr<Mantid::API::Progress> m_progress{nullptr};

private:
  Mantid::Geometry::PointingAlong m_beamDirection;
  Mantid::Geometry::PointingAlong m_upDirection;
  Mantid::Geometry::PointingAlong m_horizontalDirection;
  std::string m_slit1Name;
  std::string m_slit2Name;
  Mantid::API::MatrixWorkspace_const_sptr m_ws;
  Mantid::Geometry::Instrument_const_sptr m_virtualInstrument;
  double m_beam1;
  double m_beam2;

  std::map<double, size_t> m_finalAngles;
  std::map<double, size_t>::key_compare m_smallerThan =
      Mantid::Algorithms::GravityCorrection::m_finalAngles.key_comp();

  /// Initialisation code
  void init() override;
  /// Name of a string component wich may be defined in parameters file
  std::string componentName(const std::string &propertyName,
                            Mantid::Geometry::Instrument_const_sptr &instr);
  /// Final angle definition between source and sample
  double finalAngle(const double k, const double theta);
  /// Generalise instrument setup (origin, handedness, coordinate system)
  void virtualInstrument();
  /// Ensure slits to exist and be correctly ordered
  void slitCheck();
  /// The corrected spectrum number for the initialSpectrumNumber
  size_t spectrumNumber(const double angle, const double theta);
  /// Parabola arc length
  double parabolaArcLength(const double arg, double constant = 1.) const;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*GRAVITYCORRECTION_H_*/
