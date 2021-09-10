// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {

/** GroupTransformation

  This class transforms Group-objects, using a SymmetryOperation. It can
  be used to transform a point- or space group into a different setting.

  The following example shows how to transform the Group P 1 2/m 1 to
  P 1 1 2/m, with unique c-axis:

    SpaceGroup_const_sptr sg =
        SpaceGroupFactory::Instance().createSpaceGroup("P 1 2/m 1");

    Group transformed = GroupTransformation("y,z,x")(*sg);

  Using the getInverse-method, the transformation can be reversed.

      @author Michael Wedel, ESS
      @date 01/11/2015
*/
class MANTID_GEOMETRY_DLL GroupTransformation {
public:
  GroupTransformation(const MatrixVectorPair<double, V3R> &operation);
  GroupTransformation(const std::string &operationString);

  virtual ~GroupTransformation() = default;

  Group operator()(const Group &other) const;

  GroupTransformation getInverse() const;

private:
  SymmetryOperation transformOperation(const SymmetryOperation &operation) const;

  void setInverseFromPair();

  MatrixVectorPair<double, V3R> m_matrixVectorPair;
  MatrixVectorPair<double, V3R> m_inversePair;
};

} // namespace Geometry
} // namespace Mantid
