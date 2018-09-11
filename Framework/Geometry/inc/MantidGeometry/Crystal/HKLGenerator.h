#ifndef MANTID_GEOMETRY_HKLGENERATOR_H_
#define MANTID_GEOMETRY_HKLGENERATOR_H_

#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"

#include <boost/iterator/iterator_facade.hpp>

namespace Mantid {
namespace Geometry {

/** HKLGenerator

  HKLGenerator is a pseudo-container that helps in generating actual
  containers with V3D-objects representing Miller indices (HKL).

  It's a common task to generate lists of Miller indices. The simplest
  way of doing that is to simply create a nested loop structure that
  goes through all combinations of H, K, L within some limits and then
  put them into a container (vector, list, set, ...):

    for(int h = -hmin; h <= hmax; ++h) {
      for(int k = -kmin; k <= kmax; ++k) {
        for(int l = -lmin; l <= lmax; ++l) {
          hkls.emplace_back(h, k, l);
        }
      }
    }

  In most cases the list is not used like that, instead it's filtered
  using some criteria, for example a certain range of d-spacings or
  others, so the reflection needs to be checked first:

    ...
        hkl = V3D(h, k, l)
        if(isOk(hkl)) {
            hkls.push_back(hkl);
        }
    ...

  Instead of explicitly stating the triple-loop, HKLGenerator provides
  a shorter way for this process using a const_iterator. The first code
  example then becomes this:

    HKLGenerator generator(V3D(hmin, kmin, lmin), V3D(hmax, kmax, lmax));
    for(auto hkl = generator.begin(); hkl != generator.end(); ++hkl) {
        hkls.push_back(*hkl);
    }

  Or even shorter, using std::copy:

    HKLGenerator generator(V3D(hmin, kmin, lmin), V3D(hmax, kmax, lmax));
    std::copy(generator.begin(), generator.end(), std::back_inserter(hkls));

  It's also possible to use filters this way, but in a much easier fashion,
  using std::copy_remove_if (pre C++11) or std::copy_if (C++11):

    // pre C++11
    std::copy_remove_if(generator.begin(), generator.end(),
                        std::back_inserter(hkls), isNotOk)

    // C++11
    std::copy_if(generator.begin(), generator.end(), std::back_inserter(hkls),
                 isOk)

  See the documentation for HKLFilter for more details on how to perform
  actual filtering.

  Please be aware that the iterator increments infinitely if it passes the
  specified maximimum HKL. In that case K and L remain constant while H is
  incremented (until it overflows).

      @author Michael Wedel, ESS
      @date 23/09/2015

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
  /**
   * @brief The const_iterator class
   *
   * This class uses boost::iterator_facade to provide a const_iterator
   * interface to HKLGenerator. This makes it very easy to use the standard
   * library algorithms. It's a forward iterator, so it's not possible to use
   * operator[] for random access of specific HKLs (this would have to be
   * done on the generated container if that supports it).
   *
   * While the iterators can be instantiated directly, the intention is to
   * use HKLGenerator::begin() and HKLGenerator::end().
   */
  class MANTID_GEOMETRY_DLL const_iterator
      : public boost::iterator_facade<const_iterator, const Kernel::V3D &,
                                      boost::forward_traversal_tag> {
  public:
    const_iterator();

    explicit const_iterator(const Kernel::V3D &current);

    explicit const_iterator(const Kernel::V3D &hklMin,
                            const Kernel::V3D &hklMax);

  private:
    // Required for boost::iterator_facade to work
    friend class boost::iterator_core_access;

    void increment();

    /// Returns true if other is at the same position
    inline bool equal(const const_iterator &other) const {
      return this->m_h == other.m_h && this->m_k == other.m_k &&
             this->m_l == other.m_l;
    }

    /// Returns a const reference to the currently pointed at HKL.
    inline const Kernel::V3D &dereference() const { return m_hkl; }

    /// Required for compilation in VS. Forward iterator can not be used
    /// that way, so the implementation does nothing.
    inline void advance(difference_type) {}
    inline void decrement() {}

    int m_h, m_k, m_l;
    Kernel::V3D m_hkl;

    int m_hMax;
    int m_kMin, m_kMax;
    int m_lMin, m_lMax;
  };

  HKLGenerator(const Kernel::V3D &hklMin, const Kernel::V3D &hklMax);
  HKLGenerator(const Kernel::V3D &hklMinMax);
  HKLGenerator(int hMinMax, int kMinMax, int lMinMax);
  HKLGenerator(const UnitCell &unitCell, double dMin);

  virtual ~HKLGenerator() = default;

  /// Returns the number of HKLs to be generated.
  inline size_t size() const { return m_size; }

  /// Returns an iterator to the beginning of the sequence.
  inline const const_iterator &begin() const { return m_begin; }

  /// Returns an iterator which "points at" one element past the end.
  inline const const_iterator &end() const { return m_end; }

private:
  size_t getSize(const Kernel::V3D &min, const Kernel::V3D &max) const;

  const_iterator getBeginIterator() const;
  const_iterator getEndIterator() const;
  Kernel::V3D getEndHKL() const;

  Kernel::V3D m_hklMin;
  Kernel::V3D m_hklMax;

  size_t m_size;

  const_iterator m_begin;
  const_iterator m_end;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_HKLGENERATOR_H_ */
