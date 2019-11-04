// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_HKLFILTERWAVELENGTH_H_
#define MANTID_GEOMETRY_HKLFILTERWAVELENGTH_H_

#include "MantidGeometry/Crystal/HKLFilter.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Geometry {

/** HKLFilterWavelength

  This implementation of HKLFilter filters reflections by a wavelength-
  range. The wavelength is calculated from the Q-vector, so the filter
  requires an orientation matrix.

      @author Michael Wedel, ESS
      @date 23/10/2015
*/
class MANTID_GEOMETRY_DLL HKLFilterWavelength final : public HKLFilter {
public:
  HKLFilterWavelength(const Kernel::DblMatrix &ub, double lambdaMin,
                      double lambdaMax);

  std::string getDescription() const noexcept override;
  bool isAllowed(const Kernel::V3D &hkl) const noexcept override;

protected:
  void checkProperLambdaRangeValues() const;

  Kernel::DblMatrix m_ub;
  double m_lambdaMin;
  double m_lambdaMax;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_HKLFILTERWAVELENGTH_H_ */
