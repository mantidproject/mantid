#ifndef MANTID_CURVEFITTING_CONVERTTOYSPACE_H_
#define MANTID_CURVEFITTING_CONVERTTOYSPACE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace CurveFitting {

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

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ConvertToYSpace : public API::Algorithm {
public:
  /// Constructor
  ConvertToYSpace();

  const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Converts workspace in units of TOF to Y-space as defined in "
           "Compton scattering field";
  }

  int version() const;
  const std::string category() const;

  /// Creates a POD struct containing the required detector parameters for this
  /// spectrum
  static DetectorParams
  getDetectorParameters(const API::MatrixWorkspace_const_sptr &ws,
                        const size_t index);
  /// Retrieve a component parameter
  static double
  getComponentParameter(const Geometry::IComponent_const_sptr &comp,
                        const Geometry::ParameterMap &pmap,
                        const std::string &name);
  /// Convert single time value to Y,Q & Ei values
  static void calculateY(double &yspace, double &qspace, double &ei,
                         const double mass, const double tmicro,
                         const double k1, const double v1,
                         const DetectorParams &detpar);

private:
  void init();
  void exec();

  /// Perform the conversion to Y-space
  bool convert(const size_t i);
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
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CONVERTTOYSPACE_H_ */
