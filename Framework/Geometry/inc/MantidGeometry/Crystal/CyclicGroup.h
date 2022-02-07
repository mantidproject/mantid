// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/DllConfig.h"

#include <memory>
#include <set>

namespace Mantid {
namespace Geometry {

/**
    @class CyclicGroup

    A cyclic group G has the property that it can be represented by
    powers of one symmetry operation S of order n:

        G = { S^1, S^2, ..., S^n = S^0 = I }

    The operation S^m is defined as carrying out the multiplication
    S * S * ... * S. To illustrate this, a four-fold rotation around
    the z-axis is considered. The symmetry operation representing the
    transformation by this symmetry element is "-y,x,z". This is also the
    first member of the resulting group:

        S^1 = S = -y,x,z

    Then, multiplying this by itself:

        S^2 = S * S = -x,-y,z
        S^3 = S * S * S = y,-x,z
        S^4 = S * S * S * S = x,y,z = I

    Thus, the cyclic group G resulting from the operation "-y,x,z" contains
    the following members:

        G = { S^1, S^2, S^3, I } = { -y,x,z; -x,-y,z; y,-x,z; x,y,z }

    This example shows in fact how the point group "4" can be generated as
    a cyclic group by the generator S = -y,x,z. Details about this
    are given for example in [1].

    In code, the example is very concise:

        Group_const_sptr pointGroup4 =
            GroupFactory::create<CyclicGroup>("-y,x,z");

    This is much more convenient than having to construct a Group,
    where all four symmetry operations would have to be supplied.

    Related to this class is ProductOfCyclicGroups, which provides an easy way
    to express a group that is the product of multiple cyclic groups
    (such as some point groups).

    [1] Shmueli, U. Acta Crystallogr. A 40, 559–567 (1984).
        http://dx.doi.org/10.1107/S0108767384001161

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 03/10/2014
  */
class MANTID_GEOMETRY_DLL CyclicGroup : public Group {
public:
  CyclicGroup(const std::string &symmetryOperationString);
  CyclicGroup(const SymmetryOperation &symmetryOperation);

protected:
  std::vector<SymmetryOperation> generateAllOperations(const SymmetryOperation &operation) const;
};

using CyclicGroup_sptr = std::shared_ptr<CyclicGroup>;
using CyclicGroup_const_sptr = std::shared_ptr<const CyclicGroup>;

} // namespace Geometry
} // namespace Mantid
