// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {

namespace Kernel {
class V3D;
}

namespace Geometry {
class ParameterMap;
}

namespace CurveFitting {
namespace Algorithms {

/// Simple data structure to store nominal detector values
/// It avoids some functions taking a huge number of arguments
struct DetectorParams {
  double l1;       ///< source-sample distance in metres
  double l2;       ///< sample-detector distance in metres
  Kernel::V3D pos; ///< Full 3D position
  double theta;    ///< scattering angle in radians
  double t0;       ///< time delay in seconds
  double efixed;   ///< final energy
};

/**
  Takes a workspace with X axis in TOF and converts it to Y-space where the
  transformation is defined
  by equation (7) in http://link.aip.org/link/doi/10.1063/1.3561493?ver=pdfcov
*/
class MANTID_CURVEFITTING_DLL ConvertToYSpace : public API::Algorithm {
public:
  /// Constructor
  ConvertToYSpace();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts workspace in units of TOF to Y-space as defined in "
           "Compton scattering field";
  }

  int version() const override;
  const std::string category() const override;

  /// Creates a POD struct containing the required detector parameters for this
  /// spectrum
  static DetectorParams getDetectorParameters(const API::MatrixWorkspace_const_sptr &ws, const size_t index);
  /// Retrieve a component parameter
  static double getComponentParameter(const Geometry::IComponent &det, const Geometry::ParameterMap &pmap,
                                      const std::string &name);
  /// Convert single time value to Y,Q & Ei values
  static void calculateY(double &yspace, double &qspace, double &ei, const double mass, const double tsec,
                         const double k1, const double v1, const DetectorParams &detpar);

private:
  void init() override;
  void exec() override;

  /// Perform the conversion to Y-space
  bool convert(const size_t index);
  /// Check and store appropriate input data
  void retrieveInputs();
  /// Create the output workspace
  void createOutputWorkspace();
  /// Compute & store the parameters that are fixed during the correction
  void cacheInstrumentGeometry();

  /// Input workspace
  API::MatrixWorkspace_sptr m_inputWS;
  /// The input mass in AMU
  double m_mass;
  /// Source-sample distance
  double m_l1;
  /// Sample position
  Kernel::V3D m_samplePos;

  /// Output workspace
  API::MatrixWorkspace_sptr m_outputWS;
  API::MatrixWorkspace_sptr m_qOutputWS;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
