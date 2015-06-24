#ifndef MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMKIKF_H_
#define MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMKIKF_H_

#include <cmath>

#include "MantidKernel/ClassMacros.h"

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include "MantidDataObjects/CalculateReflectometry.h"
#include "MantidDataObjects/ReflectometryTransform.h"

namespace Mantid {
namespace MDAlgorithms {
/**
class CalculateReflectometryK: Calculation type for converting to ki or kf given
a theta value (in degrees) and a wavelength
*/
class CalculateReflectometryK : public DataObjects::CalculateReflectometry {
private:
  double m_sin_theta_i;
  double m_sin_theta_f;

public:
  CalculateReflectometryK() : m_sin_theta_i(0.0), m_sin_theta_f(0.0) {}
  ~CalculateReflectometryK(){};

  void setThetaIncident(double thetaIncident) {
    m_sin_theta_i = sin(to_radians_factor * thetaIncident);
  }

  void setThetaFinal(double thetaFinal) {
    m_sin_theta_f = sin(to_radians_factor * thetaFinal);
  }

  double calculateDim0(double wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_sin_theta_i;
  }

  double calculateDim1(double wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_sin_theta_f;
  }
};

/** ReflectometryTransformKiKf : Type to transform from R vs Wavelength
  workspace to a 2D MDEW with dimensions of Ki and Kf.

  @date 2012-06-06

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ReflectometryTransformKiKf
    : public DataObjects::ReflectometryTransform {
public:
  ReflectometryTransformKiKf(double kiMin, double kiMax, double kfMin,
                             double kfMax, double incidentTheta,
                             int numberOfBinsQx = 100,
                             int numberOfBinsQz = 100);
  virtual ~ReflectometryTransformKiKf();

  /// Execute transformation
  virtual Mantid::API::MatrixWorkspace_sptr
  execute(Mantid::API::MatrixWorkspace_const_sptr inputWs) const;

  /// Execute transformation
  virtual Mantid::API::IMDEventWorkspace_sptr
  executeMD(Mantid::API::MatrixWorkspace_const_sptr inputWs,
            Mantid::API::BoxController_sptr boxController) const;

private:
  DISABLE_DEFAULT_CONSTRUCT(ReflectometryTransformKiKf)
  DISABLE_COPY_AND_ASSIGN(ReflectometryTransformKiKf)
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMKIKF_H_ */
