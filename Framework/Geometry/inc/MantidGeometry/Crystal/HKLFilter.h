#ifndef MANTID_GEOMETRY_HKLFILTER_H_
#define MANTID_GEOMETRY_HKLFILTER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"

#include <boost/shared_ptr.hpp>
#include <functional>

namespace Mantid {
namespace Geometry {

/** HKLFilter

  There are many ways to filter lists of Miller indices HKL. In order
  to be able to use HKLGenerator with arbitrary filters, HKLFilter provides
  a general interface for such filters.

  The abstract base class HKLFilter defines a pure virtual method
  HKLFilter::isAllowed(), which takes a V3D as argument and returns a boolean.
  Implementing classes can then implement this method, wrapping very different
  concepts of checking HKLs for certain characteristics.

  There are two general ways of using HKLFilters. When used "standalone",
  the isAllowed()-function can be used directly:

    if(filter->isAllowed(hkl)) {
      // do something
    }

  For interoperability with STL-algorithms, HKLFilter provides a method that
  returns a function-object with the filter:

    std::copy_if(generator.begin(), generator.end(),
                 hkls.begin(), filter->fn());

  Often, it's not enough to filter a list by one criterion. To this end,
  there are two implementations of HKLFilter that provide binary logic
  operations
  "and" (HKLFilterAnd) and "or" (HKLFilterOr). They can be constructed from two
  HKLFilter_const_sptrs, or, more conveniently, by using the operators & and |
  directly with those types. This makes it possible to combine filters in many
  ways:

    HKLFilter_const_sptr filter = (filter1 | filter2) & filter3;

  Lastly, the unary logic operation "not" (HKLFilterNot) is implemented. This is
  important for usability with the pre-C++11 algorithm std::remove_copy_if that
  works logically reversed compared to std::copy_if:

    std::remove_copy_if(generator.begin(), generator.end(), hkls.begin(),
                        (~filter)->fn());

  For actual implementations of HKLFilter, check out BasicHKLFilters, where some
  important filters are defined.

      @author Michael Wedel, ESS
      @date 06/09/2015

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
class MANTID_GEOMETRY_DLL HKLFilter {
public:
  virtual ~HKLFilter() = default;

  std::function<bool(const Kernel::V3D &)> fn() const;

  virtual std::string getDescription() const = 0;
  virtual bool isAllowed(const Kernel::V3D &hkl) const = 0;
};

using HKLFilter_const_sptr = boost::shared_ptr<const HKLFilter>;

/// Base class for unary logic operations for HKLFilter.
class MANTID_GEOMETRY_DLL HKLFilterUnaryLogicOperation : public HKLFilter {
public:
  HKLFilterUnaryLogicOperation(const HKLFilter_const_sptr &filter);

  /// Returns the operand of the function.
  const HKLFilter_const_sptr &getOperand() const { return m_operand; }

protected:
  HKLFilter_const_sptr m_operand;
};

/// Logical "Not"-operation for HKLFilter.
class MANTID_GEOMETRY_DLL HKLFilterNot : public HKLFilterUnaryLogicOperation {
public:
  /// Constructor, calls base class constructor, throws exception if filter is a
  /// null pointer.
  HKLFilterNot(const HKLFilter_const_sptr &filter)
      : HKLFilterUnaryLogicOperation(filter) {}

  std::string getDescription() const override;
  bool isAllowed(const Kernel::V3D &hkl) const override;
};

/// Base class for binary logic operations for HKLFilter.
class MANTID_GEOMETRY_DLL HKLFilterBinaryLogicOperation : public HKLFilter {
public:
  HKLFilterBinaryLogicOperation(const HKLFilter_const_sptr &lhs,
                                const HKLFilter_const_sptr &rhs);

  /// Returns the left-hand side operand of the operation.
  const HKLFilter_const_sptr &getLHS() const { return m_lhs; }

  /// Returns the right-hand side operand of the operation.
  const HKLFilter_const_sptr &getRHS() const { return m_rhs; }

protected:
  HKLFilter_const_sptr m_lhs;
  HKLFilter_const_sptr m_rhs;
};

/// Logical "And"-operation for HKLFilter.
class MANTID_GEOMETRY_DLL HKLFilterAnd : public HKLFilterBinaryLogicOperation {
public:
  /// Constructor, calls base class constructor, throws exception if either of
  /// the operands is null.
  HKLFilterAnd(const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs)
      : HKLFilterBinaryLogicOperation(lhs, rhs) {}

  std::string getDescription() const override;
  bool isAllowed(const Kernel::V3D &hkl) const override;
};

/// Logical "Or"-operation for HKLFilter.
class MANTID_GEOMETRY_DLL HKLFilterOr : public HKLFilterBinaryLogicOperation {
public:
  /// Constructor, calls base class constructor, throws exception if either of
  /// the operands is null.
  HKLFilterOr(const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs)
      : HKLFilterBinaryLogicOperation(lhs, rhs) {}

  std::string getDescription() const override;
  bool isAllowed(const Kernel::V3D &hkl) const override;
};

MANTID_GEOMETRY_DLL const HKLFilter_const_sptr
operator~(const HKLFilter_const_sptr &filter);

MANTID_GEOMETRY_DLL const HKLFilter_const_sptr
operator&(const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs);

MANTID_GEOMETRY_DLL const HKLFilter_const_sptr
operator|(const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_HKLFILTER_H_ */
