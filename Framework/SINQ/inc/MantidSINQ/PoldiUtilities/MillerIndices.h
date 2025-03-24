// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/V3D.h"
#include "MantidSINQ/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Poldi {

/** MillerIndices :
 *
  Small helper class which holds Miller indices for use with other
  POLDI routines.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 14/03/2014
*/

class MANTID_SINQ_DLL MillerIndices {
public:
  MillerIndices(int h = 0, int k = 0, int l = 0);
  MillerIndices(const std::vector<int> &hkl);
  MillerIndices(const Kernel::V3D &hkl);

  int h() const;
  int k() const;
  int l() const;

  int operator[](int index);
  bool operator==(const MillerIndices &other) const;
  bool operator!=(const MillerIndices &other) const;

  const std::vector<int> &asVector() const;
  const Kernel::V3D &asV3D() const;

private:
  void populateVector();

  int m_h;
  int m_k;
  int m_l;

  std::vector<int> m_asVector;
  Kernel::V3D m_asV3D;
};
} // namespace Poldi
} // namespace Mantid
