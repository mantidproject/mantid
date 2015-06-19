#ifndef MANTID_DATAOBJECTS_REFLECTOMETRYTRANFORMQXQZ_H_
#define MANTID_DATAOBJECTS_REFLECTOMETRYTRANFORMQXQZ_H_

#include "MantidDataObjects/ReflectometryTransform.h"
#include "MantidDataObjects/CalculateReflectometryQxQz.h"
#include "MantidKernel/ClassMacros.h"

namespace Mantid {

namespace DataObjects {

/** ReflectometryTranformQxQz : Type of ReflectometyTransform. Used to convert
 from an input R vs Wavelength workspace to a 2D MDEvent workspace with
 dimensions of QxQy.
 Transformation is specific for reflectometry purposes.

 @date 2012-05-29

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
class DLLExport ReflectometryTransformQxQz : public ReflectometryTransform {
private:
  const double m_qxMin;
  const double m_qxMax;
  const double m_qzMin;
  const double m_qzMax;
  const double m_inTheta;

  /// Two theta angles cache
  mutable std::vector<double> m_theta;
  /// Two theta widths cache
  mutable std::vector<double> m_thetaWidths;

public:
  /// Constructor
  ReflectometryTransformQxQz(double qxMin, double qxMax, double qzMin,
                             double qzMax, double incidentTheta,
                             int numberOfBinsQx = 100,
                             int numberOfBinsQz = 100);
  /// Destructor
  virtual ~ReflectometryTransformQxQz();
  /// Execute transformation
  virtual Mantid::API::MatrixWorkspace_sptr
  execute(Mantid::API::MatrixWorkspace_const_sptr inputWs) const;
  /// Execute MD transformation
  virtual Mantid::API::IMDEventWorkspace_sptr
  executeMD(Mantid::API::MatrixWorkspace_const_sptr inputWs,
            Mantid::API::BoxController_sptr boxController) const;
  // Execuate transformation using normalised polynomial binning
  virtual Mantid::API::MatrixWorkspace_sptr
  executeNormPoly(Mantid::API::MatrixWorkspace_const_sptr inputWs) const;

private:
  void
  initAngularCaches(const API::MatrixWorkspace_const_sptr &workspace) const;

  DISABLE_DEFAULT_CONSTRUCT(ReflectometryTransformQxQz)
  DISABLE_COPY_AND_ASSIGN(ReflectometryTransformQxQz)
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_REFLECTOMETRYTRANFORMQXQZ_H_ */
