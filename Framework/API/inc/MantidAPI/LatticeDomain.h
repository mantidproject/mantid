// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_LATTICEDOMAIN_H_
#define MANTID_API_LATTICEDOMAIN_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace API {

/** LatticeDomain

  This domain stores V3D-objects as HKLs instead of double-values. It can be
  used to refine lattice parameters from HKL/d-pairs.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/04/2015
*/
class MANTID_API_DLL LatticeDomain : public FunctionDomain {
public:
  LatticeDomain(const std::vector<Kernel::V3D> &hkls);

  size_t size() const override;

  const Kernel::V3D &operator[](size_t i) const;

protected:
  std::vector<Kernel::V3D> m_hkls;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_LATTICEDOMAIN_H_ */
