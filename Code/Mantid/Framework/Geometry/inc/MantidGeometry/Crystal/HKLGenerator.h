#ifndef MANTID_GEOMETRY_HKLGENERATOR_H_
#define MANTID_GEOMETRY_HKLGENERATOR_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidKernel/V3D.h"

#include <boost/iterator/iterator_facade.hpp>

namespace Mantid {
namespace Geometry {

/** HKLGenerator : TODO: DESCRIPTION

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL HKLGenerator {
public:
  class const_iterator
      : public boost::iterator_facade<const_iterator, const Kernel::V3D &,
                                      boost::forward_traversal_tag> {
  public:
    const_iterator() {}

    explicit const_iterator(const Kernel::V3D &current);

    explicit const_iterator(const Kernel::V3D &hklMin,
                            const Kernel::V3D &hklMax);

    explicit const_iterator(const Kernel::V3D &hklMin,
                            const Kernel::V3D &hklMax,
                            const Kernel::V3D &current);

  private:
    friend class boost::iterator_core_access;

    void increment();

    bool equal(const const_iterator &other) const {
      return this->m_h == other.m_h && this->m_k == other.m_k &&
             this->m_l == other.m_l;
    }

    const Kernel::V3D &dereference() const { return m_hkl; }

    int m_h, m_k, m_l;
    Kernel::V3D m_hkl;

    int m_hMin, m_hMax;
    int m_kMin, m_kMax;
    int m_lMin, m_lMax;
  };

  HKLGenerator(const Kernel::V3D &hklMin, const Kernel::V3D &hklMax);
  HKLGenerator(const Kernel::V3D &hklMinMax);
  HKLGenerator(int hMinMax, int kMinMax, int lMinMax);
  HKLGenerator(const UnitCell &unitCell, double dMin);

  virtual ~HKLGenerator() {}

  size_t size() const { return m_size; }

  const_iterator begin() const;
  const_iterator end() const;

private:
  Kernel::V3D getEndHKL() const;
  size_t getSize(const Kernel::V3D &min, const Kernel::V3D &max) const;

  Kernel::V3D m_hklMin;
  Kernel::V3D m_hklMax;

  size_t m_size;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_HKLGENERATOR_H_ */
