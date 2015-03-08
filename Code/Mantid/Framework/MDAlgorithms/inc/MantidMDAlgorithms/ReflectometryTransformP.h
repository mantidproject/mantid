#ifndef MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMP_H_
#define MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMP_H_

#include <cmath>

#include "MantidKernel/System.h"
#include "MantidDataObjects/ReflectometryTransform.h"

namespace Mantid {
namespace MDAlgorithms {

/**
class CalculateReflectometryPBase: Base class for p-type transforms.
*/
class CalculateReflectometryPBase {
protected:
  double m_sinThetaInitial;
  double m_sinThetaFinal;

  CalculateReflectometryPBase(const double &thetaIncident) {
    m_sinThetaInitial = sin(M_PI / 180.0 * thetaIncident);
  }
  ~CalculateReflectometryPBase() {}

public:
  void setThetaFinal(const double &thetaFinal) {
    m_sinThetaFinal = sin(M_PI / 180.0 * thetaFinal);
  }
};

/**
class CalculateReflectometryDiffP: Calculates difference between ki and kf.
*/
class CalculateReflectometryDiffP : public CalculateReflectometryPBase {
public:
  CalculateReflectometryDiffP(const double &thetaInitial)
      : CalculateReflectometryPBase(thetaInitial) {}
  ~CalculateReflectometryDiffP(){};
  double execute(const double &wavelength) {
    double wavenumber = 2 * M_PI / wavelength;
    double ki = wavenumber * m_sinThetaInitial;
    double kf = wavenumber * m_sinThetaFinal;
    return ki - kf;
  }
};

/**
class CalculateReflectometrySumP: Calculates sum of ki and kf.
*/
class CalculateReflectometrySumP : public CalculateReflectometryPBase {
public:
  CalculateReflectometrySumP(const double &thetaInitial)
      : CalculateReflectometryPBase(thetaInitial) {}
  ~CalculateReflectometrySumP(){};
  double execute(const double &wavelength) {
    double wavenumber = 2 * M_PI / wavelength;
    double ki = wavenumber * m_sinThetaInitial;
    double kf = wavenumber * m_sinThetaFinal;
    return ki + kf;
  }
};

/** ReflectometryTransformP : Calculates workspace(s) of Pi and Pf based on the
  input workspace and incident theta angle.

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
class DLLExport ReflectometryTransformP : public ReflectometryTransform {
private:
  const double m_pSumMin;
  const double m_pSumMax;
  const double m_pDiffMin;
  const double m_pDiffMax;
  /// Object performing raw calculation to determine pzi + pzf
  mutable CalculateReflectometrySumP m_pSumCalculation;
  /// Object performing raw calculation to determine pzi - pzf
  mutable CalculateReflectometryDiffP m_pDiffCalculation;

public:
  ReflectometryTransformP(double pSumMin, double pSumMax, double pDiffMin,
                          double pDiffMax, double incidentTheta,
                          int numberOfBinsQx = 100, int numberOfBinsQz = 100);
  virtual ~ReflectometryTransformP();
  virtual Mantid::API::IMDEventWorkspace_sptr
  executeMD(Mantid::API::MatrixWorkspace_const_sptr inputWs,
            Mantid::API::BoxController_sptr boxController) const;
  /// Execute transformation
  virtual Mantid::API::MatrixWorkspace_sptr
  execute(Mantid::API::MatrixWorkspace_const_sptr inputWs) const;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMP_H_ */
